--[[
VLSI_BEGIN_SPEC
scriptname::cmodtests
scriptversion::0.1
scriptdescription::Lua C/C++ native code module testing
function::RunPuts|Run puts() with a shitty invalid pointer
VLSI_END_SPEC
]]

function RunPuts()
	
	local puts = VL.GetCFunction('/lib64/libc.so.6', 'puts')
	
	print(puts)
	
	puts('derp nipples')
end
