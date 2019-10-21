--[[
VLSI_BEGIN_SPEC
scriptname::loadtest
scriptversion::0.1
scriptdescription::More tests
function::execmore|Execute more tests that don't take arguments
function::bincopy|Copy a binary file|ARGTYPE_STRING,ARGTYPE_STRING
VLSI_END_SPEC
]]

function execmore()
	print('XM: Binary path is "' .. VL.GetSelfBinaryPath() .. '"')
	
	VL.WriteFile('/dh/Desktop/assnipples.txt', VL.SlurpFile('/etc/rc.local'))
end

function bincopy()
	local Stream = VL.RecvStream()
	
	if not Stream then
		print('No ConationStream instance, exiting')
		return false
	end
	Stream:Pop()
	Stream:Pop()
	Stream:Pop()
	
	local Source, Destination
	_, Source = Stream:Pop() 
	_, Destination = Stream:Pop() 
	print('Copying ' .. Source .. ' to ' .. Destination)
	
	local Success = VL.WriteFile(Destination, VL.SlurpFile(Source))
	
	
	print('Copy was ' .. (Success and 'successful' or 'unsuccessful'))
end
