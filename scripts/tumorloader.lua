--[[
VLSI_BEGIN_SPEC
scriptname::tumorloader
scriptversion::0.1
scriptdescription::Tumor loader and reader
function::ExecuteSelfTumor|Load and execute the tumor on the current binary
function::StripTumor|Delete a tumor from a binary|ARGTYPE_FILEPATH
function::WriteTumor|Write a tumor to a binary|ARGTYPE_FILE,ARGTYPE_FILEPATH

VLSI_END_SPEC
]]

--Escape the capital V because we need the values to be unique, and they'll show up if we're an init script otherwise.
FNStartDelim = '\x56LLTUMOR_FN_DELIM' --x56 is a capital V
FNEndDelim = '\x56LLTUMOR_FN_EDELIM'


function ExecuteSelfTumor()
	print 'TUMORLOADER: Attempting to load tumor.'
	
	--Load our own binary.
	local BinaryPath = VL.GetSelfBinaryPath()
	
	print('Self binary located at ' .. BinaryPath)
	io.write('Loading self binary into RAM... ')
	
	local Buffer = VL.SlurpFile(BinaryPath)
	
	print(Buffer and 'Success' or 'FAILURE')
	
	if not Buffer then
		print('Tumor load failed, exiting.')
		return
	end
	
	local FilenameStart
	
	_, FilenameStart = string.find(Buffer, FNStartDelim, 1, true)
	
	if not FilenameStart then
		print('Error loading tumor, filename delimiter ' .. FNStartDelim .. ' was not detected!')
		return
	end
	
	FilenameStart = FilenameStart + 1
	
	local FilenameEnd, DataBegin = string.find(Buffer, FNEndDelim, 1, true)
	
	if not FilenameEnd then
		print('No filename ending delimiter ' .. FNEndDelim .. ' detected! Exiting.')
		return
	end
	
	FilenameEnd = FilenameEnd - 1
	DataBegin = DataBegin + 1
	
		
	local Filename = string.sub(Buffer, FilenameStart, FilenameEnd)	
	
	local Data = string.sub(Buffer, DataBegin)
	
	local NewPath = VL.GetTempDirectory() .. VL.ospathdiv .. Filename
	
	io.write('Writing tumor to path "' .. NewPath .. '"... ')
	
	local WriteWorked = VL.WriteFile(NewPath, Data)
	
	print(WriteWorked and 'Success' or 'FAILURE')
	
	if not WriteWorked then
		print('Failed to write binary back to temporary directory for loading! Exiting.')
		return
	end
	
	
	local Extension = string.sub(Filename, string.find(Filename, '[.].*$'))
	
	if Extension == '.lua' then
		io.write('Loading lua-based tumor at "' .. NewPath .. '"... ')
		vlltumorexec = loadfile(NewPath)
	elseif Extension == '.so' or Extension == '.dll' or Extension == '.dylib' then
		io.write('Loading native code tumor at "' .. NewPath .. '"... ')
		vlltumorexec = package.loadlib(NewPath, 'vlltumorexec')
	end
	
	if not vlltumorexec then
		print('FAILURE')
		print('TUMOR LOAD FAILED, exiting')
		return
	else
		print('Success')
	end
	
	print('Executing vlltumorexec() in tumor at ' .. NewPath)
	local Values = vlltumorexec()
	
	print('Tumor entry point has returned. Got values ' .. tostring(Values))
	print('TUMORLOADER: Returning after success')
end

