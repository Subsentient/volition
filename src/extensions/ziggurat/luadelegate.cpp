#include "libvolition/include/conation.h"
#include "libvolition/include/utils.h"
#include "ziggurat.h"
#include <atomic>

extern "C"
{
#include <lua.h>
}
#include <QtWidgets>


static int ZigDeleteNativeMsg(lua_State *State)
{
	if (lua_getfield(State, 1, "RenderedData") != LUA_TLIGHTUSERDATA) return 0; //Not ready to render so probably didn't have one
	
	delete static_cast<Ziggurat::ZigMessage*>(lua_touserdata(State, 1));
	
	return 0;
}
	
static int ZigRenderTextMessage(lua_State *State)
{
	VLASSERT(lua_getfield(State, 1, "Delegate") == LUA_TLIGHTUSERDATA);
	VLASSERT(lua_type(State, 2) == LUA_TSTRING);
	VLASSERT(lua_type(State, 3) == LUA_TNUMBER);
	VLASSERT(lua_type(State, 4) == LUA_TSTRING);
	
	Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));
	Ziggurat::ZigMessage *const Msg = new Ziggurat::ZigMessage(lua_tostring(State, 2), lua_tointeger(State, 3), lua_tostring(State, 4));
	
	Delegate->RenderDisplayMessage(Msg);
	
	lua_pushboolean(State, true);
	return 1;
}

static int ZigRenderLinkMessage(lua_State *State)
{
	VLASSERT(lua_getfield(State, 1, "Delegate") == LUA_TLIGHTUSERDATA);
	VLASSERT(lua_type(State, 2) == LUA_TSTRING);
	VLASSERT(lua_type(State, 3) == LUA_TNUMBER);
	VLASSERT(lua_type(State, 4) == LUA_TSTRING);
	
	Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));
	Ziggurat::ZigMessage *const Msg = new Ziggurat::ZigMessage(lua_tostring(State, 2), lua_tointeger(State, 3), lua_tostring(State, 4), Ziggurat::ZigMessage::ZIGMSG_LINK);
	
	Delegate->RenderDisplayMessage(Msg);
	
	lua_pushboolean(State, true);
	return 1;
}

static int ZigGetHasFocus(lua_State *State)
{
	VLASSERT(lua_getfield(State, 1, "Delegate") == LUA_TLIGHTUSERDATA);
	
	Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));
	
	lua_pushboolean(State, Delegate->GetHasFocus());
	return 1;
}

static int ZigLoadFont(lua_State *State)
{
	VLASSERT(lua_getfield(State, 1, "Delegate") == LUA_TLIGHTUSERDATA);
	
	Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));

	if (!Delegate)
	{
		VLWARN("Delegate is null pointer!");
		return 0;
	}
	
	QFont Font;
	Font.fromString(lua_tostring(State, 2));
	
	emit Delegate->LoadFont(Font);
	return 0;
}

static int ZigRenderImageMessage(lua_State *State)
{
	VLASSERT(lua_getfield(State, 1, "Delegate") == LUA_TLIGHTUSERDATA);
	VLASSERT(lua_type(State, 2) == LUA_TSTRING);
	VLASSERT(lua_type(State, 3) == LUA_TNUMBER);
	VLASSERT(lua_type(State, 4) == LUA_TSTRING);
	VLASSERT(lua_type(State, 5) == LUA_TSTRING);
	
	Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));
	
	const VLString Node { lua_tostring(State, 2) };
	
	const uint32_t MsgID = lua_tointeger(State, 3);
	
	const VLString Text { lua_tostring(State, 4) };
	
	size_t ImageSize = 0u;
	const char *ImageData = lua_tolstring(State, 5, &ImageSize);
	
	std::vector<uint8_t> Image;
	Image.resize(ImageSize);
	
	memcpy(Image.data(), ImageData, ImageSize);
	
	Ziggurat::ZigMessage *const Msg = new Ziggurat::ZigMessage(Node, MsgID, Text, std::move(Image));
	
	Delegate->RenderDisplayMessage(Msg);
	
	lua_pushboolean(State, true);
	return 1;
}


