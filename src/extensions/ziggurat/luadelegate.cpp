#include "libvolition/include/conation.h"
#include "libvolition/include/utils.h"
#include "ziggurat.h"
#include <atomic>

extern "C"
{
#include <lua.h>
}
#include <QtWidgets>

static int ZigRenderTextMessage(lua_State *State)
{
	VLASSERT(lua_getfield(State, 1, "Delegate") == LUA_TLIGHTUSERDATA);
	VLASSERT(lua_type(State, 2) == LUA_TSTRING);
	VLASSERT(lua_type(State, 3) == LUA_TSTRING);
	
	Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));
	Ziggurat::ZigMessage *const Msg = new Ziggurat::ZigMessage(lua_tostring(State, 2), lua_tostring(State, 3));
	
	Delegate->RenderDisplayMessage(Msg);
	
	lua_pushboolean(State, true);
	return 1;
}

static int ZigRenderLinkMessage(lua_State *State)
{
	VLASSERT(lua_getfield(State, 1, "Delegate") == LUA_TLIGHTUSERDATA);
	VLASSERT(lua_type(State, 2) == LUA_TSTRING);
	VLASSERT(lua_type(State, 3) == LUA_TSTRING);
	
	Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));
	Ziggurat::ZigMessage *const Msg = new Ziggurat::ZigMessage(lua_tostring(State, 2), lua_tostring(State, 3), Ziggurat::ZigMessage::ZIGMSG_LINK);
	
	Delegate->RenderDisplayMessage(Msg);
	
	lua_pushboolean(State, true);
	return 1;
}

static int ZigRenderImageMessage(lua_State *State)
{
	VLASSERT(lua_getfield(State, 1, "Delegate") == LUA_TLIGHTUSERDATA);
	VLASSERT(lua_type(State, 2) == LUA_TSTRING);
	VLASSERT(lua_type(State, 3) == LUA_TSTRING);
	
	Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));
	
	const VLString Node { lua_tostring(State, 2) };
	
	size_t ImageSize = 0u;
	const char *ImageData = lua_tolstring(State, 3, &ImageSize);
	
	std::vector<uint8_t> Image;
	Image.resize(ImageSize);
	
	memcpy(Image.data(), ImageData, ImageSize);
	
	Ziggurat::ZigMessage *const Msg = new Ziggurat::ZigMessage(Node, std::move(Image));
	
	Delegate->RenderDisplayMessage(Msg);
	
	lua_pushboolean(State, true);
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
	
extern "C" int InitLibZiggurat(lua_State *State)
{
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
	
	QObject::connect(Win, &ZigMainWindow::SendClicked, Delegate, &LuaDelegate::MessageToSend);
	return Delegate;
}

void Ziggurat::LuaDelegate::RenderDisplayMessage(const ZigMessage *const Msg)
{
	this->Window->RenderDisplayMessage(Msg);
}

void Ziggurat::LuaDelegate::OnMessageToSend(const QString &Node, const QString &Msg)
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
	
	lua_pushstring(this->LuaState, qs2vls(Node));
	lua_pushstring(this->LuaState, qs2vls(Msg));
	
	if (lua_pcall(this->LuaState, 2, 0, 0) != LUA_OK)
	{ //Call NewLuaConationStream() to get a new copy for arguments.
		VLWARN("Call of lua callback failed!");
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
