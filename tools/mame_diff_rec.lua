-- mame_diff_rec.lua: the arcade side of the whole-game differential (docs/proof.md "Whole-game differential"):
-- from a start frame on, writes what the runtime needs to replay the arcade frame by frame and what it must reach.
-- Run through tools\run_diff_rec.ps1 (replays a recorded session with tools/mame_dump.lua, headless).
--
-- Output in HOTD_DIFF_DIR:
--   snapshot.bin   work RAM (0x500000, 1 MB) at the end of the start frame S (the runtime's --snapshot)
--   palette2.bin   palette RAM bank 2 (0x1802000, the polygon palette: 1024 words) at the same point (--diff-replay
--                  loads it: palette_grey_build copies it into work RAM 0x51CCB0 / 0x51D4B0)
--   state.rec      HOTDREC1 (tools/mame_state_rec.lua's format: the state map + the tasks run) for frames S..end
--   inputs.bin     "HOTDINP1", then per frame F > S: "INPT", u32 F, u32 run count, runs (u32 address, u32 length,
--                  bytes): every work RAM byte that changed between the end of frame F-1 and the start of frame F's
--                  game logic (frame_tick's frame_counter increment): the vblank interrupt (gun_input_update /
--                  gun_flash: the guns, the flash cycle), wait_vblank / inputs_read (the switches), and this script's
--                  session writes (lives, ammo, route: mame_dump.lua's lua_write; always listed, changed or not);
--                  then u32 count, per interrupt or session write in the middle of frame F's logic:
--                  u32 tasks started before it in the frame, u32 address, u32 data, u32 byte mask (the bus write)
--   full_<F>.bin   work RAM at the end of frame F every HOTD_DIFF_FULL_EVERY frames (the wider comparison)
-- Environment: HOTD_DIFF_DIR, HOTD_DIFF_CHAPTER (start at the HOTD_DIFF_DELAY-th frame end, default 30, after the
-- chapter byte 0x51F9D1 first holds this value in game mode 4) or HOTD_DIFF_START (a frame number), HOTD_DIFF_FULL_EVERY
-- (default 600), HOTD_DIFF_FRAMES (frames to record after the start, default: to the end of the run).

local out = os.getenv("HOTD_DIFF_DIR") or "."
local want_chapter = tonumber(os.getenv("HOTD_DIFF_CHAPTER") or "")
local start_at = tonumber(os.getenv("HOTD_DIFF_START") or "")
local delay = tonumber(os.getenv("HOTD_DIFF_DELAY") or "30")
local full_every = tonumber(os.getenv("HOTD_DIFF_FULL_EVERY") or "600")
local max_frames = tonumber(os.getenv("HOTD_DIFF_FRAMES") or "") or 1e12
local script_dir = debug.getinfo(1, "S").source:match("^@(.*[\\/])") or "./"
local map_path = script_dir .. "state_map.tsv"

local FRAME_END = 0x008000F0
local CUR_TASK = 0x005122A8
local FRAME_COUNTER = 0x0051F428
local GAME_MODE = 0x0051F494
local RAND_STATE = 0x0053E380
local CHAPTER = 0x0051F9D1
local TASK_MAGIC = 0x80018691
local RAM0, RAM1 = 0x00500000, 0x005FFFFF
local CHUNK = 1024

local machine = manager.machine
local space = machine.devices[":maincpu"].spaces["program"]

local ranges = {}
for line in io.lines(map_path) do
	if line:sub(1, 1) ~= "#" then
		local name, a, s = line:match("^([^\t]+)\t(%x+)\t(%x+)")
		if name then ranges[#ranges + 1] = {tonumber(a, 16), tonumber(s, 16)} end
	end
end
local function u32(v) return string.pack("<I4", v & 0xffffffff) end

local rec, inp, log = nil, nil, io.open(out .. "/diff_rec.log", "w")
local frame = 0            -- frame ends seen (mame_state_rec.lua's numbering)
local started = nil        -- the start frame S
local seen_chapter = nil   -- frame ends since the chapter was first seen
local tasks, seen = {}, {}
local end_ram = nil        -- work RAM at the last frame end
local at_start = false     -- the next frame_counter write is the frame's start
local stopping = false
local nsession = 0

local function dump(name, data)
	local f = io.open(out .. "/" .. name, "wb"); f:write(data); f:close()
end

local function record_state()
	local parts = {"FRAM", u32(frame), u32(space:read_u32(FRAME_COUNTER)), u32(space:read_u32(GAME_MODE)),
	               u32(space:read_u32(RAND_STATE))}
	for _, r in ipairs(ranges) do parts[#parts + 1] = space:read_range(r[1], r[1] + r[2] - 1, 8) end
	local blocks = {}
	for _, t in ipairs(tasks) do
		if t >= 0x500000 and t < 0x600000 and space:read_u32(t + 0xC) == TASK_MAGIC then
			local words = space:read_u32(t - 12)
			local size = (words - 4) * 4
			if size < 0x10 or size > 0x2000 then size = 0x10 end
			blocks[#blocks + 1] = u32(t) .. u32(size) .. space:read_range(t, t + size - 1, 8)
		end
	end
	parts[#parts + 1] = u32(#blocks)
	parts[#parts + 1] = table.concat(blocks)
	rec:write(table.concat(parts))
end

-- the bytes of b that differ from a (both RAM0..RAM1), as runs
local function diff_runs(a, b)
	local runs = {}
	local n = #a
	for c = 1, n, CHUNK do
		local e = math.min(c + CHUNK - 1, n)
		if a:sub(c, e) ~= b:sub(c, e) then
			local i = c
			while i <= e do
				if a:byte(i) ~= b:byte(i) then
					local j = i
					-- extend over equal gaps shorter than 4 bytes
					while j < e do
						local k = j + 1
						local gap = 0
						while k <= e and a:byte(k) == b:byte(k) and gap < 4 do k = k + 1; gap = gap + 1 end
						if k <= e and gap < 4 and a:byte(k) ~= b:byte(k) then j = k else break end
					end
					runs[#runs + 1] = {RAM0 + i - 1, b:sub(i, j)}
					i = j + 1
				else
					i = i + 1
				end
			end
		end
	end
	return runs
end

-- session writes asked for in the middle of a task: held to the next new task (or the frame's end), the point where
-- the replay applies them (DiffReplay::midUpTo), so that both sides see the same order (the ammo refill once came in
-- the middle of the player's own task: Chapter 1 / 2 frames 17826 / 36471)
local held = {}
local function flush_held()
	for _, w in ipairs(held) do
		if w[1] == 4 then space:write_u32(w[2], w[3]) else space:write_u8(w[2], w[3]) end
	end
	held = {}
end

diffrec_tap_task = space:install_write_tap(CUR_TASK, CUR_TASK + 3, "diffrec_task", function(offset, data, mask)
	if not seen[data] then
		if #held > 0 then flush_held() end
		seen[data] = true; tasks[#tasks + 1] = data
	end
end)

-- the frame's record for inputs.bin, written at its end: the runs found at its start, then the interrupt writes that
-- came in the middle of its logic
local pending = nil
local forced = {}   -- the session writes made between the last frame's end and this frame's start (listed always)

-- mame_dump.lua's session writes (lives, ammo, route: lua_write) come here: kept for the replay: between frames
-- written at once and listed as runs of the next frame; during the logic held to the next new task (see held) and
-- listed as a mid-frame write after the running task (holding them to the frame's end changed the game: the .inp's
-- human aim then missed)
function hotd_defer_write(size, addr, value)
	nsession = nsession + 1
	if pending and not at_start then
		held[#held + 1] = {size, addr, value}
		local a = addr & ~3
		local sh = (addr & 3) * 8
		local data = size == 4 and value or ((value & 0xFF) << sh)
		local mask = size == 4 and 0xFFFFFFFF or (0xFF << sh)
		pending.mid[#pending.mid + 1] = u32(#tasks) .. u32(a) .. u32(data) .. u32(mask)
	else
		if size == 4 then space:write_u32(addr, value) else space:write_u8(addr, value) end
		forced[#forced + 1] = {size, addr, value}
	end
end
diffrec_tap_counter = space:install_write_tap(FRAME_COUNTER, FRAME_COUNTER + 3, "diffrec_counter", function(offset, data, mask)
	if stopping or not at_start then return end
	at_start = false
	if started and end_ram then
		local now = space:read_range(RAM0, RAM1, 8)
		local runs = diff_runs(end_ram, now)
		for _, w in ipairs(forced) do
			runs[#runs + 1] = {w[2], w[1] == 4 and string.pack("<I4", w[3]) or string.char(w[3] & 0xFF)}
		end
		pending = {frame = frame, runs = runs, mid = {}}
	end
	forced = {}
end)

-- interrupt writes in the middle of a frame (the vblank interrupt comes when it comes: usually while wait_vblank
-- waits, sometimes during the logic). Process controls (i960 register "pc") bit 13 = interrupted state. Each is kept
-- with the number of tasks started so far in the frame, so the replay applies it after the task it interrupted.
-- The ranges: every 16-byte granule an interrupt was seen writing (probe of 200 s of play, docs/proof.md); arcfix2:
-- the whole palette / colour-row queues 0x51DCB0..0x51E18F (palette_flush empties a long list, past the probe's
-- granules: Chapter 1 frame 19967), coin_update's prompt records 0x51E820.. (their blink state, set in a chapter
-- change's long chapter_load) and vblank_words_upload's colours 0x520660..0x5206AF.
local pcs = machine.devices[":maincpu"].state["pc"]
local W = {{0x511F00, 0x511F1F}, {0x511FE0, 0x511FFF}, {0x512060, 0x512063}, {0x512340, 0x51234F}, {0x517CA0, 0x517CAF}, {0x51DCB0, 0x51E18F},
	{0x520660, 0x5206AF}, {0x51E820, 0x51E8EF}, {0x51F284, 0x51F44F}, {0x51F4D0, 0x51F5FF}, {0x51F700, 0x51F70F},
	{0x51F790, 0x51F7AF}, {0x51F990, 0x51F9AF}, {0x5430A0, 0x5430A3}, {0x510EC0, 0x510EC3}}
diffrec_mid = {}
for i, r in ipairs(W) do
	diffrec_mid[i] = space:install_write_tap(r[1], r[2], "diffrec_mid" .. i, function(offset, data, mask)
		if pending and not at_start and (pcs.value & 0x2000) ~= 0 then
			pending.mid[#pending.mid + 1] = u32(#tasks) .. u32(offset) .. u32(data) .. u32(mask)
		end
	end)
end

diffrec_tap_end = space:install_write_tap(FRAME_END, FRAME_END + 3, "diffrec_end", function(offset, data, mask)
	if stopping then return end
	if #held > 0 then flush_held() end
	if not started then
		if start_at and frame >= start_at then started = frame
		elseif want_chapter and space:read_u8(CHAPTER) == want_chapter and space:read_u32(GAME_MODE) & 0xFFFF == 4 then
			seen_chapter = (seen_chapter or 0) + 1
			if seen_chapter >= delay then started = frame end
		end
		if started then
			rec = io.open(out .. "/state.rec", "wb")
			local head = {"HOTDREC1", u32(#ranges)}
			for _, r in ipairs(ranges) do head[#head + 1] = u32(r[1]); head[#head + 1] = u32(r[2]) end
			rec:write(table.concat(head))
			inp = io.open(out .. "/inputs.bin", "wb")
			inp:write("HOTDINP1")
			log:write(string.format("start frame %d (time %.3f, frame_counter %d)\n", frame, machine.time:as_double(),
				space:read_u32(FRAME_COUNTER)))
			log:flush()
		end
	end
	if started then
		if frame - started > max_frames then
			stopping = true
			rec:close(); inp:close(); rec, inp = nil, nil
			log:write(string.format("end frame %d\n", frame)); log:close(); log = nil
			machine:exit()
			return
		end
		record_state()
		if pending and pending.frame == frame then
			local parts = {"INPT", u32(frame), u32(#pending.runs)}
			for _, r in ipairs(pending.runs) do parts[#parts + 1] = u32(r[1]) .. u32(#r[2]) .. r[2] end
			parts[#parts + 1] = u32(#pending.mid)
			parts[#parts + 1] = table.concat(pending.mid)
			inp:write(table.concat(parts))
		end
		pending = nil
		end_ram = space:read_range(RAM0, RAM1, 8)
		if frame == started then
			dump("snapshot.bin", end_ram)
			dump("palette2.bin", space:read_range(0x01802000, 0x018027ff, 8))
		end
		if full_every > 0 and (frame - started) % full_every == 0 then dump(string.format("full_%06d.bin", frame), end_ram) end
	end
	frame = frame + 1
	tasks, seen = {}, {}
	at_start = true
end)

diffrec_stop = emu.add_machine_stop_notifier(function()
	stopping = true
	if rec then rec:close(); rec = nil end
	if inp then inp:close(); inp = nil end
	if log then log:write(string.format("stopped at frame %d\n", frame)); log:close(); log = nil end
	print(string.format("mame_diff_rec.lua: %d frames, start %s, %d session writes", frame, tostring(started), nsession))
end)

-- (in this script's environment: a plain dofile runs in another one, where hotd_defer_write is not seen)
assert(loadfile(script_dir .. "mame_dump.lua", "t", _ENV))()