static int ZigFireAudioNotification(lua_State *State)
{
	lua_pushboolean(State, Ziggurat::PlayAudioNotification(":zigdefault.wav"));
	
	return 1;
}

static int ZigAddNode(lua_State *State)
{
	VLASSERT(lua_getfield(State, 1, "Delegate") == LUA_TLIGHTUSERDATA);
	VLASSERT(lua_type(State, 2) == LUA_TSTRING);

	Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));
	
	VLASSERT_ERRMSG(Delegate != nullptr, "Delegate is null!");
	Delegate->AddNode(lua_tostring(State, 2));
	
	return 0;
}

static int ZigOnRemoteSessionTerminated(lua_State *State)
{
	VLASSERT(lua_getfield(State, 1, "Delegate") == LUA_TLIGHTUSERDATA);
	VLASSERT(lua_type(State, 2) == LUA_TSTRING);

	Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));
	
	VLASSERT_ERRMSG(Delegate != nullptr, "Delegate is null!");

	emit Delegate->RemoteSessionTerminated(lua_tostring(State, 2));
	
	return 0;
}

#ifdef WIN32
asm (".section .drectve");
asm (".ascii \"-export:InitLibZiggurat\"");
#endif //WIN32

extern "C" DLLEXPORT int InitLibZiggurat(lua_State *State)
{
	lua_settop(State, 0);
	
	static std::atomic_bool AlreadyRunning;
	
	if (AlreadyRunning)
	{
		VLWARN("Ziggurat shared library is already running, allowing request to fail gracefully.");
		return 0;
	}
	
	AlreadyRunning = true;

	Ziggurat::LuaDelegate *const Delegate = Ziggurat::LuaDelegate::Fireup(State);
	
	VLASSERT_ERRMSG(Delegate != nullptr, "Fireup failed, returned null somehow!");
	
	lua_getglobal(State, "Ziggurat");
	
	lua_pushstring(State, "Delegate");
	lua_pushlightuserdata(State, Delegate);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "DeleteNativeMsg");
	lua_pushcfunction(State, ZigDeleteNativeMsg);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "GetHasFocus");
	lua_pushcfunction(State, ZigGetHasFocus);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "LoadFont");
	lua_pushcfunction(State, ZigLoadFont);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "AddNode");
	lua_pushcfunction(State, ZigAddNode);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "RenderTextMessage");
	lua_pushcfunction(State, ZigRenderTextMessage);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "RenderLinkMessage");
	lua_pushcfunction(State, ZigRenderLinkMessage);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "RenderImageMessage");
	lua_pushcfunction(State, ZigRenderImageMessage);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "FireAudioNotification");
	lua_pushcfunction(State, ZigFireAudioNotification);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "OnRemoteSessionTerminated");
	lua_pushcfunction(State, ZigOnRemoteSessionTerminated);
	
	lua_settable(State, -3);

	lua_pushstring(State, "ProcessQtEvents");
	lua_pushcfunction(State,
	[] (lua_State *State) -> int
	{
		VLASSERT(lua_getfield(State, -1, "Delegate") == LUA_TLIGHTUSERDATA);
		
		Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));
		
		Delegate->ProcessQtEvents();
		return 0;
	});

	lua_settable(State, -3);
	
	lua_pushboolean(State, true);
	return 1;
}

Ziggurat::LuaDelegate::LuaDelegate(ZigMainWindow *Win, VLThreads::Thread *ThreadObj, lua_State *LuaState)
	: QObject(), Window(Win), ThreadObj(ThreadObj), EventLoop(new QEventLoop), LuaState(LuaState)
{
	this->Window->show();
	QObject::connect(this, &LuaDelegate::MessageToSend, this, &LuaDelegate::OnMessageToSend, Qt::ConnectionType::QueuedConnection);
}

