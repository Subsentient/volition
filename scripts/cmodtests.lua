--[[
VLSI_BEGIN_SPEC
scriptname::cmodtests
scriptversion::0.1
scriptdescription::Lua C/C++ native code module testing
function::SelfDlsym|Load and execute a demo C linkage function from our own binary|~ARGTYPE_STRING
VLSI_END_SPEC
]]

function SelfDlsym()
	assert(VL.RunningAsJob(), 'Not running as a job!')
	
	local Stream = VL.RecvStream()
	
	if not Stream then
		io.stderr:write('Stream missing!\n')
		return
	end
	
	local Response = Stream:BuildResponse()

	local JobArgs = Stream:PopJobArguments()
	
	print(JobArgs.Origin, JobArgs.Destination, JobArgs.ScriptName, JobArgs.FuncName)
	
	
	local vllexec = VL.GetCFunction(nil, 'SelfTestCFunc')

	local _, Stringy = Stream:Pop()
	
	if not Stringy then
		Stringy = 'Testing, no string provided!'
	end
	
	print('Lua got: ' .. vllexec(Stringy))
end
