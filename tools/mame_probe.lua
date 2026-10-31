-- mame_probe.lua: list the input ports/fields of the running machine, then exit.
-- mame hotd -autoboot_script mame_probe.lua
local done = false
emu.add_machine_frame_notifier(function()
	if done then return end
	done = true
	for tag, port in pairs(manager.machine.ioport.ports) do
		for name, field in pairs(port.fields) do
			print(string.format("PORT %s FIELD %s mask=%x type=%s", tag, name, field.mask, tostring(field.type)))
		end
	end
	manager.machine:exit()
end)
