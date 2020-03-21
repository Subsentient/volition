#include "libvolition/include/conation.h"
#include "libvolition/include/utils.h"
#include "node/script.h"
#include "ziggurat.h"

#if defined(WIN32) && defined(STATIC)
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif //WIN32

#include <QtWidgets>

static const char *fakeargv[] = { "vl", nullptr };
static int fakeargc = 1;

Ziggurat::ZigMainWindow *Ziggurat::ZigMainWindow::Instance;


static void RecursiveSetFont(QWidget *const Widgy, const QFont &Font)
{
	Widgy->setFont(Font);

	QList<QWidget*> Children { Widgy->findChildren<QWidget*>() };

	for (QWidget *const Child : Children)
	{
		Child->setFont(Font);
	}
}

void Ziggurat::ZigMainWindow::OnRemoteSessionTerminated(const QString Node_)
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
	
	VLThreads::MutexKeeper Keeper { &this->MessengersLock };

	this->ZigMsgTabs->removeTab(Index);
	this->Messengers.erase(Node);
	
	VLDEBUG("Remote session with node " + Node + " successfully destroyed.");
}

void Ziggurat::ZigMainWindow::OnTabCloseClicked(int TabIndex)
{
	const VLString Node { static_cast<ZigMessengerWidget*>(this->ZigMsgTabs->widget(TabIndex))->GetNode() };

	VLThreads::MutexKeeper Keeper { &this->MessengersLock };

	this->ZigMsgTabs->removeTab(TabIndex);
	this->Messengers.erase(Node);

	emit SessionEndRequested(+Node);
}

void Ziggurat::ZigMainWindow::OnFocusAltered(QWidget *Old, QWidget *Now)
{
	this->HasFocus = Now != nullptr;
	VLDEBUG("Focus updated to " + (this->HasFocus ? "true" : "false"));
}

void Ziggurat::ZigMainWindow::OnLoadFont(const QFont &Selected)
{
	VLDEBUG("Setting font to " + qs2vls(Selected.toString()));
	
	RecursiveSetFont(this, Selected);
}

void Ziggurat::ZigMainWindow::OnFontChooserWanted(void)
{
	QFontDialog *const Dialog = new QFontDialog(this->font(), this);

	QObject::connect(Dialog, &QFontDialog::fontSelected, this,
					[this, Dialog](const QFont &Chosen)
					{
						VLDEBUG("SETTING FONT");
						RecursiveSetFont(this, Chosen);
						
						delete Dialog;
						
						emit this->NewFontSelected(Chosen);
					});

	Dialog->setVisible(true);
}

void *Ziggurat::ZigMainWindow::ThreadFuncInit(void *Waiter_)
{
	VLThreads::ValueWaiter<ZigMainWindow*> *const Waiter = static_cast<VLThreads::ValueWaiter<ZigMainWindow*>*>(Waiter_);

	VLDEBUG("Constructing QApplication");
	QApplication *const App = QApplication::instance() ? (QApplication*)QApplication::instance() : new QApplication { fakeargc, (char**)fakeargv };
	
	QFile SS { ":qdarkstyle/style.qss" };
	
	if (SS.exists() && SS.open(QFile::ReadOnly | QFile::Text))
	{
		App->setStyleSheet(SS.readAll().data());
	}
	
	VLDEBUG("Constructing ZigWin");
	ZigMainWindow *const ZigWin = new ZigMainWindow;

	ZigWin->show();
	ZigWin->setWindowIcon(QIcon(":ziggurat.png"));
	
	QObject::connect(ZigWin, &ZigMainWindow::FontChooserWanted, ZigWin, &ZigMainWindow::OnFontChooserWanted, Qt::ConnectionType::QueuedConnection);
	QObject::connect(ZigWin, &ZigMainWindow::NewDisplayMessage, ZigWin, &ZigMainWindow::OnNewDisplayMessage, Qt::ConnectionType::QueuedConnection);
	QObject::connect(ZigWin, &ZigMainWindow::NodeAdded, ZigWin, &ZigMainWindow::OnNodeAdded, Qt::ConnectionType::QueuedConnection);
	QObject::connect(ZigWin->ZigActChooseFont, &QAction::triggered, ZigWin, &ZigMainWindow::OnFontChooserWanted);
	QObject::connect(ZigWin->ZigActNewNode, &QAction::triggered, ZigWin, &ZigMainWindow::OnNewNodeClicked);
	QObject::connect(ZigWin->ZigMsgTabs, &QTabWidget::tabCloseRequested, ZigWin, &ZigMainWindow::OnTabCloseClicked);
	QObject::connect(ZigWin->ZigActQuit, &QAction::triggered, ZigWin, [] { exit(0); });
	QObject::connect(App, &QApplication::focusChanged, ZigWin, &ZigMainWindow::OnFocusAltered);
	
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
	RecursiveSetFont(Chooser, this->font());
	
	Chooser->show();
}

void Ziggurat::ZigMainWindow::OnNewDisplayMessage(ZigMessage *const Item)
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

void Ziggurat::ZigMainWindow::closeEvent(QCloseEvent*)
{
	exit(0);
}

