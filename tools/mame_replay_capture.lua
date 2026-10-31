-- mame_replay_capture.lua: replay a recorded session (.inp) as tools/replay_hotd.ps1 does, log every coprocessor call
-- (tools/mame_copro_probe.lua) and capture routines (tools/mame_capture.lua). Run through tools\run_capture.ps1.
local dir = debug.getinfo(1, "S").source:match("^@(.*[\\/])") or "./"
dofile(dir .. "mame_copro_probe.lua")
dofile(dir .. "mame_capture.lua")
dofile(dir .. "mame_dump.lua")
