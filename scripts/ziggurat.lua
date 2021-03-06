--[[
VLSI_BEGIN_SPEC
scriptname::ziggurat
scriptversion::0.1
scriptdescription::Ziggurat Qt N2N based messenger
function::InitZiggurat|Start Ziggurat
VLSI_END_SPEC
]]

function ZigDebug(String)
	local Dbg = debug.getinfo(2, 'lf')
	io.stdout:write('Ziggurat messenger: DEBUG: Line ' .. Dbg.currentline .. ': '.. String .. '\n')
end

function ZigError(String)
	local Dbg = debug.getinfo(2, 'lf')
	io.stdout:write('Ziggurat messenger: !!ERROR!!: Line ' .. Dbg.currentline .. ': '.. String .. '\n')
end

function ZigWarn(String)
	local Dbg = debug.getinfo(2, 'lf')
	io.stdout:write('Ziggurat messenger: ~WARNING~: Line ' .. Dbg.currentline .. ': '.. String .. '\n')
end

Ziggurat = { StayAlive = true }

--Command codes internal to and exclusively used by Ziggurat
ZIGCMD_PING = 1
ZIGCMD_GREET = 2
ZIGCMD_UNGREET = 3
ZIGCMD_MSG = 4
ZIGCMD_SETNICK = 5

MsgIDCounter = 0 --Separate message IDs for every client's incoming messages.

Peers = {} --Full of ZigPeers

ZigPeer = { Message = {} }

ConfigManager = { Keys = {} }

ZigLibEntryPointName = 'InitLibZiggurat'

ZigConfigFilePath = VL.GetHomeDirectory() .. VL.ospathdiv .. '.zigguratconfig.cnb'
PingoutSecs = 10
PingIntervalSecs = 3

function ConfigManager:SetKey(Key, Value)
	self.Keys[tostring(Key)] = tostring(Value)
end

function ConfigManager:UnsetKey(Key)
	self.Keys[tostring(Key)] = nil
end

function ConfigManager:GetKey(Key)
	return self.Keys[tostring(Key)]
end

function ConfigManager:SaveKeys()
	local Stream = VL.ConationStream.New(VL.CMDCODE_N2N_GENERIC, 0, 0)

	if not Stream then
		ZigWarn('Error constructing empty ConationStream!')
		return false
	end

	for Key, Value in pairs(self.Keys) do
		Stream:Push(VL.ARGTYPE_STRING, tostring(Key))
		Stream:Push(VL.ARGTYPE_STRING, tostring(Value))
	end

	local Result = VL.WriteFile(ZigConfigFilePath, Stream:GetData())

	if not Result then
		ZigWarn('Failed to write config file "' .. ZigConfigFilePath .. '" back to disk!')
		return false
	end

	return true
end

function ConfigManager:LoadKeys()
	local BinBlob = VL.SlurpFile(ZigConfigFilePath)

	if not BinBlob then
		ZigWarn('Unable to slurp config file at path "' .. ZigConfigFilePath .. '"!')
		return false
	end

	local Stream = VL.ConationStream.New(BinBlob)

	if not Stream then
		ZigError('Error decoding config file as ConationStream!')
		return false
	end

	if #Stream % 2 ~= 0 then
		ZigError('Uneven number of arguments, unsuitable for key/value pair!')
		return false
	end
	
	for Inc = 1, #Stream // 2 do
		local _, Key = Stream:Pop()
		local _, Value = Stream:Pop()

		self:SetKey(Key, Value)
	end
end

function ZigPeer.New(Node, ForeignJobID)
	local Table =	{
						LastActivity = os.time(),
						LastOutgoingPing = os.time(),
						ID = Node,
						DisplayName = Node, --Changeable by the user
						Messages = {},
						ForeignJobID = ForeignJobID,
					}
					
	setmetatable(Table, { __index = ZigPeer })
	return Table
end

function ZigPeer.Message.New(Origin, Destination, Body, SentMessageID)
	local Instance =	{
							Origin = Origin,
							Destination = Destination,
							Body = Body,
							SendTime = os.time(),
							SentMessageID = SentMessageID,
							DeliverTime = nil,
							Delivered = false,
							RenderedData = nil,
						}

	setmetatable(Instance, { __index = ZigPeer.Message, __gc = Ziggurat.DeleteNativeMsg })
	return Instance
end

