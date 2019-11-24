#include "libvolition/include/conation.h"
#include "libvolition/include/utils.h"
#include "ziggurat.h"
#include <atomic>

extern "C"
{
#include <lua.h>
}
#include <QtWidgets>

static int ZigPushTextMessage(lua_State *State)
{
	VLASSERT(lua_getfield(State, 1, "Delegate") == LUA_TLIGHTUSERDATA);
	VLASSERT(lua_type(State, 2) == LUA_TSTRING);
	VLASSERT(lua_type(State, 3) == LUA_TSTRING);
	
	Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));
	Ziggurat::ZigMessage *const Msg = new Ziggurat::ZigMessage(lua_tostring(State, 2), lua_tostring(State, 3));
	
	Delegate->PushIncomingMessage(Msg);
	
	lua_pushboolean(State, true);
	return 1;
}

static int ZigPushLinkMessage(lua_State *State)
{
	VLASSERT(lua_getfield(State, 1, "Delegate") == LUA_TLIGHTUSERDATA);
	VLASSERT(lua_type(State, 2) == LUA_TSTRING);
	VLASSERT(lua_type(State, 3) == LUA_TSTRING);
	
	Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));
	Ziggurat::ZigMessage *const Msg = new Ziggurat::ZigMessage(lua_tostring(State, 2), lua_tostring(State, 3), Ziggurat::ZigMessage::ZIGMSG_LINK);
	
	Delegate->PushIncomingMessage(Msg);
	
	lua_pushboolean(State, true);
	return 1;
}

static int ZigPushImageMessage(lua_State *State)
{
	VLASSERT(lua_getfield(State, 1, "Delegate") == LUA_TLIGHTUSERDATA);
	VLASSERT(lua_type(State, 2) == LUA_TSTRING);
	VLASSERT(lua_type(State, 3) == LUA_TSTRING);
	
	Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));
	
	const VLString TargetNode { lua_tostring(State, 2) };
	
	size_t ImageSize = 0u;
	const char *ImageData = lua_tolstring(State, 3, &ImageSize);
	
	std::vector<uint8_t> Image;
	Image.resize(ImageSize);
	
	memcpy(Image.data(), ImageData, ImageSize);
	
	Ziggurat::ZigMessage *const Msg = new Ziggurat::ZigMessage(TargetNode, std::move(Image));
	
	Delegate->PushIncomingMessage(Msg);
	
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

static int ZigPopOutgoingMessage(lua_State *State)
{
	VLASSERT(lua_getfield(State, 1, "Delegate") == LUA_TLIGHTUSERDATA);
	VLASSERT(lua_type(State, 2) == LUA_TSTRING);
	
	Ziggurat::LuaDelegate *const Delegate = static_cast<Ziggurat::LuaDelegate*>(lua_touserdata(State, -1));
	
	const VLString &Msg { Delegate->PopOutgoingMessage(lua_tostring(State, 2)) };
	
	if (!Msg) return 0;
	
	lua_pushstring(State, Msg);
	return 1;
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

	Ziggurat::LuaDelegate *Delegate = Ziggurat::LuaDelegate::Fireup(State);
	
	VLASSERT_ERRMSG(Delegate != nullptr, "Fireup failed, returned null somehow!");
	
	lua_newtable(State);
	
	lua_pushstring(State, "Delegate");
	lua_pushlightuserdata(State, Delegate);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "AddNode");
	lua_pushcfunction(State, ZigAddNode);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "PushTextMessage");
	lua_pushcfunction(State, ZigPushTextMessage);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "PushLinkMessage");
	lua_pushcfunction(State, ZigPushLinkMessage);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "PushImageMessage");
	lua_pushcfunction(State, ZigPushImageMessage);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "PopNextOutgoing");
	lua_pushcfunction(State, ZigPopOutgoingMessage);
	
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
	
	lua_setglobal(State, "Ziggurat");
	lua_getglobal(State, "Ziggurat"); //Because we just popped it off the stack.
	
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

void Ziggurat::LuaDelegate::PushIncomingMessage(const ZigMessage *const Msg)
{
	this->Window->AddIncomingMessage(Msg);
}

VLString Ziggurat::LuaDelegate::PopOutgoingMessage(const VLString &TargetNode)
{
	VLThreads::MutexKeeper MessengersKeeper { &this->Window->MessengersLock };
	
	if (!this->Window->Messengers.count(TargetNode)) return {};
	
	ZigMessengerWidget *const Messenger = this->Window->Messengers.at(TargetNode);
	
	VLThreads::MutexKeeper OutKeeper { &Messenger->OutLock };
	
	if (Messenger->OutgoingMessageList.empty()) return {};
	
	const VLString Msg { Messenger->OutgoingMessageList.front() };
	
	Messenger->OutgoingMessageList.pop_front();
	
	return Msg;
}
	
void Ziggurat::LuaDelegate::OnMessageToSend(const QString &TargetNode, const QString &Msg)
{
	lua_settop(this->LuaState, 0);
	lua_getglobal(this->LuaState, "Ziggurat");
	
	if (lua_type(this->LuaState, -1) != LUA_TTABLE)
	{
		VLWARN("Ziggurat Lua global is not set correctly!");
		lua_settop(this->LuaState, 0);
		return;
	}
	
	lua_pushstring(this->LuaState, "OnIncomingMessage");
	lua_gettable(this->LuaState, -2);
	
	if (lua_type(this->LuaState, -1) != LUA_TFUNCTION)
	{
		VLDEBUG("No function for OnIncomingMessage detected, exiting method.");
		return;
	}
	
	lua_pushstring(this->LuaState, qs2vls(TargetNode));
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

void Ziggurat::LuaDelegate::AddNode(const VLString &TargetNode)
{
	emit this->Window->NodeAdded(+TargetNode);
}
