-- mame_state_rec.lua: the differential checker's arcade side (docs/decompilation.md "Differential checker (plan)", step 2).
-- Writes the game state of House of the Dead after EVERY frame (tick) to one binary file, for tools/state_diff.py.
-- Run through tools\run_state_rec.ps1 (headless, -video none).
--
-- How a frame is found: frame_tick (0xD900) ends every tick with a write of 0xF0F to 0x008000F0 (after the game mode's
-- routine); a write tap there records the state at exactly that point, whatever MAME does with video. Tasks: every task
-- task_group_run starts is first stored in the current-task word 0x5122A8, so a write tap there lists the tasks run in
-- this frame, in run order; each is dumped whole (its heap block: size word 12 bytes before the task, verified on the
-- fullgame_us dumps). Globals: the ranges of tools/state_map.tsv (gen_state_map.py).
--
-- Environment variables:
--   HOTD_REC_FILE   output file (keep it inside private\)
--   HOTD_REC_MAP    state map (default tools/state_map.tsv next to this script)
--   HOTD_SECONDS    emulated seconds to run before exiting (default 300)
--   HOTD_REC_FROM   first frame to record (default 0)
--   HOTD_REC_MODES  comma list of game_mode values to record (default: all)
--   HOTD_START      emulated second at which to insert coins and press 1P start (default 0 = never: attract mode only)
--   HOTD_COINS      coins before start (default 0)
--   HOTD_REC_TAPS_ONLY  1 = only install the taps (tools/mame_replay_rec.lua runs mame_dump.lua for the session)
-- File: "HOTDREC1", u32 map line count, then per map line: u32 address, u32 size; then per frame:
--   "FRAM", u32 frame number (ticks since boot), u32 frame_counter (0x51F428), u32 game_mode, u32 rand_state,
--   the map's ranges back to back, u32 task count, per task: u32 address, u32 size, the block.
-- All inputs go through MAME's input ports from this script; nothing is sent through Windows.

local out_path = os.getenv("HOTD_REC_FILE") or "state.rec"
local script_dir = debug.getinfo(1, "S").source:match("^@(.*[\\/])") or "./"
local map_path = os.getenv("HOTD_REC_MAP") or (script_dir .. "state_map.tsv")
local total = tonumber(os.getenv("HOTD_SECONDS") or "300")
local rec_from = tonumber(os.getenv("HOTD_REC_FROM") or "0")
local start_time = tonumber(os.getenv("HOTD_START") or "0")
local coins = tonumber(os.getenv("HOTD_COINS") or "0")
local modes = nil
if (os.getenv("HOTD_REC_MODES") or "") ~= "" then
	modes = {}
	for m in os.getenv("HOTD_REC_MODES"):gmatch("%d+") do modes[tonumber(m)] = true end
end

local FRAME_END = 0x008000F0   -- frame_tick: _DAT_008000f0 = 0xf0f after the mode routine
local CUR_TASK = 0x005122A8    -- task_group_run's current task
local FRAME_COUNTER = 0x0051F428
local GAME_MODE = 0x0051F494
local RAND_STATE = 0x0053E380
local TASK_MAGIC = 0x80018691

local machine = manager.machine
local space = machine.devices[":maincpu"].spaces["program"]
local ports = machine.ioport.ports
local coin, start = ports[":IN0"].fields["Coin 1"], ports[":IN0"].fields["1 Player Start"]

local ranges = {}
for line in io.lines(map_path) do
	if line:sub(1, 1) ~= "#" then
		local name, a, s = line:match("^([^\t]+)\t(%x+)\t(%x+)")
		if name then ranges[#ranges + 1] = {tonumber(a, 16), tonumber(s, 16)} end
	end
end

local f = io.open(out_path, "wb")
local function u32(v) return string.pack("<I4", v & 0xffffffff) end
local head = {"HOTDREC1", u32(#ranges)}
for _, r in ipairs(ranges) do head[#head + 1] = u32(r[1]); head[#head + 1] = u32(r[2]) end
f:write(table.concat(head))

local frame = 0
local frames_written = 0
local tasks, seen = {}, {}
local stopping = false

local function record()
	local mode = space:read_u32(GAME_MODE)
	if frame >= rec_from and (modes == nil or modes[mode]) then
		local parts = {"FRAM", u32(frame), u32(space:read_u32(FRAME_COUNTER)), u32(mode), u32(space:read_u32(RAND_STATE))}
		for _, r in ipairs(ranges) do parts[#parts + 1] = space:read_range(r[1], r[1] + r[2] - 1, 8) end
		local blocks = {}
		for _, t in ipairs(tasks) do
			-- the task may have ended this frame (block freed): keep it only while the magic is still there
			if t >= 0x500000 and t < 0x600000 and space:read_u32(t + 0xC) == TASK_MAGIC then
				local words = space:read_u32(t - 12)
				local size = (words - 4) * 4
				if size < 0x10 or size > 0x2000 then size = 0x10 end
				blocks[#blocks + 1] = u32(t) .. u32(size) .. space:read_range(t, t + size - 1, 8)
			end
		end
		parts[#parts + 1] = u32(#blocks)
		parts[#parts + 1] = table.concat(blocks)
		f:write(table.concat(parts))
		frames_written = frames_written + 1
	end
	frame = frame + 1
	tasks, seen = {}, {}
end

staterec_tap_end = space:install_write_tap(FRAME_END, FRAME_END + 3, "staterec_end", function(offset, data, mask)
	if not stopping then record() end
end)
staterec_tap_task = space:install_write_tap(CUR_TASK, CUR_TASK + 3, "staterec_task", function(offset, data, mask)
	if not seen[data] then seen[data] = true; tasks[#tasks + 1] = data end
end)

staterec_stop_sub = emu.add_machine_stop_notifier(function()
	stopping = true
	if f then f:close(); f = nil end
end)

local function main()
	local step = 1 / 30
	while true do
		local now = machine.time:as_double()
		if start_time > 0 then
			local t = now - start_time
			local pulse = t >= 0 and t < coins * 0.5 and (t % 0.5) < 0.2
			coin:set_value(pulse and 1 or 0)
			local s0 = coins * 0.5 + 0.5
			start:set_value((t >= s0 and t < s0 + 0.2) and 1 or 0)
		end
		if now >= total then
			print(string.format("mame_state_rec.lua: %d frames seen, %d recorded", frame, frames_written))
			if f then f:close(); f = nil end
			stopping = true
			machine:exit()
			return
		end
		local waited = emu.wait(step)
		if stopping or waited == false then return end
	end
end

if (os.getenv("HOTD_REC_TAPS_ONLY") or "") == "1" then
	-- replay mode (tools/mame_replay_rec.lua): mame_dump.lua drives the session and exits; report at the stop
	staterec_report = emu.add_machine_stop_notifier(function()
		print(string.format("mame_state_rec.lua: %d frames seen, %d recorded", frame, frames_written))
	end)
else
	local co = coroutine.create(main)
	local ok, err = coroutine.resume(co)
	if not ok then print("mame_state_rec.lua error: " .. tostring(err)) end
end
