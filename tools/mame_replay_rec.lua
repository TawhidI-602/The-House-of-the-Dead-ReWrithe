-- mame_replay_rec.lua: replay a recorded session (.inp) exactly as tools/replay_hotd.ps1 does (tools/mame_dump.lua
-- repeats the session's Lua inputs: coin / start, infinite lives and ammo, start stage, on the same emulated schedule)
-- and record the game state every frame (tools/mame_state_rec.lua, taps only). Run through tools\run_state_rec.ps1 -Name.
local dir = debug.getinfo(1, "S").source:match("^@(.*[\\/])") or "./"
dofile(dir .. "mame_state_rec.lua")
dofile(dir .. "mame_dump.lua")
