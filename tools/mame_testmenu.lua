-- mame_testmenu.lua: drive the game's operator test menu from Lua (headless) and take a screenshot after each step.
-- Environment: HOTD_DUMP_DIR (output, inside private\), HOTD_KEYS = comma-separated steps, each "T" (TEST button = Service Mode),
-- "S" (SERVICE button = Service 1), "W" (wait 1 s), or "G<x>:<y>" (set P1 gun X/Y port values and pull the trigger). The menu is entered at 20 s; a screenshot follows every step.
-- Changes to settings go to the -nvram_directory given to MAME (use a copy).
local out = os.getenv("HOTD_DUMP_DIR") or "."
local keys = os.getenv("HOTD_KEYS") or "T"
local machine = manager.machine
local ports = machine.ioport.ports
local test, service = ports[":IN0"].fields["Service Mode"], ports[":IN0"].fields["Service 1"]
local gun_x, gun_y, trigger = ports[":P1_X"].fields["Lightgun X"], ports[":P1_Y"].fields["Lightgun Y"], ports[":IN1"].fields["P1 Trigger"]
local stopping = false
local stop_sub = emu.add_machine_stop_notifier(function() stopping = true end)

local function press(f)
	f:set_value(1); emu.wait(0.15); f:set_value(0); emu.wait(0.6)
end

local function main()
	emu.wait(20)
	if stopping then return end
	local log = io.open(out .. "/steps.txt", "w")
	local n = 0
	for k in string.gmatch(keys, "[^,]+") do
		if stopping then return end
		local gx, gy = string.match(k, "^G(%d+):(%d+)$")
		if gx then gun_x:set_value(tonumber(gx)); gun_y:set_value(tonumber(gy)); emu.wait(0.2); press(trigger)
		elseif k == "T" then press(test) elseif k == "S" then press(service) else emu.wait(1) end
		emu.wait(0.5)
		if stopping then return end
		machine.video:snapshot()
		n = n + 1
		log:write(string.format("%d %s\n", n, k)); log:flush()
	end
	log:close()
	emu.wait(2)   -- let the game write its settings
	if not stopping then machine:exit() end
end
local co = coroutine.create(main)
local ok, err = coroutine.resume(co)
if not ok then print("mame_testmenu.lua error: " .. tostring(err)) end
