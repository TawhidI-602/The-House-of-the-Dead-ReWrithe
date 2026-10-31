-- mame_flow_trace.lua: the attract's flow every frame (no coin, no start): one line per change of
-- "mode sub-mode scene_number event_index evt_pc" (0x51F494 / 0x51F4C8 / 0x520704 / 0x51F9D0 / 0x52092C), prefixed
-- with the frame number (emulated time x 57.52, the vblank rate), to <out>/flow.txt; the same line format as hotd's
-- HOTD_TRACE_FLOW (comparison). Snapshots every HOTD_EVERY seconds between HOTD_SNAP_FROM and HOTD_SNAP_TO
-- (emulated seconds; default none). Environment: HOTD_DUMP_DIR (output, inside private\), HOTD_SECONDS (default 60).
-- (The machine frame notifier stops firing with -video none here, so it polls with emu.wait once a frame.)
-- Run like tools\run_mame_attract.ps1 (headless, -video none, -str and a wall-clock timeout).
local out = os.getenv("HOTD_DUMP_DIR") or "."
local total = tonumber(os.getenv("HOTD_SECONDS") or "60")
local every = tonumber(os.getenv("HOTD_EVERY") or "0.25")
local snapFrom = tonumber(os.getenv("HOTD_SNAP_FROM") or "-1")
local snapTo = tonumber(os.getenv("HOTD_SNAP_TO") or "-1")
local hz = 57.52
local machine = manager.machine
local space = machine.devices[":maincpu"].spaces["program"]
local stopping = false
local stop_sub = emu.add_machine_stop_notifier(function() stopping = true end)

local function main()
	local log = io.open(out .. "/flow.txt", "w")
	local last = ""
	local nextShot = snapFrom
	while not stopping do
		emu.wait(1 / hz)
		local t = machine.time:as_double()
		local m = string.format("%d %d %d %d %X", space:read_i16(0x0051F494), space:read_u32(0x0051F4C8),
			space:read_i16(0x00520704), space:read_i8(0x0051F9D0), space:read_u32(0x0052092C))
		if m ~= last then
			log:write(string.format("%d %s %.3f\n", math.floor(t * hz + 0.5), m, t)); log:flush()
			last = m
		end
		if snapFrom >= 0 and t >= nextShot and t <= snapTo then
			machine.video:snapshot()
			nextShot = nextShot + every
		end
		if t >= total then break end
	end
	log:close()
	if not stopping then machine:exit() end
end
local co = coroutine.create(main)
local ok, err = coroutine.resume(co)
if not ok then print("mame_flow_trace.lua error: " .. tostring(err)) end
