#include "libvolition/include/common.h"
#include "libvolition/include/vlthreads.h"
#include "ui_zigmainwindow.h"
#include "ui_zigtextchooser.h"
#include "ui_zigmessenger.h"
#include <queue>
#include <QtWidgets>
#include <QDesktopServices>

extern "C"
{
#include <lua.h>
}

#define qs2vls(String) (VLString{String.toUtf8().constData()})
#ifdef WIN32
#define DLLEXPORT __declspec(dllexport) __cdecl
#else
#define DLLEXPORT
#endif //WIN32
namespace Ziggurat
{
	class KeyboardMonitor : public QObject
	{
	    Q_OBJECT
	protected:
	    bool eventFilter(QObject *Object, QEvent *Event);
	signals:
		void KeyPress(QObject *Target, int Key);
	};
	
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
		std::vector<uint8_t> BinData;
		VLString Node;
		VLString Text;
		uint32_t MsgID;
		MessageType MsgType;
		VLScopedPtr<QWidget*> MsgWidget;

	public:
		inline ZigMessage(const VLString &Node, const uint32_t MsgID, const VLString &String, MessageType MsgType = ZIGMSG_TEXT)
						: Node(Node), Text(String), MsgID(MsgID), MsgType(MsgType)
		{
		}
		
		inline ZigMessage(const VLString &Node, const uint32_t MsgID, const VLString &String, const std::vector<uint8_t> &ImageData)
						: BinData(ImageData), Node(Node), Text(String), MsgID(MsgID), MsgType(ZIGMSG_IMAGE)
		{
		}
		inline ZigMessage(const VLString &Node, const uint32_t MsgID, const VLString &String, std::vector<uint8_t> &&ImageData)
						: BinData(ImageData), Node(Node), Text(String), MsgID(MsgID), MsgType(ZIGMSG_IMAGE)
		{
		}
		
		inline const VLString &GetNode(void) const { return this->Node; }
		inline uint32_t GetMsgID(void) const { return this->MsgID; }
		inline MessageType GetMsgType(void) const { return this->MsgType; }
				
		inline QLayout *PopulateImageWidget(const int Width, const int Height) const
		{
			QVBoxLayout *const Layout = new QVBoxLayout;
			Layout->setSpacing(0);
			Layout->setContentsMargins(0, 0, 0, 0);
			
			QLabel *const TextLabel = new QLabel(QString(+this->Text));
			QLabel *const ImageLabel = new QLabel;

			TextLabel->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::TextSelectableByMouse);
			TextLabel->setOpenExternalLinks(true);
			
			Layout->addWidget(TextLabel);
			Layout->addWidget(ImageLabel);

			QPixmap Pix { QPixmap::fromImage(QImage::fromData(this->BinData.data(), this->BinData.size())) };
			
			if (Width < Pix.width()) Pix = Pix.scaledToWidth(Width, Qt::SmoothTransformation);
			if (Height < Pix.height()) Pix = Pix.scaledToHeight(Height, Qt::SmoothTransformation);
			
			ImageLabel->setPixmap(std::move(Pix));
			