function ZigPeer.Message:IsLinkMsg()
	return string.find(self.Body, '^http[s]?://') ~= nil
end

function ZigPeer.Message:MarkDelivered()
	self.DeliverTime = os.time()
	self.Delivered = true
end

function ZigPeer.Message:IsImageMsg()
	if not self:IsLinkMsg() then
		return false
	end
	
	local Patterns = { '[.]jpg[?]?.*$', '[.]png[?]?.*$', '[.]jpeg[?]?.*$' }
	
	local Match = false
	
	for Index, Pattern in ipairs(Patterns) do
		if string.find(self.Body, Pattern) ~= nil then
			ZigDebug('FOUND IMAGE MESSAGE ' .. self.Body)
			Match = true
			break
		end
	end
	
	return Match
end


function ZigPeer.Message:BuildMsgHeader(DisplayName, Color)
	return '<font color="#AAFFAA">[' .. os.date('%a %Y-%m-%d %I:%M:%S %p') .. '] </font><font color="' .. Color .. '">' .. DisplayName .. ':</font> '
end

function ZigPeer:RenderMessage(Msg)

	local MessengerNode = self.ID == VL.GetIdentity() and Msg.Destination or self.ID
	
	local Color
	
	if self == ZigPeer.Us then
		Color = '#00cd00'
	else
		Color = '#00cdcd'
	end
	
	if Msg:IsImageMsg() then
		local Blob = VL.GetHTTP(Msg.Body, 1, nil, nil, true) --Get a blob of binary data back.
		
		if not Blob then
			ZigWarn('Unable to download file at "' .. Msg.Body .. '".')
			Ziggurat:RenderTextMessage(MessengerNode, Msg.SentMessageID, Msg:BuildMsgHeader(self.DisplayName, Color) .. '<font color="#cd0000">BROKEN IMAGE at URL ' .. Msg.Body .. '</font>')
			return
		end
		
		local ImageText = Msg:BuildMsgHeader(self.DisplayName, Color) .. '<br><i>Image at</i> <a href="' .. Msg.Body .. '">' .. Msg.Body .. '</a>'

		Ziggurat:RenderImageMessage(MessengerNode, Msg.SentMessageID, ImageText, Blob)
	elseif Msg:IsLinkMsg() then
		local LinkText = Msg:BuildMsgHeader(self.DisplayName, Color) .. '<br><a href="' .. Msg.Body .. '">' .. Msg.Body .. '</a>'
		
		Ziggurat:RenderLinkMessage(MessengerNode, Msg.SentMessageID, LinkText)
	else
		Ziggurat:RenderTextMessage(MessengerNode, Msg.SentMessageID, Msg:BuildMsgHeader(self.DisplayName, Color) .. '<br>' .. Msg.Body)
	end
end

function Ziggurat:NewEmptyStream(Destination, Subcommand, IsReport, MsgID)
	--[[
		Format:

		ODHeader, origin/destination (node ID, NOT nickname)
		Origin uint64 job IS
		Destination uint64 job ID
		uint32 Message ID counter, used for the LOCAL node to keep track, not foreign nodes!
		uint32 Subcommand code, used by just Ziggurat
	]]

	local Flags = VL.IDENT_ISN2N_BIT
	
	if IsReport then
		Flags = Flags | VL.IDENT_ISREPORT_BIT
	end
	
	local Msg = VL.ConationStream.New(VL.CMDCODE_N2N_GENERIC, Flags, 0)
	
	Msg:Push(VL.ARGTYPE_ODHEADER, VL.GetIdentity(), Destination)

	Msg:Push(VL.ARGTYPE_UINT64, VL.GetJobID()) --Our job ID
	
	local Peer = Peers[Destination]
	
	Msg:Push(VL.ARGTYPE_UINT64, Peer and Peer.ForeignJobID or 0) --Their job ID
	
	local CounterID
	
	--If we're a report, we reply with the same message ID the other guy sent.
	if IsReport or MsgID then
		CounterID = MsgID
	else
		CounterID = MsgIDCounter
		MsgIDCounter = MsgIDCounter + 1
	end
	
	Msg:Push(VL.ARGTYPE_UINT32, CounterID) --Local message ID, not used by foreign nodes
	Msg:Push(VL.ARGTYPE_UINT32, Subcommand) --Sub-command code used by Ziggurat exclusively
	
	return Msg, CounterID
end