auto Ziggurat::LuaDelegate::Fireup(lua_State *State) -> LuaDelegate*
{
	VLThreads::ValueWaiter<ZigMainWindow*> Waiter;

	VLThreads::Thread *const ThreadObj = new VLThreads::Thread(&ZigMainWindow::ThreadFuncInit, &Waiter);
	VLDEBUG("Executing thread");
	ThreadObj->Start();
	
	VLDEBUG("Getting Win");
	ZigMainWindow *Win = Waiter.Await();
	
	VLDEBUG("Making delegate");
	LuaDelegate *Delegate = new LuaDelegate { Win, ThreadObj, State };
	
	QObject::connect(Win, &ZigMainWindow::SendClicked, Delegate, &LuaDelegate::MessageToSend, Qt::ConnectionType::QueuedConnection);
	QObject::connect(Win, &ZigMainWindow::NewNodeChosen, Delegate, &LuaDelegate::OnNewNodeChosen, Qt::ConnectionType::QueuedConnection);
	QObject::connect(Win, &ZigMainWindow::NewFontSelected, Delegate, &LuaDelegate::OnNewFontSelected, Qt::ConnectionType::QueuedConnection);
	QObject::connect(Win, &ZigMainWindow::SessionEndRequested, Delegate, &LuaDelegate::OnSessionEndRequested, Qt::ConnectionType::QueuedConnection);
	QObject::connect(Win, &ZigMainWindow::NativeMessageReady, Delegate, &LuaDelegate::OnNativeMessageReady, Qt::ConnectionType::QueuedConnection);
	QObject::connect(Delegate, &LuaDelegate::RemoteSessionTerminated, Win, &ZigMainWindow::OnRemoteSessionTerminated, Qt::ConnectionType::QueuedConnection);
	QObject::connect(Delegate, &LuaDelegate::LoadFont, Win, &ZigMainWindow::OnLoadFont, Qt::ConnectionType::QueuedConnection);
	return Delegate;
}

void Ziggurat::LuaDelegate::OnNewNodeChosen(const QString NodeID)
{
	lua_settop(this->LuaState, 0);
	
	lua_getglobal(this->LuaState, "Ziggurat");
	
	if (lua_type(this->LuaState, -1) != LUA_TTABLE)
	{
		VLWARN("Ziggurat Lua global is not set correctly!");
		lua_settop(this->LuaState, 0);
		return;
	}
	
	lua_pushstring(this->LuaState, "OnNewNodeChosen");
	lua_gettable(this->LuaState, -2);
	
	if (lua_type(this->LuaState, -1) != LUA_TFUNCTION)
	{
		VLDEBUG("No function for OnNewNodeChosen detected, exiting method.");
		return;
	}
	
	VLDEBUG("New node ID is " + qs2vls(NodeID));

	lua_pushvalue(this->LuaState, 1);
	lua_pushstring(this->LuaState, qs2vls(NodeID));
	
	if (lua_pcall(this->LuaState, 2, 0, 0) != LUA_OK)
	{
		VLWARN("Call of lua callback failed!");
		return;
	}
}

void Ziggurat::LuaDelegate::OnNativeMessageReady(Ziggurat::ZigMessage *const Msg)
{
	lua_settop(this->LuaState, 0);
	
	lua_getglobal(this->LuaState, "Ziggurat");
	
	if (lua_type(this->LuaState, -1) != LUA_TTABLE)
	{
		VLWARN("Ziggurat Lua global is not set correctly!");
		lua_settop(this->LuaState, 0);
		return;
	}
	
	lua_pushstring(this->LuaState, "OnNativeMessageReady");
	lua_gettable(this->LuaState, -2);
	
	if (lua_type(this->LuaState, -1) != LUA_TFUNCTION)
	{
		VLDEBUG("No function for OnNativeMessageReady detected, exiting method.");
		return;
	}
	
	VLDEBUG("Calling Lua callback to mark native data for msg " + Msg->GetNode() + " :: " + VLString::UintToString(Msg->GetMsgID()));

	lua_pushvalue(this->LuaState, 1);
	lua_pushstring(this->LuaState, Msg->GetNode());
	lua_pushinteger(this->LuaState, Msg->GetMsgID());
	lua_pushlightuserdata(this->LuaState, Msg);
	
	if (lua_pcall(this->LuaState, 4, 0, 0) != LUA_OK)
	{
		VLWARN("Call of lua callback failed!");
		return;
	}
}