void Ziggurat::ZigMainWindow::OnNodeAdded(const QString Node)
{
	ZigMessengerWidget *const Widgy = new ZigMessengerWidget(this, qs2vls(Node));

	RecursiveSetFont(Widgy, this->font());
	
	QObject::connect(Widgy, &ZigMessengerWidget::SendClicked, this, &ZigMainWindow::SendClicked);
	QObject::connect(Widgy, &ZigMessengerWidget::NativeMessageReady, this, &ZigMainWindow::NativeMessageReady);
	
	Widgy->show();
	
	this->ZigMsgTabs->addTab(Widgy, Node);
	
	VLThreads::MutexKeeper Keeper { &this->MessengersLock };

	Widgy->ZigMessageEditor->setFocus();

	this->Messengers.emplace(Widgy->GetNode(), Widgy);

}
	
Ziggurat::ZigMainWindow::ZigMainWindow(void) : QMainWindow(), HasFocus()
{
	setupUi(this);
}

void Ziggurat::ZigMessengerWidget::OnEnterPressed(QObject *Object, int Key)
{
	if (Object != this->ZigMessageEditor ||
		(Key != Qt::Key_Enter && Key != Qt::Key_Return))
	{
		return;
	}

	if (this->ZigEnterCheckbox->checkState())
	{ //Enter to send is on
		emit this->ZigSendButton->clicked();
		return;
	}
}

Ziggurat::ZigMessengerWidget::ZigMessengerWidget(QWidget *const Parent, const VLString &Node)
	: QWidget(Parent), Node(Node), KeyMon(new KeyboardMonitor{})
{
	setupUi(this);
	
	this->setFont(Parent->font());
	
	this->KeyMon->setParent(this);
	this->ZigMessageEditor->installEventFilter(this->KeyMon);

	QObject::connect(this->ZigSendButton, &QPushButton::clicked, this,
	[this]
	{
		VLString TempString { qs2vls(this->ZigMessageEditor->toPlainText()) };
		TempString.StripLeading(" \n\r");
		TempString.StripTrailing(" \n\r");
		
		if (this->ZigHTMLCheckbox->checkState())
		{
			emit SendClicked(QString(+this->Node), QString(+TempString).replace("\n", "<br />"));
		}
		else
		{
			emit SendClicked(QString(+this->Node), QString(+TempString).toHtmlEscaped().replace("\n", "<br />"));
		}

		this->ZigMessageEditor->clear();
	}, Qt::ConnectionType::QueuedConnection);

	QObject::connect(this->KeyMon, &KeyboardMonitor::KeyPress, this, &ZigMessengerWidget::OnEnterPressed);
	QObject::connect(this, &ZigMessengerWidget::NewDisplayMessage, this, &ZigMessengerWidget::OnNewDisplayMessage, Qt::ConnectionType::QueuedConnection);
}

bool Ziggurat::KeyboardMonitor::eventFilter(QObject *Object, QEvent *Event)
{
	if (Event->type() != QEvent::KeyPress) return QObject::eventFilter(Object, Event);
	
	QKeyEvent *KeyEvent = static_cast<QKeyEvent*>(Event);
	
	emit this->KeyPress(Object, KeyEvent->key());
	
	return QObject::eventFilter(Object, Event);
}

void Ziggurat::ZigMessengerWidget::OnNewDisplayMessage(ZigMessage *const Item)
{
	QListWidgetItem *const ModelItem = new QListWidgetItem;
	
	///75% max height/width for images
	const int MaxWidth = ZigMessageList->width() - (ZigMessageList->width() / 4);
	const int MaxHeight = ZigMessageList->height() - (ZigMessageList->height() / 4);
	
	QWidget *const MsgWidget = Item->RebuildMsgWidget(MaxWidth, MaxHeight);

	if (!MsgWidget)
	{
		VLERROR("FAILED TO GENERATE WIDGET FOR DISPLAY!");
		return;
	}

	RecursiveSetFont(MsgWidget, this->font());
	
	ModelItem->setSizeHint(MsgWidget->sizeHint());
	
	this->ZigMessageList->addItem(ModelItem);
	this->ZigMessageList->setItemWidget(ModelItem, MsgWidget);
	this->ZigMessageList->verticalScrollBar()->setValue(this->ZigMessageList->verticalScrollBar()->maximum());

	MsgWidget->show();
	
	emit NativeMessageReady(Item);
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
	}, Qt::ConnectionType::QueuedConnection); //We use queued connections here to prevent the below signal-forwarding from causing a segfault.
	
	QObject::connect(this->TextChooserData, &QLineEdit::editingFinished, this->TextChooserAccept,
	[this]
	{
		emit this->TextChooserAccept->clicked();
	});
	
	QObject::connect(this->TextChooserCancel, &QPushButton::clicked, this,
	[this]
	{
		if (!this->DismissCallback) return;
		
		this->DismissCallback(this, this->UserData);
	}, Qt::ConnectionType::QueuedConnection);
	
	this->TextChooserLabel->setText(+PromptText);
	this->setWindowTitle(+WindowTitle);
	
	this->TextChooserData->setFocus();
}