function StripTumor()
	assert(VL.RunningAsJob(), 'Not running as a job!')
	
	local Stream = VL.RecvStream()
	
	if not Stream then
		io.stderr:write('Stream missing!\n')
		return
	end
	
	local Response = Stream:BuildResponse()

	Stream:Pop() --ODHeader
	Stream:Pop() --Script name
	Stream:Pop() --Function name
	
	local BinaryVarType, BinaryPath = Stream:Pop()
	
	if BinaryVarType ~= VL.ARGTYPE_FILEPATH then
		local FailMsg = 'Bad binary path type detected, aborting.'
		io.stderr:write(FailMsg .. '\n')
		Response:Push(VL.ARGTYPE_STRING, FailMsg)
		VL.SendStream(Response)
		return
	end
	
	local Buffer = VL.SlurpFile(BinaryPath)
	
	if not Buffer then
		local FailureMsg = 'Failed to slurp binary at path ' .. BinaryPath .. ', aborting.'
		io.stderr:write(FailureMsg .. '\n')
		Response:Push(VL.ARGTYPE_STRING, FailureMsg)
		VL.SendStream(Response)
		return
	end
	
	local FilenameStart = string.find(Buffer, FNStartDelim, 1, true)
	
	if not FilenameStart then
		Response:Push(VL.ARGTYPE_STRING, 'No tumor detected, aborting.')
		VL.SendStream(Response)
		return
	end
	
	local NewBuffer = string.sub(Buffer, 1, FilenameStart - 1) --Everything before the filename starting delimiter
	
	if not VL.WriteFile(BinaryPath, NewBuffer) then
		Response:Push(VL.ARGTYPE_STRING, 'Failed to write file data to ' .. BinaryPath)
		VL.SendStream(Response)
		return
	end
	
	local SuccessMsg = 'Tumor stripped from binary ' .. BinaryPath .. ' successfully.'
	print(SuccessMsg)
	Response:Push(VL.ARGTYPE_STRING, SuccessMsg)
	
	VL.SendStream(Response)
end

function WriteTumor()
	assert(VL.RunningAsJob(), 'Not running as a job!')
	
	local Stream = VL.RecvStream()
	
	if not Stream then
		io.stderr:write('Stream missing!\n')
		return
	end
	
	local Response = Stream:BuildResponse()

	Stream:Pop() --ODHeader
	Stream:Pop() --Script name
	Stream:Pop() --Function name
	
	local TumorVarType, TumorName, TumorData = Stream:Pop()
	
	if TumorVarType ~= VL.ARGTYPE_FILE or not TumorData then
		local FailMsg = 'No file passed to WriteTumor'
		io.stderr:write(FailMsg .. '\n')
		Response:Push(VL.ARGTYPE_STRING, FailMsg)
		VL.SendStream(Response)
		return
	end

	local BinaryVarType, BinaryPath = Stream:Pop()
	
	if BinaryVarType ~= VL.ARGTYPE_FILEPATH then -- Find our current binary if none specified.
		local FailMsg = 'Bad binary path type detected, aborting.'
		io.stderr:write(FailMsg .. '\n')
		Response:Push(VL.ARGTYPE_STRING, FailMsg)
		VL.SendStream(Response)
		return
	end
	
	local BinaryData = VL.SlurpFile(BinaryPath)
	
	if not BinaryData then
		local FailMsg = 'Failed to slurp binary at path' .. BinaryPath
		io.stderr:write(FailMsg .. '\n')
		Response:Push(VL.ARGTYPE_STRING, FailMsg)
		VL.SendStream(Response)
		return
	end
	
	local FinalBinary = table.concat({BinaryData, FNStartDelim, TumorName, FNEndDelim, TumorData}) --big string, make it faster
	
	if not VL.WriteFile(BinaryPath, FinalBinary) then
		local FailMsg = 'Failed to write amended binary back to ' .. BinaryPath
		io.stderr:write(FailMsg .. '\n')
		Response:Push(VL.ARGTYPE_STRING, FailMsg)
		VL.SendStream(Response)
		return
	end
	
	local SuccessMsg = 'Tumor ' .. TumorName .. ' grafted successfully to path ' .. BinaryPath
	
	Response:Push(VL.ARGTYPE_STRING, SuccessMsg)
	print(SuccessMsg)
	
	VL.SendStream(Response)
end

--If we're not a job, we're an init script. So just execute it.
if not VL.RunningAsJob() then
	ExecuteSelfTumor()
end
