-- mame_calib.lua: light-gun calibration log. On every P1 trigger press, write the gun's X/Y port values (what the game reads)
-- to calib_ports.txt in HOTD_DUMP_DIR. Exits after HOTD_CALIB_CLICKS presses (default 9), or after 5 minutes.
local out = os.getenv("HOTD_DUMP_DIR") or "."
local need = tonumber(os.getenv("HOTD_CALIB_CLICKS") or "9")
local machine = manager.machine
local ports = machine.ioport.ports
local px, py, pin1 = ports[":P1_X"], ports[":P1_Y"], ports[":IN1"]
local trig_mask = pin1.fields["P1 Trigger"].mask
local stopping = false
local stop_sub = emu.add_machine_stop_notifier(function() stopping = true end)

local function main()
	local f = io.open(out .. "/calib_ports.txt", "w")
	local idle = pin1:read() & trig_mask        -- trigger state when not pressed
	local was = false
	local n = 0
	while n < need and machine.time:as_double() < 300 do
		local pressed = (pin1:read() & trig_mask) ~= idle
		if pressed and not was then
			n = n + 1
			f:write(string.format("%d %.3f %d %d\n", n, machine.time:as_double(), px:read() & 0x3ff, py:read() & 0x3ff))
			f:flush()
		end
		was = pressed
		emu.wait(1 / 240)
		if stopping then f:close(); return end
	end
	f:close()
	emu.wait(1)
	if not stopping then machine:exit() end
end
local co = coroutine.create(main)
local ok, err = coroutine.resume(co)
if not ok then print("mame_calib.lua error: " .. tostring(err)) end