function ZigPeer:SendPing()
	local Msg = Ziggurat:NewEmptyStream(self.ID, ZIGCMD_PING, false)
	
	VL.SendN2N(Msg)
	
	self.LastOutgoingPing = os.time()
end

function ZigPeer:RegisterActivity()
	self.LastActivity = os.time()
end

function ZigPeer:AddMessage(Msg)
	self.Messages[Msg.SentMessageID] = Msg
end

function ZigPeer:SendMsg(Body)

	local OutStream, MsgID = Ziggurat:NewEmptyStream(self.ID, ZIGCMD_MSG, false)

	local Msg = self.Message.New(VL.GetIdentity(), self.ID, Body, MsgID)

	self.Us:AddMessage(Msg)
	self.Us:RenderMessage(Msg)

	OutStream:Push(VL.ARGTYPE_STRING, Msg.Body)
	
	VL.SendN2N(OutStream)
end


function libZiggurat_LoadViaAnyMethod()
	if	not libZiggurat_LoadViaStatic() and
		not libZiggurat_LoadViaTumor() and
		not libZiggurat_LoadViaSharedObject()
	then
		return false
	end
	
	return true
end

function libZiggurat_LoadViaStatic()
	if not VL.NODE_COMPILETIME_EXTFUNC then
		ZigDebug('Failed to find entry point ' .. ZigLibEntryPointName .. ' in our own executable via dlsym()')
		return false
	end
	
	return VL.NODE_COMPILETIME_EXTFUNC()
end

function libZiggurat_LoadViaSharedObject()
	local SearchDirs = { '.', '..', 'ziggurat' }
	
	local InitLibZiggurat = nil
	
	local PlatformMap = { ['windows'] = '.dll', ['linux'] = '.so', ['freebsd'] = '.so', ['openbsd'] = '.so', ['haiku'] = '.so', ['macosx'] = '.dylib' }
	
	local Ext = nil
	
	for Platform, Extension in pairs(PlatformMap) do
		if string.find(VL.GetPlatformString(), Platform, 1, true) then
			Ext = Extension
			break
		end
	end
	
	if not Ext then
		ZigDebug('Failing because of unknown platform')
		return false
	end
	
	for _, Dir in pairs(SearchDirs) do
		InitLibZiggurat = package.loadlib(Dir .. VL.ospathdiv .. 'libZiggurat' .. Ext, ZigLibEntryPointName)
		
		if InitLibZiggurat then
			return InitLibZiggurat()
		end
	end
	
	ZigDebug('Shared object load failed')
	return false
end

