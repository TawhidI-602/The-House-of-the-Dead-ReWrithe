-- mame_fifo_log.lua: log what the i960 writes to the geometry DSP's FIFO (function port 0x00880000-0x00883FFF and FIFO 0x00884000-0x00887FFF) for a short time window,
-- to see the real "object data" parameters (texture point address, texture header address, model address, count) per model.
-- Headless run (see run_mame_dump.ps1); inputs only through MAME's input ports.
-- Environment: HOTD_DUMP_DIR (output folder, inside private\), HOTD_START (coin + start time, default 40),
--              HOTD_LOG_AT (emulated second at which to log, default 48), HOTD_LOG_FOR (seconds to log, default 0.05)
local out = os.getenv("HOTD_DUMP_DIR") or "."
local start_time = tonumber(os.getenv("HOTD_START") or "40")
local log_at = tonumber(os.getenv("HOTD_LOG_AT") or "48")
local log_for = tonumber(os.getenv("HOTD_LOG_FOR") or "0.05")

local machine = manager.machine
local space = machine.devices[":maincpu"].spaces["program"]
local ports = machine.ioport.ports
local coin, start = ports[":IN0"].fields["Coin 1"], ports[":IN0"].fields["1 Player Start"]

local f = io.open(out .. "/fifo.txt", "w")
local logging = false
local count = 0
local tap = space:install_write_tap(0x00880000, 0x00887fff, "fifo", function(offset, data, mask)
	if logging then
		f:write(string.format("%08x %08x %08x\n", offset, data, mask))
		count = count + 1
	end
end)

local function main()
	while true do
		local now = machine.time:as_double()
		local t = now - start_time
		coin:set_value((t >= 0 and t < 0.2) and 1 or 0)
		start:set_value((t >= 1.0 and t < 1.2) and 1 or 0)
		logging = now >= log_at and now < log_at + log_for
		if now >= log_at + log_for then
			f:close()
			-- the i960 builds display lists in buffer RAM (0x00900000, 128 KB, read by the geometry DSP)
			local b = io.open(out .. "/bufferram.bin", "wb")
			b:write(space:read_range(0x00900000, 0x0091ffff, 8))
			b:close()
			machine.video:snapshot()
			print(string.format("fifo words logged: %d", count))
			machine:exit()
			return
		end
		emu.wait(1 / 240)
	end
end
local co = coroutine.create(main)
local ok, err = coroutine.resume(co)
if not ok then print("mame_fifo_log.lua error: " .. tostring(err)) end
