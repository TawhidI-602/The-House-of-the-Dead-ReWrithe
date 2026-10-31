-- mame_replay_probe.lua: replay a recorded session (.inp) exactly as tools/replay_hotd.ps1 does (tools/mame_dump.lua
-- repeats the session's Lua inputs) and log coprocessor calls (tools/mame_copro_probe.lua). Run through
-- tools\run_copro_probe.ps1 -Name.
local dir = debug.getinfo(1, "S").source:match("^@(.*[\\/])") or "./"
dofile(dir .. "mame_copro_probe.lua")
dofile(dir .. "mame_dump.lua")
