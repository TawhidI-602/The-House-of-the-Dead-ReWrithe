-- mame_boot_writers.lua: who writes the work RAM from power-on to Chapter 1's chapter_load (the standalone start,
-- docs/teaser_status/s1.md). Run through tools\run_boot_writers.ps1 (headless: -video none).
--
-- Every write to work RAM 0x500000..0x5FFFFF is recorded per 16-byte granule: the first few distinct writer PCs
-- (CURPC) with the phase they wrote in (b = boot / attract before the coin, g = after the start button). When the
-- game writes the chapter group pointer 0x53E080 after the start (chapter_load's task_group_create), work RAM is dumped
-- (chapter_load_workram.bin) and the run ends a few frames later (after_workram.bin).
-- Environment: HOTD_DUMP_DIR output folder (inside private\), HOTD_START coin time (s, default 10), HOTD_COINS (2),
-- HOTD_SECONDS limit (default 60).
local out = os.getenv("HOTD_DUMP_DIR") or "."
local start_time = tonumber(os.getenv("HOTD_START") or "10")
local coins = tonumber(os.getenv("HOTD_COINS") or "2")
local total = tonumber(os.getenv("HOTD_SECONDS") or "60")

local machine = manager.machine
local cpu = machine.devices[":maincpu"]
local space = cpu.spaces["program"]
local ports = machine.ioport.ports
local function field(port, name) return ports[port].fields[name] end
local coin, start = field(":IN0", "Coin 1"), field(":IN0", "1 Player Start")

local stopping = false
local stop_sub = emu.add_machine_stop_notifier(function() stopping = true end)

local phase = "b"
local started = false
local loaded_at = nil
local modes = {}
local writers = {}   -- granule -> list of "pc:phase"
local seen = {}      -- granule .. pc .. phase -> true
local tap = space:install_write_tap(0x00500000, 0x005fffff, "bootwr", function(offset, data, mask)
	local g = (offset - 0x00500000) >> 4
	local pc = cpu.state["CURPC"].value
	local key = g * 0x10000000 + pc * 2 + (phase == "g" and 1 or 0)
	if not seen[key] then
		seen[key] = true
		local l = writers[g]
		if not l then l = {}; writers[g] = l end
		if #l < 6 then l[#l + 1] = string.format("%x:%s", pc, phase) end
	end
	if offset == 0x0051F494 and mask & 0xFFFF ~= 0 then
		modes[#modes + 1] = string.format("%.3f mode %x", machine.time:as_double(), data & 0xFFFF)
	end
	if started and not loaded_at and offset == 0x0053E080 then
		loaded_at = machine.time:as_double()
	end
end)

local function dump(name, first, last)
	local f = io.open(out .. "/" .. name, "wb")
	f:write(space:read_range(first, last, 8))
	f:close()
end

local function finish()
	local f = io.open(out .. "/writers.txt", "w")
	local keys = {}
	for g in pairs(writers) do keys[#keys + 1] = g end
	table.sort(keys)
	for _, g in ipairs(keys) do f:write(string.format("%06x %s\n", 0x500000 + g * 16, table.concat(writers[g], " "))) end
	f:close()
	tap:remove()
end

local function main()
	local step = 1 / 120
	local dumped = false
	while true do
		local now = machine.time:as_double()
		local t = now - start_time
		local pulse = t >= 0 and t < coins * 0.5 and (t % 0.5) < 0.2
		coin:set_value(pulse and 1 or 0)
		local s0 = coins * 0.5 + 0.5
		start:set_value((t >= s0 and t < s0 + 0.2) and 1 or 0)
		if t >= 0 and phase == "b" then phase = "g" end
		if t >= s0 then started = true end
		if loaded_at and not dumped then
			dump("chapter_load_workram.bin", 0x00500000, 0x005fffff)
			dumped = true
		end
		if (loaded_at and now > loaded_at + 0.1) or now >= total then
			dump("after_workram.bin", 0x00500000, 0x005fffff)
			print(table.concat(modes, " | "))
			dump("after_texram.bin", 0x11000000, 0x113fffff)
			dump("after_palette.bin", 0x01800000, 0x01803fff)
			dump("after_colorxlat.bin", 0x01810000, 0x0181bfff)
			finish()
			print(string.format("boot_writers: chapter_load at %s, end %.2f", tostring(loaded_at), now))
			machine:exit()
			return
		end
		local waited = emu.wait(step)
		if stopping or waited == false then return end
	end
end

local co = coroutine.create(main)
local ok, err = coroutine.resume(co)
if not ok then print("mame_boot_writers.lua error: " .. tostring(err)) end
