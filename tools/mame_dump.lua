-- mame_dump.lua: dump the Model 2 color tables (and work RAM) of House of the Dead at regular intervals, with a screenshot each time.
-- Run through tools\run_mame_dump.ps1 (headless: -video none, which also skips MAME's start-up warning).
--
-- Environment variables (times in emulated seconds):
--   HOTD_DUMP_DIR  output folder (keep it inside private\: this is ROM-derived data)
--   HOTD_START     when to insert a coin and press 1P start (default 40; 0 = never, attract mode only)
--   HOTD_EVERY     seconds between dumps (default 5)
--   HOTD_DUMP_FROM first dump time (default: HOTD_EVERY)
--   HOTD_SECONDS   seconds to run before exiting (default 300)
--   HOTD_SHOOT     1 = keep firing at moving positions after the start, so the player survives longer (default 1);
--                  2 = one shot per second at the screen center from HOTD_SHOT_FROM seconds on (for finding the ammo counter);
--                  3 = auto-aim at the nearest shootable object (see autoaim_step; HOTD_AIM_SKIP excludes routines)
--   HOTD_COINS     coins to insert before pressing start (default 2: this machine is set to 2 credits per game)
--   HOTD_START2    1 = a 2-player game: as many coins again, then 2P start 0.5 s after 1P start; player 2 only stands
--                  there (HOTD_INFLIVES keeps its lives too) so that enemies with 2-player code (the dogs) run it
--   HOTD_INFLIVES  1 = infinite lives: keep the life counter (work RAM 0x0051F290, u32, starts at 3) at 3 after the start (default 0)
--   HOTD_INFAMMO   1 = infinite ammo: keep the ammo counter (work RAM 0x0051F2C1, byte, 6 = full) at 6 after the start (default 0)
--   HOTD_START_STAGE  stage to start the game in (work RAM 0x0051F9D1, byte: 0-3 = Chapters 1-4; written from 1 s before Start until
--                  HOTD_STAGE_HOLD s after Start is pressed; default: none = normal start)
--   HOTD_ROUTE     "chapter:event,...": route_choice forced to 1 in those events (the chapter's other branch; default none)
--   HOTD_TEXRAM    1 = also dump both texture RAMs (0x11000000-0x113FFFFF, 4 MB) at every dump (default 0)
--   HOTD_TEXRAM_EVERY  seconds between texture RAM dumps when HOTD_TEXRAM=1 (default: every dump)
--   HOTD_BUFRAM    1 = also dump buffer RAM (0x00900000-0x0097FFFF, 512 KB: the geometry DSP display lists the i960 builds,
--                  with one object command per drawn model) at every dump (default 0)
--   HOTD_FIFO_AT   log every write to the geometry coprocessor ports (0x00880000-0x00887FFF: function port + FIFO) from this
--                  emulated second on, for HOTD_FIFO_FOR seconds (default 0.1), to <out>/fifo.txt: "<time> <offset> <data>" 
-- All inputs go through MAME's input ports from this script; nothing is sent through Windows.
-- The script runs as a coroutine that waits on the emulated clock (emu.wait), because frame notifiers don't fire with -video none.
-- Output per dump: t<tenths>_palette.bin (0x01800000, 16 KB), t<tenths>_colorxlat.bin (0x01810000, 48 KB),
-- t<tenths>_luma.bin (0x11400000, 64 KB; luma bytes at even addresses), t<tenths>_workram.bin (0x00500000, 1 MB), a snapshot
-- (numbered in order), and a line in index.txt.

local out = os.getenv("HOTD_DUMP_DIR") or "."
local start_time = tonumber(os.getenv("HOTD_START") or "40")
local every = tonumber(os.getenv("HOTD_EVERY") or "5")
local total = tonumber(os.getenv("HOTD_SECONDS") or "300")
local shoot_mode = os.getenv("HOTD_SHOOT") or "1"
local shoot = shoot_mode == "1"
local shot_from = tonumber(os.getenv("HOTD_SHOT_FROM") or "0")
local coins = tonumber(os.getenv("HOTD_COINS") or "2")
local inflives = (os.getenv("HOTD_INFLIVES") or "0") == "1"
local LIFE_COUNTER = 0x0051F290
local LIFE_COUNTER_P2 = 0x0051F35C   -- players + 0xCC + 0xC
local start2_on = (os.getenv("HOTD_START2") or "0") == "1"
if start2_on then coins = coins * 2 end
local infammo = (os.getenv("HOTD_INFAMMO") or "0") == "1"
-- HOTD_CONTINUE n: up to n continues by player 1: once its state (0x51F284) has been 4 (out of lives, CONTINUE?) for
-- 2 s, the game's coins again, then 1P start (proofs-10: player_st1_continue, player_continue_start with 2 players)
local continues = tonumber(os.getenv("HOTD_CONTINUE") or "0")
local noskip = (os.getenv("HOTD_NOSKIP") or "0") == "1"
-- HOTD_HOLDFIRE "chapter:event:seconds,..." (proofs-11): the auto-aim holds its fire for that many seconds after the
-- game enters the event (e.g. "1:3:25": event 3's captor throws the researcher it carries, hostage_s03_thrown)
local holdfire, hold_ev, hold_until = {}, -1, 0
for item in (os.getenv("HOTD_HOLDFIRE") or ""):gmatch("[^,]+") do
	local c, e, sec = item:match("^%s*(%d+):(%d+):([%d.]+)")
	if c then holdfire[#holdfire + 1] = {tonumber(c) - 1, tonumber(e), tonumber(sec)} end
end
-- HOTD_FIRE_DUTY "chapter:event:on:off,..." (hangfix): in that event the auto-aim fires only `on` seconds out of
-- every `on + off` (counted from the event's start), so a boss takes its damage slowly
local duty = {}
for item in (os.getenv("HOTD_FIRE_DUTY") or ""):gmatch("[^,]+") do
	local c, e, on, off = item:match("^%s*(%d+):(%d+):([%d.]+):([%d.]+)")
	if c then duty[#duty + 1] = {tonumber(c) - 1, tonumber(e), tonumber(on), tonumber(off)} end
end
local duty_on, duty_off, duty_t0 = 0, 0, 0
local cont_t0 = nil
local coins1 = tonumber(os.getenv("HOTD_COINS") or "2")
local AMMO_COUNTER = 0x0051F2C1
local start_stage = tonumber(os.getenv("HOTD_START_STAGE") or "")
local stage_hold = tonumber(os.getenv("HOTD_STAGE_HOLD") or "3")
local STAGE_INDEX = 0x0051F9D1
-- HOTD_ROUTE "chapter:event[=value],..." (chapter 1-based): route_choice (0x51F270, byte) forced to value (default 1)
-- while the game is in one of those events (event_index 0x51F9D0, chapter 0x51F9D1): evt_NEXTEV's branch events
-- (route_branches 0xAAF80, type 1) go to their first target with 0, the second with 1
local routes = {}
for item in (os.getenv("HOTD_ROUTE") or ""):gmatch("[^,]+") do
	local c, e, v = item:match("^%s*(%d+):(%d+)=?(%d*)")
	if c then routes[#routes + 1] = {tonumber(c) - 1, tonumber(e), tonumber(v) or 1} end
end
local texram = (os.getenv("HOTD_TEXRAM") or "0") == "1"
local texram_every = tonumber(os.getenv("HOTD_TEXRAM_EVERY") or "0")
local last_tex_slot = -1
local bufram = (os.getenv("HOTD_BUFRAM") or "0") == "1"
local fifo_at = tonumber(os.getenv("HOTD_FIFO_AT") or "")
local fifo_for = tonumber(os.getenv("HOTD_FIFO_FOR") or "0.1")
local fifo_file, fifo_tap = nil, nil

local machine = manager.machine
local space = machine.devices[":maincpu"].spaces["program"]
local ports = machine.ioport.ports

local function field(port, name) return ports[port].fields[name] end
local coin, start, trigger = field(":IN0", "Coin 1"), field(":IN0", "1 Player Start"), field(":IN1", "P1 Trigger")
local start2 = start2_on and field(":IN0", "2 Players Start") or nil
local gun_x, gun_y = field(":P1_X", "Lightgun X"), field(":P1_Y", "Lightgun Y")

-- the session's RAM writes (lives, ammo, stage, route): at once, or, when tools/mame_diff_rec.lua runs too
-- (hotd_defer_write set) through it: written there and logged, so that a replay can repeat them at the same point
local function lua_write(size, addr, value)
	if hotd_defer_write then hotd_defer_write(size, addr, value)
	elseif size == 4 then space:write_u32(addr, value) else space:write_u8(addr, value) end
end

local function dump(name, first, last)
	local f = io.open(out .. "/" .. name, "wb")
	f:write(space:read_range(first, last, 8))
	f:close()
end

-- When MAME stops (Esc, -str) while this script waits in emu.wait, MAME resumes it during shutdown; touching the machine then
-- crashes MAME (access violation in lua_engine::on_machine_stop) and the -record file is lost. So stop as soon as MAME is stopping.
local stopping = false
local stop_sub = emu.add_machine_stop_notifier(function() stopping = true end)

if fifo_at then
	fifo_file = io.open(out .. "/fifo.txt", "w")
	fifo_tap = space:install_write_tap(0x00880000, 0x00887fff, "fifo", function(offset, data, mask)
		local now = machine.time:as_double()
		if fifo_file and now >= fifo_at and now < fifo_at + fifo_for then
			fifo_file:write(string.format("%.6f %08x %08x\n", now, offset, data))
		end
	end)
end

-- HOTD_SHOOT=3, auto-aim (as the PC harness's): player 1's gun steered at the nearest shootable object of the last frame
-- (tasks from the current-task word 0x5122A8, as mame_state_rec.lua; flags +0x58 bit 0, camera-space view_pos +0x38
-- projected with focal 496 as shot_ray_setup takes the aim), the trigger pulled while on it. The gun -> aim mapping
-- (gun_to_screen's operator calibration) is learnt from the game's own reading: after each shot the aim the game
-- took (player 1 +0x34 / +0x36) is paired with the gun value; two distinct pairs per axis give the line. Until then
-- it fires at two fixed points. Only game state decides the input, so runs repeat.
local aim_tasks, aim_cur = {}, {}
-- HOTD_AIM_SKIP: routine addresses (hex, comma-separated) the auto-aim never aims at (e.g. 6dbd0, the researchers'
-- frame, so their rescue paths run)
local aim_skip = {}
for h in string.gmatch(os.getenv("HOTD_AIM_SKIP") or "", "[%x]+") do aim_skip[tonumber(h, 16)] = true end
-- HOTD_AIM_PREFER: routine addresses the auto-aim aims at first when one is on screen (e.g. the hostages' states, so
-- a hostage is shot); otherwise the nearest object as usual (so the events still end)
local aim_prefer = {}
for h in string.gmatch(os.getenv("HOTD_AIM_PREFER") or "", "[%x]+") do aim_prefer[tonumber(h, 16)] = true end
if shoot_mode == "3" then
	autoaim_tap_task = space:install_write_tap(0x005122A8, 0x005122AB, "autoaim_task", function(offset, data, mask)
		aim_cur[#aim_cur + 1] = data
	end)
	autoaim_tap_end = space:install_write_tap(0x008000F0, 0x008000F3, "autoaim_end", function(offset, data, mask)
		aim_tasks, aim_cur = aim_cur, {}
	end)
end
local function f32(a) return string.unpack("<f", string.pack("<I4", space:read_u32(a))) end
local function s16(a) local v = space:read_u16(a); return v >= 0x8000 and v - 0x10000 or v end
local cal = {x = {}, y = {}}   -- per axis: samples {gun, aim}
local function cal_line(s)
	if #s < 2 then return nil end
	local p, q = s[1], s[#s]
	if math.abs(q[1] - p[1]) < 40 then return nil end
	local a = (q[2] - p[2]) / (q[1] - p[1])
	if a == 0 then return nil end
	return a, p[2] - a * p[1]
end
local aim_state = {press = false, gx = 233, gy = 233, last_ax = nil, last_ay = nil, n = 0}
local function autoaim_step()
	local st = aim_state
	local ax, ay = s16(0x0051F2B8), s16(0x0051F2BA)
	if ax ~= 1000 and (ax ~= st.last_ax or ay ~= st.last_ay) and st.shot_gx and not st.calibrated then
		-- a new reading: pair it with the gun values held at the trigger pull
		local function add(s, g, a)
			if #s < 2 then s[#s + 1] = {g, a} elseif math.abs(g - s[1][1]) > math.abs(s[2][1] - s[1][1]) then s[2] = {g, a} end
		end
		add(cal.x, st.shot_gx, ax); add(cal.y, st.shot_gy, ay)
	end
	st.last_ax, st.last_ay = ax, ay
	local axx, bx = cal_line(cal.x)
	local ayy, by = cal_line(cal.y)
	st.calibrated = axx ~= nil and ayy ~= nil   -- kept once learnt (later pairs can mix up shots)
	local tx, ty, best, bestp = nil, nil, 1e30, false
	for _, t in ipairs(aim_tasks) do
		if t >= 0x00500000 and t < 0x005FFF00 and (space:read_u32(t + 0x58) & 1) == 1 and not aim_skip[space:read_u32(t)] then
			local vz = f32(t + 0x40)
			local pref = aim_prefer[space:read_u32(t)] == true
			if vz > 5 and vz < 2000 and ((pref and not bestp) or (pref == bestp and vz < best)) then
				local sx, sy = f32(t + 0x38) * 496 / vz, f32(t + 0x3C) * 496 / vz
				if sx > -240 and sx < 240 and sy > -180 and sy < 180 then tx, ty, best, bestp = sx, sy, vz, pref end
			end
		end
	end
	if st.calibrated and space:read_u8(AMMO_COUNTER) == 0 then
		-- out of rounds: reload by shooting off screen (no RAM write, so captures stay clean)
		st.gx, st.gy = gun_x.minvalue, gun_y.minvalue
	elseif not (axx and ayy) then
		-- calibration shots at two fixed points (one second each)
		st.n = st.n + 1
		local k = math.floor(st.n / 15) % 10
		local function at(fl, q) return math.floor(fl.minvalue + (fl.maxvalue - fl.minvalue) * (0.2 + 0.06 * q)) end
		st.gx, st.gy = at(gun_x, k), at(gun_y, (k * 3) % 10)
		if st.n % 15 == 14 then
			aim_log = aim_log or io.open(out .. "/autoaim.txt", "w")
			aim_log:write(string.format("cal: gun %d %d -> aim %d %d (fields x %d..%d y %d..%d)\n", st.gx, st.gy, ax, ay,
				gun_x.minvalue, gun_x.maxvalue, gun_y.minvalue, gun_y.maxvalue))
		end
	elseif tx then
		st.gx = math.max(gun_x.minvalue, math.min(gun_x.maxvalue, math.floor((tx - bx) / axx + 0.5)))
		st.gy = math.max(gun_y.minvalue, math.min(gun_y.maxvalue, math.floor((ty - by) / ayy + 0.5)))
	else
		trigger:set_value(0); st.press = false
		return
	end
	gun_x:set_value(st.gx); gun_y:set_value(st.gy)
	st.log_n = (st.log_n or 0) + 1
	if st.log_n % 30 == 0 then
		aim_log = aim_log or io.open(out .. "/autoaim.txt", "w")
		aim_log:write(string.format("%.1f s: tasks %d target %s gun %d %d aim %d %d cal x %s y %s score %d ch %d ev %d\n",
			machine.time:as_double(), #aim_tasks, tx and string.format("%.0f %.0f", tx, ty) or "-", st.gx, st.gy, ax, ay,
			axx and string.format("%.3f %.1f", axx, bx) or "-", ayy and string.format("%.3f %.1f", ayy, by) or "-",
			space:read_u32(0x0051F28C), space:read_u8(0x0051F9D1), space:read_u8(0x0051F9D0)))
		aim_log:flush()
	end
	st.press = not st.press
	if st.press then st.shot_gx, st.shot_gy = st.gx, st.gy end
	trigger:set_value(st.press and 1 or 0)
end

local function main()
	local index = io.open(out .. "/index.txt", "a")
	local step = 1 / 30
	local next_dump = tonumber(os.getenv("HOTD_DUMP_FROM") or tostring(every))
	local shots = 0
	while true do
		local now = machine.time:as_double()
		if start_time > 0 then
			local t = now - start_time
			-- one 0.2 s coin pulse every 0.5 s, then start 0.5 s after the last coin
			local pulse = t >= 0 and t < coins * 0.5 and (t % 0.5) < 0.2
			local s0 = coins * 0.5 + 0.5
			local press = t >= s0 and t < s0 + 0.2
			if continues > 0 and t > s0 + 5 then
				if not cont_t0 and space:read_u32(0x0051F284) == 4 then cont_t0 = now + 2 end
				if cont_t0 then
					local u = now - cont_t0
					pulse = u >= 0 and u < coins1 * 0.5 and (u % 0.5) < 0.2
					local c0 = coins1 * 0.5 + 0.5
					press = u >= c0 and u < c0 + 0.2
					if u >= c0 + 1 then cont_t0 = nil; continues = continues - 1 end
				end
			end
			coin:set_value(pulse and 1 or 0)
			start:set_value(press and 1 or 0)
			if start2 then start2:set_value((t >= s0 + 0.5 and t < s0 + 0.7) and 1 or 0) end
			local free = not (capture_is_open and capture_is_open())   -- no refills inside a routine capture
			if free and inflives and t > coins * 0.5 + 1 then lua_write(4, LIFE_COUNTER, 3) end
			if free and inflives and start2 and t > coins * 0.5 + 1.5 then lua_write(4, LIFE_COUNTER_P2, 3) end
			if free and infammo and t > coins * 0.5 + 1 then lua_write(1, AMMO_COUNTER, 6) end
			if start_stage and t > s0 - 1 and t < s0 + stage_hold then lua_write(1, STAGE_INDEX, start_stage) end
			for _, r in ipairs(routes) do
				-- (verify: held inside a routine capture too, as the refills: a write there showed as a difference)
				if free and space:read_u8(0x0051F9D1) == r[1] and space:read_u8(0x0051F9D0) == r[2] then lua_write(1, 0x0051F270, r[3]) end
			end
			if shoot_mode == "2" and now >= shot_from then
				gun_x:set_value(233); gun_y:set_value(233)
				trigger:set_value(((now - shot_from) % 1.0) < 0.1 and 1 or 0)
			end
			if shoot_mode == "3" and t > coins * 0.5 + 2 then
				-- HOTD_NOSKIP=1 (proofs-11): the trigger stays released while a cut-scene is skippable (evt_skip_on
				-- 0x520944), so the scenes play to their end (the players' scene bodies' late steps)
				local ev = space:read_u8(0x0051F9D1) * 256 + space:read_u8(0x0051F9D0)
				if ev ~= hold_ev then
					hold_ev, hold_until = ev, 0
					for _, h in ipairs(holdfire) do
						if h[1] * 256 + h[2] == ev then hold_until = now + h[3] end
					end
					duty_on, duty_off, duty_t0 = 0, 0, now
					for _, d in ipairs(duty) do
						if d[1] * 256 + d[2] == ev then duty_on, duty_off = d[3], d[4] end
					end
				end
				local duty_hold = duty_off > 0 and ((now - duty_t0) % (duty_on + duty_off)) >= duty_on
				if now < hold_until or duty_hold or (noskip and space:read_u32(0x00520944) ~= 0) then trigger:set_value(0); aim_state.press = false
				else autoaim_step() end
			end
			if shoot and t > 2 then
				-- sweep the gun over the screen (MAME range 87-380 on both axes) and pull the trigger on and off
				gun_x:set_value(math.floor(233 + 140 * math.sin(now * 1.7)))
				gun_y:set_value(math.floor(233 + 140 * math.sin(now * 2.9)))
				shots = shots + 1
				trigger:set_value((shots % 6 < 2) and 1 or 0)
			end
		end
		if fifo_file and now >= fifo_at + fifo_for then
			fifo_file:close()   -- the log is buffered: close it as soon as the window is over
			fifo_file = nil
		end
		if now >= next_dump then
			local tag = string.format("t%05d", math.floor(now * 10 + 0.5))
			dump(tag .. "_palette.bin", 0x01800000, 0x01803fff)
			dump(tag .. "_colorxlat.bin", 0x01810000, 0x0181bfff)
			dump(tag .. "_luma.bin", 0x11400000, 0x1140ffff)
			dump(tag .. "_workram.bin", 0x00500000, 0x005fffff)
			local slot = texram_every > 0 and math.floor(now / texram_every + 1e-6) or -2
			if texram and (texram_every <= 0 or slot ~= last_tex_slot) then dump(tag .. "_texram.bin", 0x11000000, 0x113fffff); last_tex_slot = slot end
			if bufram then dump(tag .. "_bufram.bin", 0x00900000, 0x0097ffff) end
			machine.video:snapshot()
			index:write(string.format("%s time %.2f\n", tag, now))
			index:flush()
			next_dump = next_dump + every
		end
		if now >= total then
			index:close()
			machine:exit()
			return
		end
		local waited = emu.wait(step)
		if stopping or waited == false then return end
	end
end

local co = coroutine.create(main)
local ok, err = coroutine.resume(co)
if not ok then print("mame_dump.lua error: " .. tostring(err)) end
