-- mame_copro_probe.lua: log geometry-coprocessor calls and their results, to model the coprocessor's maths exactly
-- (tools/tgp_model.py). A call starts with a function-port write (0x880000 + n * 0x10, value n | n << 8) or with an
-- encoded command word pushed straight into the FIFO (n << 23 | n << 8 | n, e.g. 0x0D001A1A = function 0x1A).
-- Logs, per call of a selected function: the words written to the FIFO 0x884000 after the command, then the words
-- read back. Line format: "<command word> in <hex words> out <hex words>".
-- Environment: HOTD_PROBE_FILE (output, inside private\), HOTD_SECONDS (default 300),
--              HOTD_PROBE_FUNCS (comma list of hex function numbers, default 1f), HOTD_PROBE_MAX (calls per function,
--              default 5000), HOTD_PROBE_TOTAL (stop after this many logged calls, default unlimited).
--              HOTD_PROBE_START (emulated seconds before logging starts; the trace then starts with unknown state).
--              HOTD_PROBE_FUNCS=all logs every call in order (the matrix functions keep state in the coprocessor:
--              tools/tgp_model.py trace needs the whole sequence from boot). No inputs: the attract loop runs.
local out = os.getenv("HOTD_PROBE_FILE") or "copro.txt"
local total = tonumber(os.getenv("HOTD_SECONDS") or "300")
local maxn = tonumber(os.getenv("HOTD_PROBE_MAX") or "5000")
local totalmax = tonumber(os.getenv("HOTD_PROBE_TOTAL") or "0")
local tstart = tonumber(os.getenv("HOTD_PROBE_START") or "0")   -- emulated seconds before logging starts
local active = tstart <= 0
local logged = 0
local started = 0
-- calls started so far (tools/mame_capture.lua marks its captures with it)
function copro_probe_started() return started end
local funcs = {}
local fspec = os.getenv("HOTD_PROBE_FUNCS") or "1f"
if fspec == "all" then
	for n = 0, 255 do funcs[n] = 0 end
else
	for h in fspec:gmatch("%x+") do funcs[tonumber(h, 16)] = 0 end
end
local machine = manager.machine
local space = machine.devices[":maincpu"].spaces["program"]
local f = io.open(out, "w")
local cur = nil
local function flush()
	if cur and funcs[cur.n] < maxn then
		f:write(string.format("%08x in %s out %s\n", cur.cmd, table.concat(cur.ins, " "), table.concat(cur.outs, " ")))
		funcs[cur.n] = funcs[cur.n] + 1
		logged = logged + 1
	end
	cur = nil
end
local function start(n, word)
	flush()
	if not active then return end
	if funcs[n] then cur = {n = n, cmd = word, ins = {}, outs = {}}; started = started + 1 end
end
-- an encoded command word in the FIFO: the three copies of the function number agree (as the dispatcher 0x198 checks)
local function fifo_command(data)
	local n = data & 0xff
	if n ~= 0 and ((data >> 8) & 0xff) == n and ((data >> 23) & 0xff) == n and (data & 0x007f0000) == 0 then return n end
	return nil
end
copro_tap_port = space:install_write_tap(0x00880000, 0x00883fff, "probe_port", function(offset, data, mask)
	start((offset >> 4) & 0xff, data)
end)
copro_tap_in = space:install_write_tap(0x00884000, 0x00887fff, "probe_in", function(offset, data, mask)
	local n = fifo_command(data)
	if n then start(n, data) return end
	if cur then
		if #cur.outs > 0 then flush() else cur.ins[#cur.ins + 1] = string.format("%08x", data) end
	end
end)
copro_tap_out = space:install_read_tap(0x00884000, 0x00887fff, "probe_out", function(offset, data, mask)
	if cur then cur.outs[#cur.outs + 1] = string.format("%08x", data) end
end)
local function done()
	if totalmax > 0 and logged >= totalmax then return true end
	if fspec == "all" then return false end
	for _, c in pairs(funcs) do if c < maxn then return false end end
	return true
end
local function main()
	while true do
		if not active and machine.time:as_double() >= tstart then active = true end
		if machine.time:as_double() >= total or done() then
			flush(); f:close(); f = nil
			local s = {}
			for n, c in pairs(funcs) do if c > 0 then s[#s + 1] = string.format("%x:%d", n, c) end end
			print("mame_copro_probe.lua: calls logged " .. table.concat(s, " "))
			machine:exit()
			return
		end
		if emu.wait(0.1) == false then return end
	end
end
local co = coroutine.create(main)
local ok, err = coroutine.resume(co)
if not ok then print("mame_copro_probe.lua error: " .. tostring(err)) end
