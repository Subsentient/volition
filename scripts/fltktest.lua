
local Platform = VL.GetPlatformString()
local OSDividerOffset = string.find(Platform, '.', 1, true)

OS = string.sub(Platform, 1, OSDividerOffset - 1)
CPU = string.sub(Platform, OSDividerOffset + 1, -1)

print('FLTK debug script running on OS ' .. OS .. ' and CPU ' .. CPU)

if OS ~= 'windows' and not VL.getenv('DISPLAY') then
	if not VL.setenv('DISPLAY', ':0') then
		print('Failed to set ENV for DISPLAY!')
		return	
	end
	
	print('Got DISPLAY ' .. VL.getenv('DISPLAY'))
end

fltk = VL.NODE_COMPILETIME_EXTFUNC()

local KeepLooping = true
local Win = fltk.Window(300, 100)
local Button = fltk.Button(100, 20, 128, 32, 'Click to exit.')

function Button:callback()
	KeepLooping = false
end

Win:show()
while KeepLooping do
	fltk.wait(0.1)
end

Win:hide()
fltk.wait()

