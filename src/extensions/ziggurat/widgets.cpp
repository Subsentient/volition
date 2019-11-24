#include "libvolition/include/conation.h"
#include "libvolition/include/utils.h"
#include "node/script.h"
#include "ziggurat.h"


#include <QtWidgets>

static const char *fakeargv[] = { "vl", nullptr };
static int fakeargc = 1;

Ziggurat::ZigMainWindow *Ziggurat::ZigMainWindow::Instance;

void *Ziggurat::ZigMainWindow::ThreadFuncInit(void *Waiter_)
{
	VLThreads::ValueWaiter<ZigMainWindow*> *const Waiter = static_cast<VLThreads::ValueWaiter<ZigMainWindow*>*>(Waiter_);

	VLDEBUG("Constructing QApplication");
	if (!QApplication::instance()) new QApplication { fakeargc, (char**)fakeargv };
	
	VLDEBUG("Constructing ZigWin");
	ZigMainWindow *const ZigWin = new ZigMainWindow;

	ZigWin->show();
	ZigWin->setWindowIcon(QIcon(":ziggurat.png"));
	
	VLDEBUG("Sending ZigWin");
	Waiter->Post(ZigWin);
	
	VLDEBUG("Executing main loop");
	
	ZigWin->ThreadFunc();
	
	return nullptr;
}

void Ziggurat::ZigMainWindow::OnNewIncomingMessage(const ZigMessage *const Item)
{
	VLThreads::MutexKeeper Keeper { &this->MessengersLock };
	
	if (!this->Messengers.count(Item->GetTargetNode()))
	{
		VLWARN("No target node " + Item->GetTargetNode());
		return;
	}
	
	ZigMessengerWidget *const Messenger = this->Messengers.at(Item->GetTargetNode());
	
	Keeper.Unlock(); //Just in case to prevent deadlock
	
	emit Messenger->NewIncomingMessage(Item);
}

void Ziggurat::ZigMainWindow::ThreadFunc(void)
{	
	QEventLoop Loop;
	
	QObject::connect(this, &ZigMainWindow::ZigDies, this, [&Loop] { Loop.exit(); }, Qt::ConnectionType::QueuedConnection);
	QObject::connect(this, &ZigMainWindow::NewIncomingMessage, this, &ZigMainWindow::OnNewIncomingMessage, Qt::ConnectionType::QueuedConnection);
	QObject::connect(this, &ZigMainWindow::NodeAdded, this, &ZigMainWindow::OnNodeAdded, Qt::ConnectionType::QueuedConnection);
	
	Loop.exec();
}

void Ziggurat::ZigMainWindow::OnNodeAdded(const QString &TargetNode)
{
	ZigMessengerWidget *const Widgy = new ZigMessengerWidget(qs2vls(TargetNode));
	
	QObject::connect(Widgy, &ZigMessengerWidget::SendClicked, this, &ZigMainWindow::SendClicked);
	
	this->Messengers.emplace(Widgy->GetTargetNode(), Widgy);
	Widgy->show();
	
	this->ZigMsgTabs->addTab(Widgy, TargetNode);
}
	
Ziggurat::ZigMainWindow::ZigMainWindow(void) : QMainWindow()
{
	setupUi(this);
}

Ziggurat::ZigMessengerWidget::ZigMessengerWidget(const VLString &TargetNode)
	: TargetNode(TargetNode)
{
	setupUi(this);

	QObject::connect(this->ZigSendButton, &QPushButton::clicked, this,
	[this]
	{
		emit SendClicked(QString(+this->TargetNode), this->ZigMessageEditor->toPlainText());
		this->ZigMessageEditor->clear();
	});

	QObject::connect(this, &ZigMessengerWidget::NewIncomingMessage, this, &ZigMessengerWidget::OnNewIncomingMessage, Qt::ConnectionType::QueuedConnection);
}


void Ziggurat::ZigMessengerWidget::OnNewIncomingMessage(const ZigMessage *const Item)
{
	VLThreads::MutexKeeper Keeper { &this->InLock };
	
	this->IncomingMessageList.emplace_back(Item);
	
	QListWidgetItem *const ModelItem = new QListWidgetItem;

	QWidget *const MsgWidget = Item->GetMsgWidget();
	
	MsgWidget->setParent(this->ZigMessageList);
	MsgWidget->show();

	this->ZigMessageList->addItem(ModelItem);
	this->ZigMessageList->setItemWidget(ModelItem, MsgWidget);
}
