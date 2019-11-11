
--[[
VLSI_BEGIN_SPEC
scriptname::scripting_selftest
scriptversion::0.1
scriptdescription::Self-test routine for scripting core and API
function::execselftest|Execute scripting self-test|ARGTYPE_STRING,ARGTYPE_STRING
VLSI_END_SPEC
]]

function execselftest()

	
	local Stream = VL.RecvStream()
	
	if not Stream then
		print("Stream missing")
	end
	
	local Header = Stream:GetHeader()
	local Incy = 0
	
	for Values in Stream:Iter() do
		Incy = Incy + 1
		print('new value at index ' .. Incy)
		print(table.unpack(Values))
	end
	Stream:Rewind()
	
	local _, Origin = Stream:Pop()

	-- We don't care about the script name and function name, we're already in it.
	Stream:Pop() --Script name
	Stream:Pop() --Function name
	
	local StringToReverse, WebPath
	_, StringToReverse = Stream:Pop()
	_, WebPath = Stream:Pop()

	
	print "Creating response"
	local Response = Stream:BuildResponse()

	Response:Push(VL.ARGTYPE_STRING, "Script initialized and executed successfully.")

	local NewHdr = Response:GetHeader()
	
	print('Got header')
	for Key, Value in pairs(Header) do
		print(Key .. '::' .. Value)
	end
	for Key, Value in pairs(NewHdr) do
		print(Key .. '::' .. Value)
	end
	
	if not Response then
		print("Response stream creation failed")
	end

	print "Populating response"

	local IdentityString = "Node identity is " .. VL.GetIdentity() .. "\nRevision " .. VL.GetRevision()
							.. "\nPlatform string " .. VL.GetPlatformString() .. "\nServer address " .. VL.GetServerAddr()

	
	Response:Push(VL.ARGTYPE_STRING, IdentityString)

	Response:Push(VL.ARGTYPE_STRING, string.reverse(StringToReverse))

	print('DirListing time')
	local DirListing = VL.ListDirectory(".")
	
	if not DirListing then
		print "Failed to run DirListing"
	end
	
	local DirString = "Directory listing is as follows:\n"
	
	for i=1,#DirListing
	do
		if DirListing[i][2] then
			DirString = DirString .. "d "
		else
			DirString = DirString .. "f "
		end

		DirString = DirString .. DirListing[i][1] .. '\n'
	end
	
	print('Pushing dirlist string')
	Response:Push(VL.ARGTYPE_STRING, DirString)
	
	local FilePath = VL.GetHTTP(WebPath);
	local FileData
	
	if not FilePath then
		FileData = "Failed to fetch file" .. WebPath
	else
		local Desc = io.open(FilePath, 'r')
		FileData = Desc:read("*all")
		Desc:close()
	end
	
	
	Response:Push(VL.ARGTYPE_STRING, "Temporary file path for web fetched file is " .. FilePath)
	Response:Push(VL.ARGTYPE_STRING, FileData)
	VL.SendStream(Response)
	
end
print "Self-test diagnostics script loaded successfully."
