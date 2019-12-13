--[[
VLSI_BEGIN_SPEC
scriptname::ziggurat
scriptversion::0.1
scriptdescription::Ziggurat Qt N2N based messenger
function::InitZiggurat|Start Ziggurat
VLSI_END_SPEC
]]

Ziggurat = { StayAlive = true }

--Command codes internal to and exclusively used by Ziggurat
ZIGCMD_PING = 1
ZIGCMD_GREET = 2
ZIGCMD_EXIT = 3
ZIGCMD_MSG = 4
ZIGCMD_SETNICK = 5

MsgIDCounter = 1 --Separate message IDs for every client's incoming messages.

Peers = {} --Full of ZigPeers

ZigPeer = { Message = {} }

function ZigPeer.New(Node)
	local Table =	{
						LastActivity = 0,
						ID = Node,
						DisplayName = Node, --Changeable by the user
						Messages = {}
					}
					
	setmetatable(Table, { __index = ZigPeer })
	return Table
end

function ZigPeer.Message.New(Origin, Destination, Body)
	local Instance =	{
							Origin = Origin,
							Destination = Destination,
							Body = Body,
							SendTime = os.time()
						}

	setmetatable(Instance, { __index = ZigPeer.Message })
	return Instance
end

function ZigPeer.Message:IsLinkMsg()
	return string.find(self.Body, '^http[s]?://') ~= nil
end

function ZigPeer.Message:IsImageMsg()
	if not self:IsLinkMsg() then
		return false
	end
	
	local Patterns = { '\\.jpg[?]?.*$', '\\.png[?]?.*$', '\\.jpeg[?]?.*$' }
	
	local Match = false
	
	for Index, Pattern in ipairs(Patterns) do
		if string.find(self.Body, Pattern) ~= nil then
			Match = true
			break
		end
	end
	
	return Match
end

function ZigPeer:RenderMessage(Msg)
	if Msg:IsImageMsg() then
		local Blob = VL.GetHTTP(Msg.Body, 1, nil, nil, true); --Get a blob of binary data back.
		
		if not Blob then
			ZigWarn('Unable to download file at "' .. Msg.Body .. '".')
		end

		Ziggurat:RenderImageMessage(self.DisplayName, Blob)
	elseif Msg:IsLinkMsg() then
		Ziggurat:RenderLinkMessage(self.DisplayName, Msg.Body)
	else
		Ziggurat:RenderTextMessage(self.DisplayName, Msg.Body)
	end
end

function Ziggurat:NewEmptyStream(Destination, Subcommand, IsReport)
	local Flags = VL.IDENT_ISN2N_BIT
	
	if IsReport then
		Flags = Flags | VL.IDENT_ISREPORT_BIT
	end
	
	local Msg = VL.ConationStream.New(VL.CMDCODE_N2N_GENERIC, Flags, 0)
	
	Msg:Push(VL.ARGTYPE_ODHEADER, VL.GetIdentity())
	--We don't care about Job IDs here.
	Msg:Push(VL.ARGTYPE_UINT64, 0)
	Msg:Push(VL.ARGTYPE_UINT64, 0)
	
	Msg:Push(VL.ARGTYPE_UINT32, MsgIDCounter)

	MsgIDCounter = MsgIDCounter + 1 --Increment counter
	
	Msg:Push(VL.ARGTYPE_UINT16, Subcommand)
	
	return Msg
end

function ZigPeer:SendPing()
	local Msg = Ziggurat:NewEmptyStream(self.ID, ZIGCMD_PING, false)
	
	VL.SendN2N(Msg)
end

function ZigPeer:RegisterActivity()
	self.LastActivity = os.time()
end

function ZigPeer:AddMessage(Msg)
	self.Messages[#self.Messages + 1] = Msg
end

function ZigPeer:SendMsg(Body)
	local Msg = self.Message.New(VL.GetIdentity(), self.ID, Body)
	
	self.Us:AddMessage(Msg)
	self.Us:RenderMessage(Msg)

	local OutMsg = Ziggurat:NewEmptyStream(Msg.Destination, ZIGCMD_MSG, false)

	OutMsg:Push(VL.ARGTYPE_STRING, Msg.Body)
	
	VL.SendN2N(OutMsg)
end


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
		return false
	end
	
	local FilenameStart
	
	_, FilenameStart = string.find(Buffer, ZigLibStartDelim, 1, true)
	
	if not FilenameStart then
		ZigDebug('Error loading tumor, filename delimiter ' .. ZigLibStartDelim .. ' was not detected!')
		return false
	end
	
	FilenameStart = FilenameStart + 1
	
	local FilenameEnd, DataBegin = string.find(Buffer, ZigLibEndDelim, 1, true)
	
	if not FilenameEnd then
		ZigDebug('No filename ending delimiter ' .. ZigLibEndDelim .. ' detected! Exiting.')
		return false
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
		return false
	end

	ZigDebug('Finding Ziggurat shared library entry point in library at "' .. NewPath .. '"... ')

	local InitLibZiggurat = VL.GetCFunction(NewPath, ZigLibEntryPointName)
	
	if not InitLibZiggurat then
		ZigError('Unable to find ZigLib entry point "' .. ZigLibEntryPointName .. '" in library at path "' .. NewPath .. '"!')
		return false
	end
	
	local ZigLib = InitLibZiggurat()
	
	if type(ZigLib) ~= 'boolean' or not ZigLib then
		ZigError('Return code from ' .. ZigLibEntryPointName .. ' is failure!')
		return false
	end
	
	return true
end

function Ziggurat:OnMessageToSend(Node, MsgBody)
	local Peer = Peers[Node]

	if not Peer then
		ZigWarn('No such node ' .. Node .. ' is active to send message to.')
	end
	
	Peer:SendMsg(MsgBody)
end

function InitZiggurat() --Must be executable as both init script and a job.
	if not LoadZigSharedLibrary() or type(Ziggurat) ~= 'table' then
		ZigError('Failed to load Ziggurat shared library!')
		return false
	end
	
	ZigDebug('Library load successful!')
	
	VL.SetN2NState(true) --Enable node-to-node communications for our job.
	
	ZigPeer.Us = ZigPeer.New(VL.GetIdentity())
	Peers[VL.GetIdentity()] = ZigPeer.Us
	
	local DemoNode = 'demonode'
	
	Ziggurat:AddNode(DemoNode)
	Ziggurat:RenderTextMessage(DemoNode, '<font color="#0000cd">' .. DemoNode .. ':</font> ' .. 'Ass Nipples')
	Ziggurat:RenderTextMessage(DemoNode, '<font color="#0000cd">' .. DemoNode .. ':</font> ' .. 'Fart waffles')
	Ziggurat:RenderLinkMessage(DemoNode, 'https://universe2.us')
	
	while Ziggurat.StayAlive do
		Ziggurat:MainLoopIter()
	end
	
	return true
end

function Ziggurat:MainLoopIter()
	Ziggurat:ProcessQtEvents()
	VL.vl_sleep(20)
end

if not VL.RunningAsJob() then
	InitZiggurat()
end