void Ziggurat::LuaDelegate::OnNewFontSelected(const QFont &Selected)
{
	VLDEBUG("Storing font string " + qs2vls(Selected.toString()));
	
	lua_settop(this->LuaState, 0);
	
	lua_getglobal(this->LuaState, "Ziggurat");
	
	if (lua_type(this->LuaState, -1) != LUA_TTABLE)
	{
		VLWARN("Ziggurat Lua global is not set correctly!");
		lua_settop(this->LuaState, 0);
		return;
	}
	
	lua_pushstring(this->LuaState, "OnNewFontSelected");
	lua_gettable(this->LuaState, -2);
	
	if (lua_type(this->LuaState, -1) != LUA_TFUNCTION)
	{
		VLDEBUG("No function for OnNewFontSelected detected, exiting method.");
		return;
	}
	
	VLDEBUG("Calling Lua callback to set font to " + qs2vls(Selected.toString()));

	lua_pushvalue(this->LuaState, 1);
	lua_pushstring(this->LuaState, qs2vls(Selected.toString()));
	
	if (lua_pcall(this->LuaState, 2, 0, 0) != LUA_OK)
	{
		VLWARN("Call of lua callback failed!");
		return;
	}
}

void Ziggurat::LuaDelegate::OnSessionEndRequested(const QString NodeID)
{
	lua_settop(this->LuaState, 0);
	
	lua_getglobal(this->LuaState, "Ziggurat");
	
	if (lua_type(this->LuaState, -1) != LUA_TTABLE)
	{
		VLWARN("Ziggurat Lua global is not set correctly!");
		lua_settop(this->LuaState, 0);
		return;
	}
	
	lua_pushstring(this->LuaState, "OnSessionEndRequested");
	lua_gettable(this->LuaState, -2);
	
	if (lua_type(this->LuaState, -1) != LUA_TFUNCTION)
	{
		VLDEBUG("No function for OnSessionEndRequested detected, exiting method.");
		return;
	}
	
	VLDEBUG("Calling Lua callback to end session for node " + qs2vls(NodeID));

	lua_pushvalue(this->LuaState, 1);
	lua_pushstring(this->LuaState, qs2vls(NodeID));
	
	if (lua_pcall(this->LuaState, 2, 0, 0) != LUA_OK)
	{
		VLWARN("Call of lua callback failed!");
		return;
	}
}

void Ziggurat::LuaDelegate::RenderDisplayMessage(ZigMessage *const Msg)
{
	this->Window->RenderDisplayMessage(Msg);
}

void Ziggurat::LuaDelegate::OnMessageToSend(const QString Node, const QString Msg)
{
	lua_settop(this->LuaState, 0);
	lua_getglobal(this->LuaState, "Ziggurat");
	
	if (lua_type(this->LuaState, -1) != LUA_TTABLE)
	{
		VLWARN("Ziggurat Lua global is not set correctly!");
		lua_settop(this->LuaState, 0);
		return;
	}
	
	lua_pushstring(this->LuaState, "OnMessageToSend");
	lua_gettable(this->LuaState, -2);
	
	if (lua_type(this->LuaState, -1) != LUA_TFUNCTION)
	{
		VLDEBUG("No function for OnMessageToSend detected, exiting method.");
		return;
	}
	
	lua_pushvalue(this->LuaState, 1);
	lua_pushstring(this->LuaState, qs2vls(Node));
	lua_pushstring(this->LuaState, qs2vls(Msg));
	
	if (lua_pcall(this->LuaState, 3, 0, 0) != LUA_OK)
	{
		VLWARN("Call of lua callback failed! Got error \"" + lua_tostring(this->LuaState, -1) + "\".");
		return;
	}
	
	VLDEBUG("Lua method call succeeded");
}

void Ziggurat::LuaDelegate::ProcessQtEvents(void) const
{
	this->EventLoop->processEvents();
}

void Ziggurat::LuaDelegate::AddNode(const VLString &Node)
{
	emit this->Window->NodeAdded(+Node);
}
