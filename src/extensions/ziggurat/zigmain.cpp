#include "libvolition/include/conation.h"
#include "libvolition/include/utils.h"
#include "node/script.h"
#include "zigapp.h"


extern "C"
{
#include <lua.h>
}

#include <QtWidgets>

static const char *fakeargv[] = { "vl", nullptr };
static int fakeargc = 1;

static VLScopedPtr<QApplication*> App;

Ziggurat::ZigMainWindow *Ziggurat::ZigMainWindow::Instance;

extern "C" int vlltumorexec(lua_State *State)
{
	Ziggurat::LuaDelegate *Delegate = Ziggurat::LuaDelegate::Fireup();
	
	lua_newtable(State);
	
	lua_pushstring(State, "Delegate");
	lua_pushlightuserdata(State, Delegate);
	
	lua_settable(State, -3);
	
	lua_pushstring(State, "EventLoop");
	lua_pushlightuserdata(State, new QEventLoop);
	
	lua_settable(State, -3);

	lua_pushstring(State, "ProcessQtEvents");
	lua_pushcfunction(State,
	[] (lua_State *State) -> int
	{
		VLASSERT(lua_getfield(State, -1, "EventLoop") == LUA_TLIGHTUSERDATA);
		
		QEventLoop *EventLoop = static_cast<QEventLoop*>(lua_touserdata(State, -1));
		
		EventLoop->processEvents();
		
		return 0;
	});

	lua_settable(State, -3);
	
	return 1;
}


void *Ziggurat::ZigMainWindow::ThreadFuncInit(void *Waiter_)
{
	VLThreads::ValueWaiter<ZigMainWindow*> *const Waiter = static_cast<VLThreads::ValueWaiter<ZigMainWindow*>*>(Waiter_);

	VLDEBUG("Constructing QApplication");
	if (!QApplication::instance()) App = new QApplication { fakeargc, (char**)fakeargv };
	
	VLDEBUG("Constructing ZigWin");
	ZigMainWindow *const ZigWin = new ZigMainWindow;

	ZigWin->show();

	VLDEBUG("Sending ZigWin");
	Waiter->Post(ZigWin);
	
	VLDEBUG("Executing main loop");
	
	ZigWin->ThreadFunc();
	
	return nullptr;
}

void Ziggurat::ZigMainWindow::OnNewMessage(const QString &Data)
{
	*this->MessageList << Data;
	this->MessageListModel->setStringList(*this->MessageList);
}

void Ziggurat::ZigMainWindow::ThreadFunc(void)
{
	this->MessageListModel = new QStringListModel;
	this->MessageList = new QStringList { "No messages present" };
	
	this->MessageListModel->setStringList(*this->MessageList);
	
	this->ZigMessageList->setModel(this->MessageListModel);
	
	QEventLoop Loop;
	
	QObject::connect(this, &ZigMainWindow::ZigDies, this, [&Loop] { Loop.exit(); }, Qt::ConnectionType::QueuedConnection);
	QObject::connect(this, &ZigMainWindow::NewMessage, this, &ZigMainWindow::OnNewMessage, Qt::ConnectionType::QueuedConnection);
	
	Loop.exec();
}

	
Ziggurat::ZigMainWindow::ZigMainWindow(void) : QMainWindow()
{
	setupUi(this);
}

Ziggurat::LuaDelegate::LuaDelegate(ZigMainWindow *Win, VLThreads::Thread *ThreadObj)
	: QObject(), Window(Win), ThreadObj(ThreadObj)
{
	QObject::connect(this, &LuaDelegate::NewMessage, this->Window, &ZigMainWindow::NewMessage, Qt::ConnectionType::QueuedConnection);
	
}

auto Ziggurat::LuaDelegate::Fireup(void) -> LuaDelegate*
{
	VLThreads::ValueWaiter<ZigMainWindow*> Waiter;

	VLThreads::Thread *const ThreadObj = new VLThreads::Thread(&ZigMainWindow::ThreadFuncInit, &Waiter);
	VLDEBUG("Executing thread");
	ThreadObj->Start();
	
	VLDEBUG("Getting Win");
	ZigMainWindow *Win = Waiter.Await();
	
	VLDEBUG("Making delegate");
	LuaDelegate *Delegate = new LuaDelegate { Win, ThreadObj };
	
	return Delegate;
}
void Ziggurat::LuaDelegate::OnMessageToSend(const QString &Msg)
{
}

