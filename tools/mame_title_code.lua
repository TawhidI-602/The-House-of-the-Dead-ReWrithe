-- mame_title_code.lua: the title screen's start-button codes (title_code_screen 0xD2C0; docs/decompilation.md "Title
-- trigger codes"). Waits for the title (game mode 2 at 0x51F494), then plays HOTD_KEYS, comma-separated steps:
--   C  insert a coin (Coin 1)    L  P1 trigger (the left gun)    R  P2 trigger (the right gun)    S  P1 Start    U  P2 Start
--   W  wait 1 s                        T<x>:<y>  P1 gun at x / y with the trigger pulled
-- After each step: one line to <out>/steps.txt (emulated time, step, game mode / sub-mode, the code buffer 0x51F780 and
-- its length 0x51F790, the code's value 0x51F42C, P1's state / score) and a snapshot; then HOTD_AFTER seconds (default
-- 30) of play with a line and a snapshot every 2 s (the gun aimed at the middle, the trigger pulled every 0.5 s).
-- Environment: HOTD_DUMP_DIR (output, inside private\), HOTD_KEYS (default "C,C,L,L,L,R,R,S"), HOTD_AFTER.
-- Run through tools\run_mame_title_code.ps1 (headless, -str and a wall-clock timeout).
local out = os.getenv("HOTD_DUMP_DIR") or "."
local keys = os.getenv("HOTD_KEYS") or "C,C,L,L,L,R,R,S"
local after = tonumber(os.getenv("HOTD_AFTER") or "30")
local machine = manager.machine
local space = machine.devices[":maincpu"].spaces["program"]
local ports = machine.ioport.ports
local stopping = false
local stop_sub = emu.add_machine_stop_notifier(function() stopping = true end)

local function field(port, want)
	for name, f in pairs(ports[port].fields) do
		if name == want then return f end
	end
	local names = {}
	for name in pairs(ports[port].fields) do names[#names + 1] = name end
	print("mame_title_code.lua: no field '" .. want .. "' in " .. port .. ": " .. table.concat(names, ", "))
	return nil
end
local coin, start1, start2 = field(":IN0", "Coin 1"), field(":IN0", "1 Player Start"), field(":IN0", "2 Players Start")
local gun_x, gun_y, trigger = field(":P1_X", "Lightgun X"), field(":P1_Y", "Lightgun Y"), field(":IN1", "P1 Trigger")
local trigger2 = field(":IN1", "P2 Trigger")

local t = 0
local function wait(s) emu.wait(s); t = t + s end
local function press(f)
	if f then f:set_value(1) end
	wait(0.1)
	if f then f:set_value(0) end
	wait(0.4)
end
local function state()
	local code, len = "", space:read_i8(0x0051F790)
	for i = 0, 15 do
		local c = space:read_u8(0x0051F780 + i)
		if c == 0 then break end
		code = code .. string.char(c)
	end
	local pal = ""   -- the debug text's palette (slot 0 of the shadow 0x515CA0) and colour word 0x5123E0
	for i = 0, 15 do pal = pal .. string.format("%04X ", space:read_u16(0x00515CA0 + 2 * i)) end
	return string.format("mode %d sub %d code '%s' len %d char_pair 0x51F42C=%d colour %08X pal0 %s",
	                     space:read_u16(0x0051F494), space:read_u32(0x0051F4C8), code, len, space:read_u32(0x0051F42C),
	                     space:read_u32(0x005123E0), pal)
end

local function main()
	local log = io.open(out .. "/steps.txt", "w")
	while not stopping and space:read_u16(0x0051F494) ~= 2 do wait(0.1) end
	wait(1.0)
	log:write(string.format("%.1f title %s\n", t, state())); log:flush()
	machine.video:snapshot()
	for k in string.gmatch(keys, "[^,]+") do
		if stopping then return end
		local gx, gy = string.match(k, "^T(%d+):(%d+)$")
		if gx then gun_x:set_value(tonumber(gx)); gun_y:set_value(tonumber(gy)); wait(0.1); press(trigger)
		elseif k == "C" then press(coin)
		elseif k == "L" then press(trigger)
		elseif k == "R" then press(trigger2)
		elseif k == "S" then press(start1)
		elseif k == "U" then press(start2)
		else wait(1) end
		log:write(string.format("%.1f %s %s\n", t, k, state())); log:flush()
		machine.video:snapshot()
	end
	local next = t + 2
	local stop = t + after
	gun_x:set_value(0x180); gun_y:set_value(0xE9)
	while t < stop and not stopping do
		press(trigger)
		if t >= next then
			log:write(string.format("%.1f play %s\n", t, state())); log:flush()
			machine.video:snapshot()
			next = next + 2
		end
	end
	log:close()
	if not stopping then machine:exit() end
end
local co = coroutine.create(main)
local ok, err = coroutine.resume(co)
if not ok then print("mame_title_code.lua error: " .. tostring(err)) end
