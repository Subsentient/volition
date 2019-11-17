--[[
VLSI_BEGIN_SPEC
scriptname::ziggurat
scriptversion::0.1
scriptdescription::Ziggurat Qt N2N based messenger
function::InitZiggurat|Start Ziggurat
VLSI_END_SPEC
]]


function ZigDebug(String)
	io.stdout:write('Ziggurat messenger: DEBUG: ' .. String .. '\n')
end

function ZigError(String)
	io.stdout:write('Ziggurat messenger: !!ERROR!!: ' .. String .. '\n')
end

function ZigWarn(String)
	io.stdout:write('Ziggurat messenger: ~WARNING~: ' .. String .. '\n')
end

function LoadZigSharedLibrary() --Stripped down, specialized tumor loading code.
	local ZigLibStartDelim = '\x56LLTUMOR_FN_DELIM' --x56 is a capital V
	local ZigLibEndDelim = '\x56LLTUMOR_FN_EDELIM'
	local ZigLibEntryPointName = 'InitLibZiggurat'
	
	ZigDebug 'Attempting to load Ziggurat shared library as a tumor'
	
	local BinaryPath = VL.GetSelfBinaryPath()
	
	ZigDebug('Self binary located at ' .. BinaryPath)
	
	ZigDebug 'Loading self binary into RAM... '
	
	local Buffer = VL.SlurpFile(BinaryPath)
	
	if not Buffer then
		ZigError('Unable to slurp self binary at ' .. BinaryPath .. ' into RAM!')
		return nil
	end
	
	local FilenameStart
	
	_, FilenameStart = string.find(Buffer, ZigLibStartDelim, 1, true)
	
	if not FilenameStart then
		ZigDebug('Error loading tumor, filename delimiter ' .. ZigLibStartDelim .. ' was not detected!')
		return nil
	end
	
	FilenameStart = FilenameStart + 1
	
	local FilenameEnd, DataBegin = string.find(Buffer, ZigLibEndDelim, 1, true)
	
	if not FilenameEnd then
		ZigDebug('No filename ending delimiter ' .. ZigLibEndDelim .. ' detected! Exiting.')
		return nil
	end
	
	FilenameEnd = FilenameEnd - 1
	DataBegin = DataBegin + 1
		
	local Filename = string.sub(Buffer, FilenameStart, FilenameEnd)	
	local Data = string.sub(Buffer, DataBegin)
	local NewPath = VL.GetTempDirectory() .. VL.ospathdiv .. Filename
	
	ZigDebug('Extracting ZigLib tumor to path "' .. NewPath .. '"... ')
	
	local WriteWorked = VL.WriteFile(NewPath, Data)
	
	if not WriteWorked then
		ZigDebug('Failed to write binary back to temporary path ' .. NewPath .. ' for loading! Exiting.')
		return nil
	end

	ZigDebug('Finding Ziggurat shared library entry point in library at "' .. NewPath .. '"... ')

	local InitLibZiggurat = VL.GetCFunction(NewPath, ZigLibEntryPointName)
	
	if not InitLibZiggurat then
		ZigError('Unable to find ZigLib entry point "' .. ZigLibEntryPointName .. '" in library at path "' .. NewPath .. '"!')
		return nil
	end
	
	local ZigLib = InitLibZiggurat()
	
	if type(ZigLib) ~= 'table' then
		ZigError('Return code from ' .. ZigLibEntryPointName .. ' is failure!')
		return nil
	end
	
	return ZigLib
end

function InitZiggurat() --Must be executable as both init script and a job.
	Ziggurat = LoadZigSharedLibrary() --Deliberate global assignment
	
	if type(Ziggurat) ~= 'table' then
		ZigError('Failed to load Ziggurat shared library!')
		return false
	end
	
	ZigDebug('Library load successful!')
	
	while true do
		MainLoopIter()
	end
	
	return true
end

function MainLoopIter()
	Ziggurat:ProcessQtEvents()
	VL.vl_sleep(20)
end

if not VL.RunningAsJob() then
	InitZiggurat()
end