			return Layout;
		}
		
		inline QLayout *PopulateTextWidget(void) const
		{
			QVBoxLayout *const Layout = new QVBoxLayout;
			Layout->setSpacing(0);
			Layout->setContentsMargins(0, 0, 0, 0);
			
			QLabel *const Label = new QLabel(QString(+this->Text));
			Label->setWordWrap(true);
			Label->setTextInteractionFlags(Qt::TextSelectableByMouse);
			
			Layout->addWidget(Label);

			if (this->MsgType == ZIGMSG_LINK)
			{
				Label->setTextInteractionFlags(Label->textInteractionFlags() | Qt::TextBrowserInteraction);
				Label->setOpenExternalLinks(true);
			}
			
			return Layout;
		}
		
		inline QWidget *GetMsgWidget(void) const { return this->MsgWidget; }
		
		inline QWidget *RebuildMsgWidget(const int Width, const int Height)
		{
			QWidget *const BaseWidget = this->MsgWidget ? +this->MsgWidget : new QWidget;
			QLayout *ResultLayout = nullptr;
			
			switch (this->MsgType)
			{
				case ZIGMSG_IMAGE:
					ResultLayout = this->PopulateImageWidget(Width, Height);
					break;
				case ZIGMSG_TEXT:
				case ZIGMSG_LINK:
					ResultLayout = this->PopulateTextWidget();
					break;
				default:
					VLWARN("Bad ZIGMSG value");
					return nullptr;
			}
			
			if (!ResultLayout) return nullptr;
			
			delete BaseWidget->layout(); //If it's already there, this is necessary or Qt won't let us replace it.
			
			BaseWidget->setLayout(ResultLayout);
			
			this->MsgWidget = BaseWidget;

			return this->MsgWidget;
		}
	};
	
	class LuaDelegate;
	
	class ZigMessengerWidget : public QWidget, public Ui::ZigMessengerWidget
	{
		Q_OBJECT
	private:
		VLString Node;
		KeyboardMonitor *KeyMon;
		
	public:
		ZigMessengerWidget(const VLString &Node);
		
		inline const VLString GetNode(void) const { return this->Node; }
	public slots:
		void OnNewDisplayMessage(ZigMessage *const Item);
		void OnEnterPressed(QObject *Object, int Key);

	signals:
		void SendClicked(const QString Node, const QString Msg);
		void NewDisplayMessage(ZigMessage *Msg);
		void NativeMessageReady(ZigMessage *Msg);

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
		std::atomic_bool HasFocus;
		
		void ThreadFunc(void);
		static void *ThreadFuncInit(void *Waiter_);
	public:
		static ZigMainWindow *Instance;
		
		ZigMainWindow(void);
		
		inline void RenderDisplayMessage(ZigMessage *const Msg) { emit NewDisplayMessage(Msg); }
		virtual void closeEvent(QCloseEvent*);
	public slots:
		void OnNewDisplayMessage(ZigMessage *const Msg);
		void OnNodeAdded(const QString Node);
		void OnNewNodeClicked(void);
		void OnTabCloseClicked(int TabIndex);
		void OnRemoteSessionTerminated(const QString Node);
		void OnFocusAltered(QWidget *Old, QWidget *Now);

	signals:
		void ZigDies(void);
		void NewDisplayMessage(ZigMessage *Msg);
		void SendClicked(const QString Node, const QString Msg);
		void NodeAdded(const QString Node);
		void NewNodeChosen(const QString NodeID);
		void SessionEndRequested(const QString NodeID);
		void NativeMessageReady(ZigMessage *Msg);
	
		friend class LuaDelegate;
	};
	
	class ZigTextChooser : public QWidget, public Ui::ZigTextChooser
	{
		Q_OBJECT
	public:
		typedef void (*TextChooserCallback)(ZigTextChooser*, void*);
	private:
		TextChooserCallback DismissCallback, AcceptCallback;
		void *UserData;
	public:
		inline VLString GetValue(void) const
		{
			return qs2vls(this->TextChooserData->text());
		}
		
		inline void SetDismissCallback(const TextChooserCallback CB) 
		{
			this->DismissCallback = CB;
		}
		
		inline void SetAcceptCallback(const TextChooserCallback CB) 
		{
			this->AcceptCallback = CB;
		}
		
		ZigTextChooser(const VLString &WindowTitle,
						const VLString &PromptText,
						const TextChooserCallback AcceptCallback = nullptr,
						const TextChooserCallback DismissCallback = nullptr,
						void *UserData = nullptr);
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
		void OnMessageToSend(const QString Node, const QString Msg);
		void OnNewNodeChosen(const QString NodeID);
		void OnSessionEndRequested(const QString NodeID);
		void OnNativeMessageReady(ZigMessage *const Msg);

	signals:
		void MessageToSend(const QString Node, const QString Msg);
		void RemoteSessionTerminated(const QString Node);

	public:
		LuaDelegate(ZigMainWindow *Win, VLThreads::Thread *ThreadObj, lua_State *State);
	public:
		static LuaDelegate *Fireup(lua_State *State);
		void ProcessQtEvents(void) const;
		inline bool GetHasFocus(void) const { return this->Window->HasFocus; }
		void RenderDisplayMessage(ZigMessage *const Msg);
		void AddNode(const VLString &Node);
	};
	
	
	bool PlayAudioNotification(const VLString &AudioPath);
}

	
