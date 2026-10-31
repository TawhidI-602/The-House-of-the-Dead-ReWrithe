-- attract (no input): every sound command written to the i8251 at 0x1C80000 with the emulated time, the game's frame
-- counter 0x51F428, mode / sub-mode; ends after HOTD_SECONDS
local out = os.getenv("HOTD_SOUND_LOG") or "sound.txt"
local total = tonumber(os.getenv("HOTD_SECONDS") or "120")
local log = io.open(out, "w")
local machine = manager.machine
local space = machine.devices[":maincpu"].spaces["program"]
local bytes = {}
snd_tap = space:install_write_tap(0x01c80000, 0x01c80003, "snd", function(offset, data, mask)
	local b = data & 0xff
	if offset & 2 ~= 0 then b = (data >> 16) & 0xff end   -- as tools/mame_sound_probe.lua
	if #bytes == 0 and b ~= 0xa0 and b ~= 0xbe then return end
	bytes[#bytes + 1] = b
	if #bytes == 3 then
		local code = bytes[1] << 16 | bytes[2] << 8 | bytes[3]
		bytes = {}
		log:write(string.format("%.4f %d %06X mode %d sub %d\n", machine.time:as_double(), space:read_u32(0x51f428), code,
			space:read_u16(0x51f494), space:read_u32(0x51f4c8)))
		log:flush()
	end
end)
local stopping = false
local stop_sub = emu.add_machine_stop_notifier(function() stopping = true end)
local co = coroutine.create(function()
	local t = 0
	while t < total and not stopping do emu.wait(0.5); t = t + 0.5 end
	log:close()
	if not stopping then machine:exit() end
end)
local ok, err = coroutine.resume(co)
if not ok then print("lua error: " .. tostring(err)) end
