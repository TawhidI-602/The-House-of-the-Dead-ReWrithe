-- mame_capture.lua: routine-level capture for the proof harness (docs/proof.md). Uses MAME's debugger headless
-- (-debug -debugger none): a breakpoint at a routine's entry and at each of its `ret` instructions; per call it writes
-- the global registers and the whole work RAM at entry and at the matching ret (same frame pointer), plus the number of
-- coprocessor calls started so far (tools/mame_copro_probe.lua must be loaded first, with HOTD_PROBE_FUNCS=all), so
-- hotd_proof can bring its Tgp to the same state and compare the coprocessor calls the routine makes.
-- Boundary routines (not ported yet) called inside a capture are recorded as their effect: the RAM bytes they changed.
-- Environment:
--   HOTD_CAP_FILE      output (inside private\)
--   HOTD_CAP_FUNCS     comma list of hex routine addresses to capture ("addr:size" overrides the index size)
--   HOTD_CAP_BOUNDARY  comma list of hex routine addresses recorded as effects inside a capture
--   HOTD_CAP_MAX       captures per routine (default 50)
--   HOTD_CAP_IF        optional "addr:mask,addr:mask": capture only when one of these u32 & mask != 0 at entry
--   HOTD_CAP_IF_ARG    optional "off:mask": capture only when the u32 at (g0 + off) & mask != 0 (g0 = first argument)
--   HOTD_CAP_IF_NE     optional "addr:addr,...": capture only when one of these pairs of u32 differ at entry (proofs-7:
--                      depth_lists_task's list pointers 520934:520938)
--   HOTD_CAP_MINCALLS  keep a capture only if the routine made at least this many coprocessor calls (default 0)
--   HOTD_CAP_SKIP      optional: of the calls that pass HOTD_CAP_IF / _IF_ARG, the first n of each routine are not
--                      captured (default 0)
--   HOTD_CAP_EVERY     optional: after that, every n-th of them is captured (default 1: all) ...
--   HOTD_CAP_ALWAYS    ... and also, between those, the calls where one of these "addr:mask" u32 & mask != 0
--   HOTD_CAP_INDEX     maincpu_index.tsv (routine sizes, to find the ret instructions)
--   HOTD_CAP_COVER     optional text file: branch coverage of the ported routines inside the captures (every
--                      conditional branch and which of its two edges ran while a capture was open: a breakpoint on the
--                      branch, one single step, the new pc tells the edge; removed once both edges have run)
-- File: "HOTDCAP1"; per capture: "CAPT", u32 routine, u32 seq, u32 copro calls at entry, 18 u32 (g0..g14, fp, sp, pfp),
--   RAM 0x500000..0x5FFFFF; per boundary call inside it: "BND3", u32 routine, u32 copro calls at entry, at exit,
--   u32 g0 at the return (its result), 8 u32 g0..g7 at its entry (its arguments; "BND2" records have none),
--   u32 run count, runs (u32 address, u32 length, bytes); "CEXT", u32 copro calls at exit, 18 u32, RAM.
local machine = manager.machine
local cpu = machine.devices[":maincpu"]
local space = cpu.spaces["program"]
local dbg = machine.debugger
local out = os.getenv("HOTD_CAP_FILE") or "capture.bin"
local maxn = tonumber(os.getenv("HOTD_CAP_MAX") or "50")
local mincalls = tonumber(os.getenv("HOTD_CAP_MINCALLS") or "0")
local skipn = tonumber(os.getenv("HOTD_CAP_SKIP") or "0")
local every = math.max(1, tonumber(os.getenv("HOTD_CAP_EVERY") or "1"))
local always = {}
for a, m in (os.getenv("HOTD_CAP_ALWAYS") or ""):gmatch("(%x+):(%x+)") do always[#always + 1] = {tonumber(a, 16), tonumber(m, 16)} end
local RAM0, RAM1 = 0x500000, 0x5FFFFF
local REGS = {"g0", "g1", "g2", "g3", "g4", "g5", "g6", "g7", "g8", "g9", "g10", "g11", "g12", "g13", "g14", "fp", "sp", "pfp"}

local sizes = {}
for line in io.lines(os.getenv("HOTD_CAP_INDEX")) do
	local a, _, s = line:match("^(%x+)\t([^\t]*)\t(%d+)")
	if a then sizes[tonumber(a, 16)] = tonumber(s) end
end
local conds = {}
for a, m in (os.getenv("HOTD_CAP_IF") or ""):gmatch("(%x+):(%x+)") do conds[#conds + 1] = {tonumber(a, 16), tonumber(m, 16)} end

local neconds = {}
for a, b in (os.getenv("HOTD_CAP_IF_NE") or ""):gmatch("(%x+):(%x+)") do neconds[#neconds + 1] = {tonumber(a, 16), tonumber(b, 16)} end

local argcond = nil
do
	local o, m = (os.getenv("HOTD_CAP_IF_ARG") or ""):match("(%x+):(%x+)")
	if o then argcond = {tonumber(o, 16), tonumber(m, 16)} end
end
local f = io.open(out, "wb")
f:write("HOTDCAP1")
local function u32(v) return string.pack("<I4", v & 0xffffffff) end
-- HOTD_CAP_XRAM = "addr:len,...": memory outside work RAM the ported routines read (palette bank 2 0x1802000:800),
-- recorded at entry after the CAPT record as "XRAM", u32 address, u32 length, bytes (proofs-4)
local xram = {}
for a, l in (os.getenv("HOTD_CAP_XRAM") or ""):gmatch("(%x+):(%x+)") do xram[#xram + 1] = {tonumber(a, 16), tonumber(l, 16)} end
local function xram_parts(tag)
	local t = {}
	for _, x in ipairs(xram) do t[#t + 1] = (tag or "XRAM") .. u32(x[1]) .. u32(x[2]) .. space:read_range(x[1], x[1] + x[2] - 1, 8) end
	return table.concat(t)
end
-- (proofs-5) the same ranges at exit, after the CEXT record, as "XOUT": hotd_proof compares them with the port's
local function regs()
	local t = {}
	for i, r in ipairs(REGS) do t[i] = u32(cpu.state[r].value) end
	return table.concat(t)
end
local function copro_count() return copro_probe_started and copro_probe_started() or 0 end
local function ram() return space:read_range(RAM0, RAM1, 8) end

-- changed byte runs between two RAM images (compared in 4 KB pages first)
local function diff_runs(a, b)
	local runs, n = {}, 0
	for page = 1, #a, 4096 do
		local e = page + 4095
		if a:sub(page, e) ~= b:sub(page, e) then
			local i = page
			while i <= e do
				if a:byte(i) ~= b:byte(i) then
					local j = i
					while j < e and a:byte(j + 1) ~= b:byte(j + 1) do j = j + 1 end
					runs[#runs + 1] = u32(RAM0 + i - 1) .. u32(j - i + 1) .. b:sub(i, j)
					n = n + 1
					i = j + 1
				else
					i = i + 1
				end
			end
		end
	end
	return n, table.concat(runs)
end

local entries, boundary, rets = {}, {}, {}   -- rets: ret address -> entry address
local lazy = {}   -- breakpoints needed only while a capture is open (boundaries, call sites): enabled on demand
local function lazy_bp(a)
	local i = cpu.debug:bpset(a, "1", "")
	cpu.debug:bpdisable(i)
	lazy[#lazy + 1] = i
end
local cover = {}     -- conditional branches in order: {routine, at, op, target, fall, bp, taken, fell}
local covat = {}     -- branch address -> its entry
local function cov_done(b) return b.taken and b.fell end
local function set_lazy(on)
	for _, i in ipairs(lazy) do if on then cpu.debug:bpenable(i) else cpu.debug:bpdisable(i) end end
	for _, b in ipairs(cover) do
		if on and not cov_done(b) then cpu.debug:bpenable(b.bp) else cpu.debug:bpdisable(b.bp) end
	end
end
local cov_pending = nil
local cover_file = os.getenv("HOTD_CAP_COVER") or ""
local function watch(a, size)
	cpu.debug:bpset(a, "1", "")
	local n = 0
	for p = a, a + size - 4, 4 do
		if space:read_u32(p) == 0x0A000000 then rets[p] = a; cpu.debug:bpset(p, "1", ""); n = n + 1 end
	end
	print(string.format("mame_capture.lua: %x, %d ret", a, n))
end
-- entries "addr" or "addr:size" (hex; the size overrides the index where Ghidra split the routine)
for h, sz in (os.getenv("HOTD_CAP_FUNCS") or ""):gmatch("(%x+):?(%x*)") do
	local a = tonumber(h, 16)
	entries[a] = {count = 0}
	watch(a, (sz ~= "" and tonumber(sz, 16)) or sizes[a] or 4)
end
-- boundaries: a breakpoint at the entry only; their return is caught at the instruction after each call site in the
-- ported routines (HOTD_CAP_PORTED, default the captured ones), with the caller's frame pointer: a boundary routine
-- may return through a ret outside its indexed size (a Ghidra split)
local retpoints = {}
local function insn_len(w)
	local op = w >> 24
	if op >= 0x80 and (w & 0x1000) ~= 0 then
		local mode = (w >> 10) & 0xF
		if mode == 0x5 or mode >= 0xC then return 8 end
	end
	return 4
end
for h, sz in (os.getenv("HOTD_CAP_PORTED") or os.getenv("HOTD_CAP_FUNCS") or ""):gmatch("(%x+):?(%x*)") do
	local a = tonumber(h, 16)
	local size = (sz ~= "" and tonumber(sz, 16)) or sizes[a] or 4
	local p, n = a, 0
	while p < a + size do
		local w = space:read_u32(p)
		local len = insn_len(w)
		local op = w >> 24
		if op == 0x09 or op == 0x86 or op == 0x66 then   -- call, callx, calls
			retpoints[p + len] = true; lazy_bp(p + len); n = n + 1
		end
		if cover_file ~= "" and ((op >= 0x10 and op <= 0x17) or (op >= 0x30 and op <= 0x3F)) then
			local d
			if op < 0x20 then d = w & 0xFFFFFC; if d & 0x800000 ~= 0 then d = d - 0x1000000 end
			else d = w & 0x1FFC; if d & 0x1000 ~= 0 then d = d - 0x2000 end end
			if not covat[p] then
				local br = {routine = a, at = p, op = op, target = (p + d) & 0xFFFFFFFF, fall = p + len,
				            bp = cpu.debug:bpset(p, "1", ""), taken = false, fell = false}
				cpu.debug:bpdisable(br.bp)
				cover[#cover + 1] = br
				covat[p] = br
			end
		end
		p = p + len
	end
	print(string.format("mame_capture.lua: %x, %d call sites", a, n))
end
for h in (os.getenv("HOTD_CAP_BOUNDARY") or ""):gmatch("%x+") do boundary[tonumber(h, 16)] = true; lazy_bp(tonumber(h, 16)) end
TASK_EXIT = 0x150D0   -- task_exit: a captured routine that calls it directly never returns
if not boundary[TASK_EXIT] then lazy_bp(TASK_EXIT) end

local cur = nil      -- the open capture: {routine, fp, parts}
-- other scripts (mame_dump.lua's lives / ammo refills) hold their RAM writes while a capture is open
function capture_is_open() return cur ~= nil end
local bopen = nil    -- the open boundary call {routine, calls, ram, callerfp}
dbg.execution_state = "run"
emu.register_periodic(function()
	if dbg.execution_state ~= "stop" then return end
	local pc = cpu.state["CURPC"].value
	local fp = cpu.state["fp"].value
	local e = entries[pc]
	if cov_pending then   -- one step after a conditional branch: which edge it took
		local b = cov_pending
		cov_pending = nil
		if pc == b.target then b.taken = true end
		if pc == b.fall then b.fell = true end
		if cov_done(b) then cpu.debug:bpdisable(b.bp) end
	end
	if bopen and retpoints[pc] and fp == bopen.callerfp then
		local b = bopen
		bopen = nil
		local n, runs = diff_runs(b.ram, ram())
		if cur then
			cur.parts[#cur.parts + 1] = "BND3" .. u32(b.routine) .. u32(b.calls) .. u32(copro_count()) ..
			                            u32(cpu.state["g0"].value) .. b.args .. u32(n) .. runs
		end
	end
	if e and cur and cur.routine == pc and fp <= cur.fp then
		cur, bopen = nil, nil   -- the open capture never returned (task_exit unwinds the task): dropped
		set_lazy(false)
	end
	if pc == TASK_EXIT and cur then   -- (also inside a boundary: the task unwinds through it)
		-- proofs-8: the captured routine itself calls task_exit (a boundary in -Boundary, not inside another boundary):
		-- the capture ends here, task_exit recorded as a boundary call that changed nothing and the state at its entry
		-- as the exit record (the port calls the task_exit stub and returns: nothing after it)
		if boundary[TASK_EXIT] and not bopen and (cpu.state["pfp"].value & 0xFFFFFFF0) == cur.fp and
		   copro_count() - cur.calls >= mincalls and f then
			local args = {}
			for i = 0, 7 do args[#args + 1] = u32(cpu.state["g" .. i].value) end
			local ent = entries[cur.routine]
			ent.count = ent.count + 1
			cur.parts[#cur.parts + 1] = "BND3" .. u32(TASK_EXIT) .. u32(copro_count()) .. u32(copro_count()) ..
			                            u32(cpu.state["g0"].value) .. table.concat(args) .. u32(0)
			cur.parts[#cur.parts + 1] = "CEXT" .. u32(copro_count()) .. regs() .. ram() .. xram_parts("XOUT")
			f:write(table.concat(cur.parts))
		end
		cur, bopen = nil, nil   -- the captured routine itself ends its task: it never returns; dropped now (an open capture
		set_lazy(false)   -- would block all later ones)
	end
	if e and not cur and e.count < maxn then
		local ok = #conds == 0
		for _, c in ipairs(conds) do if (space:read_u32(c[1]) & c[2]) ~= 0 then ok = true end end
		if argcond and (space:read_u32(cpu.state["g0"].value + argcond[1]) & argcond[2]) == 0 then ok = false end
		if #neconds > 0 then
			local ne = false
			for _, c in ipairs(neconds) do if space:read_u32(c[1]) ~= space:read_u32(c[2]) then ne = true end end
			if not ne then ok = false end
		end
		if ok and (skipn > 0 or every > 1) then   -- sampling (HOTD_CAP_SKIP / _EVERY / _ALWAYS)
			e.seen = (e.seen or 0) + 1
			local k = e.seen - skipn
			if k <= 0 then
				ok = false
			elseif (k - 1) % every ~= 0 then
				ok = false
				for _, c in ipairs(always) do if (space:read_u32(c[1]) & c[2]) ~= 0 then ok = true end end
			end
		end
		if ok then
			cur = {routine = pc, fp = fp, calls = copro_count(),
			       parts = {"CAPT" .. u32(pc) .. u32(e.count + 1) .. u32(copro_count()) .. regs() .. ram() .. xram_parts()}}
			set_lazy(true)
		end
	elseif boundary[pc] and cur and not bopen then   -- a boundary called inside another is part of its effect
		local args = {}
		for i = 0, 7 do args[#args + 1] = u32(cpu.state["g" .. i].value) end
		bopen = {routine = pc, calls = copro_count(), ram = ram(), callerfp = cpu.state["pfp"].value & 0xFFFFFFF0,
		         args = table.concat(args)}
	elseif rets[pc] and cur and cur.fp == fp and cur.routine == rets[pc] then
		bopen = nil
		set_lazy(false)
		if copro_count() - cur.calls >= mincalls and f then
			local ent = entries[cur.routine]
			ent.count = ent.count + 1
			cur.parts[#cur.parts + 1] = "CEXT" .. u32(copro_count()) .. regs() .. ram() .. xram_parts("XOUT")
			f:write(table.concat(cur.parts))
		end
		cur = nil
	end
	local cb = covat[pc]
	if cb and cur and not cov_done(cb) then
		cov_pending = cb
		cpu.debug:step(1)
		return
	end
	dbg.execution_state = "run"
end)
capture_stop = emu.add_machine_stop_notifier(function()
	if f then f:close(); f = nil end
	local s = {}
	for a, e in pairs(entries) do s[#s + 1] = string.format("%x:%d", a, e.count) end
	print("mame_capture.lua: captured " .. table.concat(s, " "))
	if cover_file ~= "" then
		local cf = io.open(cover_file, "w")
		cf:write("# routine\tbranch\topcode\ttarget\tfall-through\ttarget ran\tfall-through ran\n")
		for _, b in ipairs(cover) do
			cf:write(string.format("%x\t%x\t%02x\t%x\t%x\t%d\t%d\n", b.routine, b.at, b.op, b.target, b.fall,
			                       b.taken and 1 or 0, b.fell and 1 or 0))
		end
		cf:close()
	end
end)
