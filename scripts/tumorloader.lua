--[[
VLSI_BEGIN_SPEC
scriptname::tumorloader
scriptversion::0.1
scriptdescription::Tumor loader and reader
function::LoadTumor|Load and execute the tumor
VLSI_END_SPEC
]]

--Escape the capital V because we need the values to be unique, and they'll show up if we're an init script otherwise.
FNStartDelim = '\x56LLTUMOR_FN_DELIM' --x56 is a capital V
FNEndDelim = '\x56LLTUMOR_FN_EDELIM'


function LoadTumor()
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
	
	_, FilenameStart = string.find(Buffer, FNStartDelim)
	
	if not FilenameStart then
		print('Error loading tumor, filename delimiter ' .. FNStartDelim .. ' was not detected!')
		return
	end
	
	FilenameStart = FilenameStart + 1
	
	local FilenameEnd, DataBegin = string.find(Buffer, FNEndDelim)
	
	FilenameEnd = FilenameEnd - 1
	DataBegin = DataBegin + 1
	
	if not FilenameEnd then
		print('No filename ending delimiter ' .. FNEndDelim .. ' detected! Exiting.')
		return
	end
		
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
		local ModChunk = loadfile(NewPath)
		
		print(ModChunk and 'Success' or 'FAILURE')
		ModChunk()
	elseif Extension == '.so' or Extension == '.dll' or Extension == '.dylib' then
		io.write('Loading native code tumor at "' .. NewPath .. '"... ')
		
		
		vlltumorexec = VL.GetCFunction(NewPath, 'vlltumorexec')
		
		print(vlltumorexec and 'Success' or 'FAILURE')
	end
	
	if not vlltumorexec then
		print('TUMOR LOAD FAILED, exiting')
		return
	end
	
	print('Executing vlltumorexec() in tumor at ' .. NewPath)
	local Values = vlltumorexec()
	
	print('Tumor entry point has returned. Got values ' .. tostring(Values))
	print('TUMORLOADER: Returning after success')
end

--If we're not a job, we're an init script. So just execute it.
if not VL.RunningAsJob() then
	LoadTumor()
end

