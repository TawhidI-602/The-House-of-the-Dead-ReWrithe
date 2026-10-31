-- mame_sound_probe.lua (overnight audio, 2026-10-16): the arcade's sound commands during a play-through, the reference
-- for the runtime's sound (docs/runtime.md "Sound"). Runs tools/mame_dump.lua (coin, start, the shooting; its
-- environment: HOTD_SHOOT=3 auto-aim, HOTD_INFLIVES=1, HOTD_EVERY large = no dumps) and taps the sound board port
-- 0x1C80000: sound_send3 / the queue drain send each command as 3 bytes (0xA0 / 0xBE first). Logs one line per command:
--   <emulated seconds> <code> mode <game_mode> chapter <n> event <n>
-- to HOTD_SOUND_LOG (inside private\). Run with -sound none (nothing is played), -str and a wall-clock timeout.
local log = io.open(os.getenv("HOTD_SOUND_LOG") or "sound_probe.txt", "w")
local machine = manager.machine
local space = machine.devices[":maincpu"].spaces["program"]
local bytes = {}
sound_probe_tap = space:install_write_tap(0x01c80000, 0x01c80003, "sound_probe", function(offset, data, mask)
	local b = data & 0xff
	if offset & 2 ~= 0 then b = (data >> 16) & 0xff end
	if #bytes == 0 and b ~= 0xa0 and b ~= 0xbe then return end
	bytes[#bytes + 1] = b
	if #bytes == 3 then
		local code = bytes[1] << 16 | bytes[2] << 8 | bytes[3]
		bytes = {}
		log:write(string.format("%.3f %06X mode %d chapter %d event %d\n", machine.time:as_double(), code,
			space:read_u16(0x51f494), space:read_u8(0x51f9d1), space:read_u8(0x51f9d0)))
		log:flush()
	end
end)
local dir = debug.getinfo(1, "S").source:match("^@(.*[\\/])") or "./"
dofile(dir .. "mame_dump.lua")
