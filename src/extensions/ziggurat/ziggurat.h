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
		VLString Node;
		VLString Text;
		std::vector<uint8_t> BinData;
		
	public:
		inline ZigMessage(const VLString &Node, const VLString &String, MessageType MsgType = ZIGMSG_TEXT)
						: MsgType(MsgType), Node(Node), Text(String)
		{
		}
		
		inline ZigMessage(const VLString &Node, const VLString &String, const std::vector<uint8_t> &ImageData)
						: MsgType(ZIGMSG_IMAGE), Node(Node), Text(String), BinData(ImageData)
		{
		}
		inline ZigMessage(const VLString &Node, const VLString &String, std::vector<uint8_t> &&ImageData)
						: MsgType(ZIGMSG_IMAGE), Node(Node), Text(String), BinData(ImageData)
		{
		}
		
		inline const VLString &GetNode(void) const { return this->Node; }
		
		inline QWidget *AsImage(const int Width, const int Height) const
		{
			QWidget *const BaseWidget = new QWidget;
			QVBoxLayout *const Layout = new QVBoxLayout;
			
			BaseWidget->setLayout(Layout);
			
			QLabel *const TextLabel = new QLabel(BaseWidget);
			QLabel *const ImageLabel = new QLabel(BaseWidget);

			TextLabel->setText(+this->Text);
			TextLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
			TextLabel->setOpenExternalLinks(true);
			
			Layout->addWidget(TextLabel);
			Layout->addWidget(ImageLabel);
			
			QPixmap Pix { QPixmap::fromImage(QImage::fromData(this->BinData.data(), this->BinData.size())) };
			
			if (Width < Pix.width()) Pix = Pix.scaledToWidth(Width);
			if (Height < Pix.height()) Pix = Pix.scaledToHeight(Height);
			
			ImageLabel->setPixmap(std::move(Pix));
			
			TextLabel->show();
			ImageLabel->show();
			return BaseWidget;
		}
		
		inline QLabel *AsText(void) const
		{
			QLabel *const Label = new QLabel(QString(+this->Text));
			Label->setWordWrap(true);
			
			if (this->MsgType == ZIGMSG_LINK)
			{
				Label->setTextInteractionFlags(Qt::TextBrowserInteraction);
				Label->setOpenExternalLinks(true);
			}
			
			return Label;
		}
		
		inline QWidget *GetMsgWidget(const int Width, const int Height) const
		{
			switch (this->MsgType)
			{
				case ZIGMSG_IMAGE:
					return this->AsImage(Width, Height);
				case ZIGMSG_TEXT:
				case ZIGMSG_LINK:
					return this->AsText();
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
		VLString Node;
		
	public:
		ZigMessengerWidget(const VLString &Node);
		
		inline const VLString GetNode(void) const { return this->Node; }
	public slots:
		void OnNewDisplayMessage(const ZigMessage *const Item);
	signals:
		void SendClicked(const QString Node, const QString Msg);
		void NewDisplayMessage(const ZigMessage *Msg);
		
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
		
		inline void RenderDisplayMessage(const ZigMessage *const Msg) { emit NewDisplayMessage(Msg); }
		virtual void closeEvent(QCloseEvent*);
	public slots:
		void OnNewDisplayMessage(const ZigMessage *const Msg);
		void OnNodeAdded(const QString Node);
		void OnNewNodeClicked(void);
		void OnTabCloseClicked(int TabIndex);
		void OnRemoteSessionTerminated(const QString Node);

	signals:
		void ZigDies(void);
		void NewDisplayMessage(const ZigMessage *Msg);
		void SendClicked(const QString Node, const QString Msg);
		void NodeAdded(const QString Node);
		void NewNodeChosen(const QString NodeID);
		void SessionEndRequested(const QString NodeID);

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

	signals:
		void MessageToSend(const QString Node, const QString Msg);
		void RemoteSessionTerminated(const QString Node);
	public:
		LuaDelegate(ZigMainWindow *Win, VLThreads::Thread *ThreadObj, lua_State *State);
	public:
		static LuaDelegate *Fireup(lua_State *State);
		void ProcessQtEvents(void) const;
		
		void RenderDisplayMessage(const ZigMessage *const Msg);
		void AddNode(const VLString &Node);
	};
}

	
