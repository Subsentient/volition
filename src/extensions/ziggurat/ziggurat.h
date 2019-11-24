#include "libvolition/include/common.h"
#include "libvolition/include/vlthreads.h"
#include "ui_zigmainwindow.h"
#include "ui_zigmessenger.h"
#include <queue>
#include <QtWidgets>
#include <QDesktopServices>

extern "C"
{
#include <lua.h>
}

#define qs2vls(String) (VLString{String.toUtf8().constData()})

namespace Ziggurat
{
	
	struct ZigMessage
	{
	public:
		enum MessageType : uint8_t
		{
			ZIGMSG_INVALID		= 0,
			ZIGMSG_TEXT			= 1,
			ZIGMSG_IMAGE		= 2,
			ZIGMSG_LINK			= 3,
			ZIGMSG_MAX
		};
	private:
		MessageType MsgType;
		VLString TargetNode;
		std::vector<uint8_t> MsgData;
		
	public:
		inline ZigMessage(const VLString &TargetNode, const VLString &String, MessageType MsgType = ZIGMSG_TEXT) : MsgType(MsgType), TargetNode(TargetNode)
		{
			this->MsgData.resize(String.Length() + 1);
			memcpy(this->MsgData.data(), +String, String.Length() + 1); //Copy null terminator.
		}
		
		inline ZigMessage(const VLString &TargetNode, const std::vector<uint8_t> &ImageData) : MsgType(ZIGMSG_IMAGE), TargetNode(TargetNode), MsgData(ImageData) {}
		inline ZigMessage(const VLString &TargetNode, std::vector<uint8_t> &&ImageData) : MsgType(ZIGMSG_IMAGE), TargetNode(TargetNode), MsgData(ImageData) {}
		
		inline const VLString &GetTargetNode(void) const { return this->TargetNode; }
		
		inline QLabel *AsImage(void) const
		{
			QLabel *const Label = new QLabel;
			Label->setPixmap(QPixmap::fromImage(QImage::fromData(this->MsgData.data(), this->MsgData.size())));
			return Label;
		}
		
		inline QLabel *AsText(void) const
		{
			QLabel *const Label = new QLabel(QString((const char*)this->MsgData.data()));
			Label->setWordWrap(true);
			return Label;
		}
		
		inline QPushButton *AsLink(void) const
		{
			QPushButton *const Button = new QPushButton((const char*)this->MsgData.data());
			
			Button->setFlat(true);
			Button->setStyleSheet("QPushButton { color: blue; }");
			QObject::connect(Button, &QPushButton::clicked, Button, [Button] { QDesktopServices::openUrl(QUrl(Button->text())); });
			return Button;
		}
		
		inline QWidget *GetMsgWidget(void) const
		{
			switch (this->MsgType)
			{
				case ZIGMSG_IMAGE:
					return this->AsImage();
				case ZIGMSG_TEXT:
					return this->AsText();
				case ZIGMSG_LINK:
					return this->AsLink();
				default:
					VLWARN("Bad ZIGMSG value");
					break;
			}
			return nullptr;
		}
	};
	
	class LuaDelegate;
	
	class ZigMessengerWidget : public QWidget, public Ui::ZigMessengerWidget
	{
		Q_OBJECT
	private:
		VLString TargetNode;
		std::list<VLScopedPtr<const ZigMessage*> > IncomingMessageList;
		std::list<VLString> OutgoingMessageList;
		VLThreads::Mutex InLock, OutLock;
		
	public:
		ZigMessengerWidget(const VLString &TargetNode);
		
		inline const VLString GetTargetNode(void) const { return this->TargetNode; }
	public slots:
		void OnNewIncomingMessage(const ZigMessage *const Item);
	signals:
		void SendClicked(const QString &TargetNode, const QString &Msg);
		void NewIncomingMessage(const ZigMessage *Msg);
		
		/*	LuaDelegate is your friend.
		* 	You trust LuaDelegate.
		* 	You love LuaDelegate.
		*	You will allow LuaDelegate to suck the horse cum from your grandmother's asshole.
		*/
		friend class LuaDelegate;
		
	};

	class ZigMainWindow : public QMainWindow, public Ui::ZigMainWindow
	{
		Q_OBJECT
	private:
		std::map<VLString, VLScopedPtr<ZigMessengerWidget*> > Messengers;
		VLThreads::Mutex MessengersLock;
		
		void ThreadFunc(void);
		static void *ThreadFuncInit(void *Waiter_);
	public:
		static ZigMainWindow *Instance;
		
		ZigMainWindow(void);
		
		inline void AddIncomingMessage(const ZigMessage *const Msg) { emit NewIncomingMessage(Msg); }
		
	public slots:
		void OnNewIncomingMessage(const ZigMessage *const Msg);
		void OnNodeAdded(const QString &TargetNode);
	signals:
		void ZigDies(void);
		void NewIncomingMessage(const ZigMessage *Msg);
		void SendClicked(const QString &TargetNode, const QString &Msg);
		void NodeAdded(const QString &TargetNode);
		friend class LuaDelegate;
	};
	
	class LuaDelegate : public QObject
	{
		Q_OBJECT
	private:
		VLScopedPtr<ZigMainWindow*> Window;
		VLScopedPtr<VLThreads::Thread*> ThreadObj;
		VLScopedPtr<QEventLoop*> EventLoop;
		lua_State *LuaState;

	public slots:
		void OnMessageToSend(const QString &TargetNode, const QString &Msg);
	signals:
		void MessageToSend(const QString &TargetNode, const QString &Msg);
	public:
		LuaDelegate(ZigMainWindow *Win, VLThreads::Thread *ThreadObj, lua_State *State);
	public:
		static LuaDelegate *Fireup(lua_State *State);
		void ProcessQtEvents(void) const;
		
		void PushIncomingMessage(const ZigMessage *const Msg);
		VLString PopOutgoingMessage(const VLString &TargetNode);
		void AddNode(const VLString &TargetNode);
	};
}

	
