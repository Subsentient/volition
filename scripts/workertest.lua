
function TestWorker(Handle)
	for Inc = 1, 10 do
		print('Derp' .. tostring(Inc))
		VL.vl_sleep(100)
	end

	local Stream = Handle:Recv(true)
	
	local _, Stringy = Stream:Pop()

	print('Ass farts: ' .. Stringy)
end

if VL_LUAJOBTYPE ~= VL.LUAJOB_WORKER then
	print('FArts')
	local Worker = VL.SpawnWorker('TestWorker')

	Worker:Start()
	local OutMsg = VL.ConationStream.New(VL.CMDCODE_INVALID, 0, 0)

	OutMsg:Push(VL.ARGTYPE_STRING, 'Poooooo farts!')
	Worker:Send(OutMsg)
	Worker:Join()
end