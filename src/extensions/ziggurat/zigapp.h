#include "libvolition/include/common.h"
#include "libvolition/include/vlthreads.h"
#include "ui_zigui.h"

#include <QtWidgets>

namespace Ziggurat
{
	class LuaDelegate;
	
	class ZigMainWindow : public QMainWindow, public Ui::ZigguratWindow
	{
		Q_OBJECT
	private:
		VLScopedPtr<QStringListModel*> MessageListModel;
		VLScopedPtr<QStringList*> MessageList;
		void ThreadFunc(void);
		static void *ThreadFuncInit(void *Waiter_);

	public:
		static ZigMainWindow *Instance;
		
		ZigMainWindow(void);
		
		inline void AddMessage(const VLString &Msg) { emit NewMessage(+Msg); }
		
	public slots:
		void OnNewMessage(const QString &Data);

	signals:
		void ZigDies(void);
		void NewMessage(const QString &Data);
		
		friend class LuaDelegate;
	};
	
	class LuaDelegate : public QObject
	{
		Q_OBJECT
	private:
		VLScopedPtr<ZigMainWindow*> Window;
		VLScopedPtr<VLThreads::Thread*> ThreadObj;

	public slots:
		void OnMessageToSend(const QString &Msg);
	signals:
		void MessageToSend(const QString &Msg);
		void NewMessage(const QString &Msg);
	public:
		LuaDelegate(ZigMainWindow *Win, VLThreads::Thread *ThreadObj);
	public:
		static LuaDelegate *Fireup(void);
	};
}

	
