--[[
VLSI_BEGIN_SPEC
scriptname::n2ntest
scriptversion::0.1
scriptdescription::Node-to-node communications tester
function::SendN2N|Send a message to a node|ARGTYPE_STRING,ARGTYPE_STRING
function::AwaitN2N|Await a node-to-node message and respond with it
VLSI_END_SPEC
]]
function SendN2N()
	assert(VL.RunningAsJob(), 'Not running as a job!')
	
	local Stream = VL.RecvStream()
	
	if not Stream then
		io.stderr:write('Stream missing!\n')
		return
	end
	
	local Response = Stream:BuildResponse()

	local JobArgs = Stream:PopJobArguments()
	
	print(JobArgs.Origin, JobArgs.Destination, JobArgs.ScriptName, JobArgs.FuncName)

	local _, Destination = Stream:Pop()
	local _, Data = Stream:Pop()
	
	local OutMsg = VL.ConationStream.New(VL.CMDCODE_N2N_GENERIC, VL.IDENT_ISN2N_BIT, 0)
	
	OutMsg:Push(VL.ARGTYPE_ODHEADER, VL.GetIdentity(), Destination)
	OutMsg:Push(VL.ARGTYPE_STRING, Data)
	
	VL.SendN2N(OutMsg)
	
	Response:Push(VL.ARGTYPE_STRING, 'It seems to have sent okay')
	VL.SendStream(Response)
end

function AwaitN2N()
	assert(VL.RunningAsJob(), 'Not running as a job!')

	VL.SetN2NState(true)
	
	local JobResponse = VL.RecvStream():BuildResponse()
	
	local N2NStream = VL.WaitN2N()
	
	if not N2NStream then
		io.stderr:write('Somehow a blocking call exited with a null value!')
		return
	end
	
	N2NStream:Pop() --ODHeader
	
	Type, Data = N2NStream:Pop()
	
	if Type ~= VL.ARGTYPE_STRING then
		JobResponse:Push(VL.ARGTYPE_STRING, 'Failure decoding N2N stream string')
		VL.SendStream(JobResponse)
		return
	end
	
	JobResponse:Push(VL.ARGTYPE_STRING, 'Got N2N string ' .. Data)
	VL.SendStream(JobResponse)
end
	