function libZiggurat_LoadViaTumor() --Stripped down, specialized tumor loading code.
	local ZigLibStartDelim = '\x56LLTUMOR_FN_DELIM' --x56 is a capital V
	local ZigLibEndDelim = '\x56LLTUMOR_FN_EDELIM'
	
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
	if #MsgBody == 0 then
		return
	end
	
	local Peer = Peers[Node]

	if not Peer then
		ZigWarn('No such node ' .. Node .. ' is active to send message to.')
	end
	
	ZigDebug('Sending message "' .. MsgBody .. '" to node ' .. Node .. ', length ' .. #MsgBody)
	
	Peer:SendMsg(MsgBody)
	
	return true
end

function InitZiggurat() --Must be executable as both init script and a job.
	if not libZiggurat_LoadViaAnyMethod() or type(Ziggurat) ~= 'table' then
		ZigError('Failed to load libZiggurat!')
		return false
	end
	
	ZigDebug('Library load successful!')
	
	VL.SetN2NState(true) --Enable node-to-node communications for our job.
	
	ZigPeer.Us = ZigPeer.New(VL.GetIdentity())
	Peers[VL.GetIdentity()] = ZigPeer.Us

	ConfigManager:LoadKeys()

	local FontString = ConfigManager:GetKey('GlobalFont')
	
	if FontString then
		ZigDebug('SETTING FONT STRING ' .. FontString)
		Ziggurat:LoadFont(FontString)
	end
	
	while Ziggurat.StayAlive do
		Ziggurat:MainLoopIter()
	end
	
	return true
end

function Ziggurat:ProcessPing(SetupArgs, Stream)
	local Peer = Peers[SetupArgs.Origin]
	
	if not Peer then
		ZigWarn('Unknown origin node ' .. SetupArgs.Origin)
		return
	end
	
	Peer:RegisterActivity()
end

function Ziggurat:ProcessGreeting(SetupArgs, Stream)
	--Someone wants to talk to us
	local _
	local Peer = ZigPeer.New(SetupArgs.Origin, SetupArgs.OriginJob)
	
	_, Peer.DisplayName = Stream:Pop() --The name we actually show.
	
	Peers[Peer.ID] = Peer
	
	local Response = Ziggurat:NewEmptyStream(SetupArgs.Origin, SetupArgs.Subcommand, true, SetupArgs.MsgID)
	
	Response:Push(VL.ARGTYPE_STRING, ZigPeer.Us.DisplayName)
	
	VL.SendN2N(Response) --Let them know we've accepted their greeting.
	
	--Open a tab to them now.
	self:AddNode(Peer.ID)
	
	ZigDebug('Added new node ' .. Peer.ID .. ' with display name "' .. Peer.DisplayName .. '".')
end

function Ziggurat:ProcessGreetingReport(SetupArgs, Stream)
	--Peer agreed to our request to talk
	local _
	local Peer = ZigPeer.New(SetupArgs.Origin, SetupArgs.OriginJob)
	
	_, Peer.DisplayName = Stream:Pop()
	
	if Peers[Peer.ID] then
		self:OnRemoteSessionTerminated(SetupArgs.Origin)
	end
	
	Peers[Peer.ID] = Peer
	
	--Open a tab now that we know they're willing to talk to us.
	self:AddNode(Peer.ID)
	
	ZigDebug('Our greeting to ' .. SetupArgs.Origin .. ' was accepted, session now open.')
end

function Ziggurat:ProcessUngreet(SetupArgs, Stream)
	local _
	local Peer = Peers[SetupArgs.Origin]
	
	if not Peer then
		ZigWarn('No peer ' .. SetupArgs.Origin .. ' to ungreet!')
	end
	
	self:OnRemoteSessionTerminated(SetupArgs.Origin)
	
	Peers[SetupArgs.Origin] = nil
	
	ZigDebug('Ungreeted node ' .. SetupArgs.Origin .. ', removed from database')
end

function Ziggurat:ProcessMsg(SetupArgs, Stream)
	local Peer = Peers[SetupArgs.Origin]
	
	if not Peer then
		ZigWarn('Received bogus message from non-peer node ' .. SetupArgs.Origin .. ', discarding')
		return
	end
	
	local MsgBody, MsgID, _
	
	_, MsgBody = Stream:Pop()
	 
	local Msg = ZigPeer.Message.New(SetupArgs.Origin, SetupArgs.Destination, MsgBody, SetupArgs.MsgID)
	
	Msg:MarkDelivered() --Because we just received it
	
	Peer:AddMessage(Msg)
	Peer:RenderMessage(Msg)
	
	if not self:GetHasFocus() then
		self:FireAudioNotification()
	end
	
	local Response = self:NewEmptyStream(SetupArgs.Origin, SetupArgs.Subcommand, true, Msg.SentMessageID)
	
	VL.SendN2N(Response)
end

function Ziggurat:ProcessMsgReport(SetupArgs, Stream)
	local Peer = Peers[SetupArgs.Origin]
	
	if not Peer then
		ZigWarn('Received message report from non-peer node ' .. SetupArgs.Origin .. ', discarding')
		return
	end
	
	local Msg = ZigPeer.Us.Messages[SetupArgs.MsgID]
	
	if not Msg then
		ZigWarn('Received message report from peer ' .. Peer.ID .. ', but we never sent a message with ID ' .. tostring(SetupArgs.MsgID) .. '!')
		return
	end

	Msg:MarkDelivered()

	ZigDebug('Successful delivery report from node ' .. Peer.ID .. ' for message ID ' .. tostring(SetupArgs.MsgID))
end

function Ziggurat:OnSessionEndRequested(NodeID)
	if not NodeID then
		ZigWarn('Called with no argument for NodeID')
		return
	end
	
	local EndMsg = Ziggurat:NewEmptyStream(NodeID, ZIGCMD_UNGREET, false)
	
	ZigDebug('Sending ungreet to node ' .. NodeID)
	
	VL.SendN2N(EndMsg)
	
	Peers[NodeID] = nil
	
	ZigDebug('Erased node from database successfully.')
end

function Ziggurat:OnNewNodeChosen(NodeID)
	--GUI click asked us to send a greeting to a node
	if not NodeID then
		ZigWarn('Called with no argument for NodeID')
		return
	end
	
	local GreetMsg = Ziggurat:NewEmptyStream(NodeID, ZIGCMD_GREET, false)
	
	GreetMsg:Push(VL.ARGTYPE_STRING, ZigPeer.Us.DisplayName)
	
	ZigDebug('Sending greeting to node ' .. NodeID)
	
	VL.SendN2N(GreetMsg)
end

function Ziggurat:OnNewFontSelected(FontString)
	ConfigManager:SetKey('GlobalFont', FontString)
	ConfigManager:SaveKeys()
end

function Ziggurat:ProcessSingleN2N(Stream)
	if not Stream then
		ZigWarn 'ProcessSingleN2N called with no argument'
		return
	end
	
	local StreamHeader = Stream:GetHeader()
	
	--See the NewEmptyStream() comment for the formatting.
	local RequiredArguments = 	{ --Arguments that absolutely every Ziggurat N2N will have
									VL.ARGTYPE_ODHEADER,
									VL.ARGTYPE_UINT64,
									VL.ARGTYPE_UINT64,
									VL.ARGTYPE_UINT32,
									VL.ARGTYPE_UINT32
								}
	
	if not Stream:VerifyArgTypesStartWith(table.unpack(RequiredArguments)) then
		ZigWarn 'Incorrect message format detected in ProcessSingleN2N. Discarding message.'
		local Msg = 'Got typecodes '
		
		for Inc, Code in ipairs(Stream:GetArgTypes()) do
			Msg = Msg .. Code .. ','
		end
		
		Msg = string.sub(Msg, 1, #Msg - 1)
		ZigWarn(Msg)
		return
	end
	
	local _
	local SetupArgs = {}

	_, SetupArgs.Origin, SetupArgs.Destination = Stream:Pop()	
	_, SetupArgs.OriginJob = Stream:Pop()
	_, SetupArgs.DestinationJob = Stream:Pop()
	_, SetupArgs.MsgID = Stream:Pop()
	_, SetupArgs.Subcommand = Stream:Pop()
		
	local SubcommandFunctions =	{
									[ZIGCMD_PING] = self.ProcessPing,
									[ZIGCMD_GREET] = self.ProcessGreeting,
									[ZIGCMD_UNGREET] = self.ProcessUngreet,
									[ZIGCMD_MSG] = self.ProcessMsg,
								}
	local AckFunctions = 		{
									[ZIGCMD_GREET] = self.ProcessGreetingReport,
									[ZIGCMD_MSG] = self.ProcessMsgReport,
								}
								
	local FuncTable
	
	if (StreamHeader.Flags & VL.IDENT_ISREPORT_BIT) == VL.IDENT_ISREPORT_BIT then
		FuncTable = AckFunctions --It's a report stream
	else
		FuncTable = SubcommandFunctions
	end
	
	if not FuncTable[SetupArgs.Subcommand] then
		ZigError('Unknown subcommand ' .. tostring(SetupArgs.Subcommand) .. ' triggered for stream from node ' .. SetupArgs.Origin)
		return
	end
	
	FuncTable[SetupArgs.Subcommand](self, SetupArgs, Stream)
end

function Ziggurat:ProcessN2NQueue()
	while true do
		local Stream = VL.RecvN2N()
		
		if not Stream then break end
		
		self:ProcessSingleN2N(Stream)
	end
end

function Ziggurat:ProcessPings()
	for ID, Peer in pairs(Peers) do
		if Peer.LastActivity + PingoutSecs <= os.time() then
			local EndMsg = Ziggurat:NewEmptyStream(NodeID, ZIGCMD_UNGREET, false)
			
			ZigDebug('Pingout detected, sending ungreet to node ' .. ID)
			
			VL.SendN2N(EndMsg)
			
			self:OnRemoteSessionTerminated(ID)
			
			Peers[ID] = nil
			
			return self:ProcessPings() --Restart loop
		elseif Peer.LastOutgoingPing + PingIntervalSecs <= os.time() then
			Peer:SendPing() --Send pings to everyone we know.
		end
	end
end
		
function Ziggurat:MainLoopIter()
	self:ProcessQtEvents()
	self:ProcessN2NQueue()
	self:ProcessPings()
	VL.vl_sleep(20)
end

if not VL.RunningAsJob() then
	InitZiggurat()
end
