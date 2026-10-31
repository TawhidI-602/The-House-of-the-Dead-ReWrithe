-- mame_attract.lua: watch the attract loop (no coin, no start): game mode / sub-mode (0x51F494 / 0x51F4C8) every
-- 0.1 s to <out>/modes.txt (one line per change: emulated time, mode, sub-mode), and a snapshot every HOTD_EVERY
-- seconds (default 1). Environment: HOTD_DUMP_DIR (output, inside private\), HOTD_SECONDS (default 240).
-- Run through tools\run_mame_attract.ps1 (headless, -str and a wall-clock timeout).
local out = os.getenv("HOTD_DUMP_DIR") or "."
local total = tonumber(os.getenv("HOTD_SECONDS") or "240")
local every = tonumber(os.getenv("HOTD_EVERY") or "1")
local machine = manager.machine
local space = machine.devices[":maincpu"].spaces["program"]
local stopping = false
local stop_sub = emu.add_machine_stop_notifier(function() stopping = true end)

local function main()
	local log = io.open(out .. "/modes.txt", "w")
	local last = ""
	local t, nextShot, n = 0, every, 0
	while t < total and not stopping do
		emu.wait(0.1)
		t = t + 0.1
		local m = string.format("%d %d", space:read_u16(0x0051F494), space:read_u32(0x0051F4C8))
		if m ~= last then
			log:write(string.format("%.1f %s snap%d\n", t, m, n)); log:flush()
			last = m
		end
		if t >= nextShot then
			machine.video:snapshot()
			n = n + 1
			nextShot = nextShot + every
		end
	end
	log:close()
	if not stopping then machine:exit() end
end
local co = coroutine.create(main)
local ok, err = coroutine.resume(co)
if not ok then print("mame_attract.lua error: " .. tostring(err)) end
