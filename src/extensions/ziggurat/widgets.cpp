#include "libvolition/include/conation.h"
#include "libvolition/include/utils.h"
#include "node/script.h"
#include "ziggurat.h"


#include <QtWidgets>

static const char *fakeargv[] = { "vl", nullptr };
static int fakeargc = 1;

Ziggurat::ZigMainWindow *Ziggurat::ZigMainWindow::Instance;


void Ziggurat::ZigMainWindow::OnRemoteSessionTerminated(const QString &Node_)
{
	const VLString Node { qs2vls(Node_) };
	
	if (!this->Messengers.count(Node))
	{
		VLWARN("Signal emitted when tab is already dead.");
		return;
	}
	
	const int Index = this->ZigMsgTabs->indexOf(this->Messengers.at(Node));
	
	if (Index < 0)
	{
		VLWARN("Widget found but no index in tabs!");
		return;
	}
	
	this->ZigMsgTabs->removeTab(Index);
	this->Messengers.erase(Node);
	
	VLDEBUG("Remote session with node " + Node + " successfully destroyed.");
}

void Ziggurat::ZigMainWindow::OnTabCloseClicked(int TabIndex)
{
	const VLString Node { static_cast<ZigMessengerWidget*>(this->ZigMsgTabs->widget(TabIndex))->GetNode() };
	
	this->Messengers.erase(Node);
	this->ZigMsgTabs->removeTab(TabIndex);
	
	emit SessionEndRequested(+Node);
}

void *Ziggurat::ZigMainWindow::ThreadFuncInit(void *Waiter_)
{
	VLThreads::ValueWaiter<ZigMainWindow*> *const Waiter = static_cast<VLThreads::ValueWaiter<ZigMainWindow*>*>(Waiter_);

	VLDEBUG("Constructing QApplication");
	if (!QApplication::instance()) new QApplication { fakeargc, (char**)fakeargv };
	
	VLDEBUG("Constructing ZigWin");
	ZigMainWindow *const ZigWin = new ZigMainWindow;

	ZigWin->show();
	ZigWin->setWindowIcon(QIcon(":ziggurat.png"));
	
	QObject::connect(ZigWin, &ZigMainWindow::NewDisplayMessage, ZigWin, &ZigMainWindow::OnNewDisplayMessage, Qt::ConnectionType::QueuedConnection);
	QObject::connect(ZigWin, &ZigMainWindow::NodeAdded, ZigWin, &ZigMainWindow::OnNodeAdded, Qt::ConnectionType::QueuedConnection);
	QObject::connect(ZigWin->ZigActNewNode, &QAction::triggered, ZigWin, &ZigMainWindow::OnNewNodeClicked);
	QObject::connect(ZigWin->ZigMsgTabs, &QTabWidget::tabCloseRequested, ZigWin, &ZigMainWindow::OnTabCloseClicked);
	QObject::connect(ZigWin->ZigActQuit, &QAction::triggered, ZigWin, [] { exit(0); });
	
	VLDEBUG("Sending ZigWin");
	Waiter->Post(ZigWin);
	
	VLDEBUG("Executing main loop");
	
	ZigWin->ThreadFunc();
	
	return nullptr;
}

void Ziggurat::ZigMainWindow::OnNewNodeClicked(void)
{
	ZigTextChooser::TextChooserCallback DismissCB = [] (ZigTextChooser *const Chooser, void*) { delete Chooser; };
	ZigTextChooser::TextChooserCallback AcceptCB
	{
		[] (ZigTextChooser *const Chooser, void *UserData)
		{
			emit static_cast<ZigMainWindow*>(UserData)->NewNodeChosen(+Chooser->GetValue());
			delete Chooser;
		}
	};
	
	ZigTextChooser *const Chooser = new ZigTextChooser("Enter Node ID", "Enter a node ID to open a chat session to.", AcceptCB, DismissCB, this);

	Chooser->show();
}

void Ziggurat::ZigMainWindow::OnNewDisplayMessage(const ZigMessage *const Item)
{
	VLThreads::MutexKeeper Keeper { &this->MessengersLock };
	
	if (!this->Messengers.count(Item->GetNode()))
	{
		VLWARN("No target node " + Item->GetNode());
		return;
	}
	
	ZigMessengerWidget *const Messenger = this->Messengers.at(Item->GetNode());
	
	Keeper.Unlock(); //Just in case to prevent deadlock
	
	emit Messenger->NewDisplayMessage(Item);
}

void Ziggurat::ZigMainWindow::ThreadFunc(void)
{	
	QEventLoop Loop;
	
	QObject::connect(this, &ZigMainWindow::ZigDies, this, [&Loop] { Loop.exit(); }, Qt::ConnectionType::QueuedConnection);

	Loop.exec();
}

void Ziggurat::ZigMainWindow::OnNodeAdded(const QString &Node)
{
	ZigMessengerWidget *const Widgy = new ZigMessengerWidget(qs2vls(Node));
	
	QObject::connect(Widgy, &ZigMessengerWidget::SendClicked, this, &ZigMainWindow::SendClicked);
	
	this->Messengers.emplace(Widgy->GetNode(), Widgy);
	Widgy->show();
	
	this->ZigMsgTabs->addTab(Widgy, Node);
}
	
Ziggurat::ZigMainWindow::ZigMainWindow(void) : QMainWindow()
{
	setupUi(this);
}

Ziggurat::ZigMessengerWidget::ZigMessengerWidget(const VLString &Node)
	: Node(Node)
{
	setupUi(this);

	QObject::connect(this->ZigSendButton, &QPushButton::clicked, this,
	[this]
	{
		emit SendClicked(QString(+this->Node), this->ZigMessageEditor->toPlainText());
		this->ZigMessageEditor->clear();
	});

	QObject::connect(this, &ZigMessengerWidget::NewDisplayMessage, this, &ZigMessengerWidget::OnNewDisplayMessage, Qt::ConnectionType::QueuedConnection);
}


void Ziggurat::ZigMessengerWidget::OnNewDisplayMessage(const ZigMessage *const Item)
{
	QListWidgetItem *const ModelItem = new QListWidgetItem;

	QWidget *const MsgWidget = Item->GetMsgWidget();
	
	MsgWidget->setParent(this->ZigMessageList);
	MsgWidget->show();

	this->ZigMessageList->addItem(ModelItem);
	this->ZigMessageList->setItemWidget(ModelItem, MsgWidget);
}

Ziggurat::ZigTextChooser::ZigTextChooser(const VLString &WindowTitle,
										const VLString &PromptText,
										const TextChooserCallback AcceptCallback,
										const TextChooserCallback DismissCallback,
										void *UserData)
	: DismissCallback(DismissCallback),
	AcceptCallback(AcceptCallback),
	UserData(UserData)
{
	setupUi(this);
	
	QObject::connect(this->TextChooserAccept, &QPushButton::clicked, this,
	[this]
	{
		if (!this->AcceptCallback) return;
		
		this->AcceptCallback(this, this->UserData);
	});
	
	QObject::connect(this->TextChooserCancel, &QPushButton::clicked, this,
	[this]
	{
		if (!this->DismissCallback) return;
		
		this->DismissCallback(this, this->UserData);
	});
	
	this->TextChooserLabel->setText(+PromptText);
	this->setWindowTitle(+WindowTitle);
}
