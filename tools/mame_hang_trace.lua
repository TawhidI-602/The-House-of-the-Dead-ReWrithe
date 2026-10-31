-- mame_hang_trace.lua (hangcheck): replay a recorded session as tools/replay_hotd.ps1 does (tools/mame_dump.lua drives
-- it: set HOTD_DUMP_FROM past the end so it dumps nothing) and write, after EVERY game frame (frame_tick's end write of
-- 0xF0F to 0x008000F0), one text line about the Hangedman (the task whose routine is hangedman_fight_frame 0x4FAA0)
-- and the camera; plus a snapshot per game frame from HOTD_SNAP_FROM to HOTD_SNAP_TO (emulated seconds).
--   HOTD_TRACE_FILE  output text file (keep it inside private\)
-- Line: frame screen_frame time obj state x y z rot(0x2C 0x30 0x34) +0x50 +0x54 motion(0xD0) mframe(0xF4) +0x58
--       cam x y z | camera matrix (0x51F1F0.., 12 floats are not read: the eye is enough)
local dir = debug.getinfo(1, "S").source:match("^@(.*[\\/])") or "./"
local machine = manager.machine
local space = machine.devices[":maincpu"].spaces["program"]
local screen = machine.screens[":screen"]
local FRAME_END = 0x008000F0
local CUR_TASK = 0x005122A8
local FN = 0x0004FAA0
local out = io.open(os.getenv("HOTD_TRACE_FILE") or "hang_trace.txt", "w")
local snap_from = tonumber(os.getenv("HOTD_SNAP_FROM") or "0")
local snap_to = tonumber(os.getenv("HOTD_SNAP_TO") or "0")
local frame, hang = 0, nil
local function f32(a) return string.unpack("<f", string.pack("<I4", space:read_u32(a))) end
hang_tap_task = space:install_write_tap(CUR_TASK, CUR_TASK + 3, "hang_task", function(offset, data, mask)
	if data >= 0x500000 and data < 0x600000 and space:read_u32(data) == FN then hang = data end
end)
hang_tap_end = space:install_write_tap(FRAME_END, FRAME_END + 3, "hang_end", function(offset, data, mask)
	frame = frame + 1
	if not out then return end
	local o = hang
	local cam = string.format("%.3f %.3f %.3f", f32(0x51F250), f32(0x51F254), f32(0x51F258))
	if o and space:read_u32(o) == FN then
		out:write(string.format("%d %d %.4f %X %d %.4f %.4f %.4f %X %X %X %d %X %d %d %X %s\n", frame, screen:frame_number(),
			machine.time:as_double(), o, space:read_u16(o + 0x5C), f32(o + 0x20), f32(o + 0x24), f32(o + 0x28),
			space:read_u32(o + 0x2C), space:read_u32(o + 0x30), space:read_u32(o + 0x34), space:read_u32(o + 0x50),
			space:read_u32(o + 0x54), space:read_u16(o + 0xD0), space:read_u16(o + 0xF4), space:read_u32(o + 0x58), cam))
	else
		hang = nil
	end
end)
hang_stop = emu.add_machine_stop_notifier(function() if out then out:close(); out = nil end end)
if snap_to > snap_from then
	local co = coroutine.create(function()
		local last = -1
		while true do
			local now = machine.time:as_double()
			if now >= snap_from and now < snap_to and frame ~= last then
				last = frame
				machine.video:snapshot()
				if out then out:write(string.format("snap %d %.4f\n", frame, now)) end
			end
			if emu.wait(1 / 240) == false then return end
		end
	end)
	coroutine.resume(co)
end
dofile(dir .. "mame_dump.lua")
