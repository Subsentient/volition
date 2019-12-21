/**
* This file is part of Volition.

* Volition is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* Volition is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with Volition.  If not, see <https://www.gnu.org/licenses/>.
**/


#include "../libvolition/include/common.h"
#include "../libvolition/include/conation.h"

#include <set>
#include <map>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "gui_mainwindow.h"
#include "gui_base.h"
#include "gui_menus.h"
#include "gui_dialogs.h"
#include "gui_icons.h"
#include "scriptscanner.h"
#include "clienttracker.h"
#include "main.h"
#include "interface.h"
#include "ticker.h"

static GdkPixbuf *GetPlatformIcon(const ClientTracker::ClientRecord &Ref);
static void SelectedNodesForEachFunc(GtkTreeModel *Model, GtkTreePath *Path, GtkTreeIter *Iter, gpointer UserData);

static std::map<uint64_t, GuiDialogs::DialogBase*> MsgInfoDialogs;

static GuiDialogs::AboutDialog *StaticAboutDialog;

GuiMainWindow::MainWindowScreen::MainWindowScreen(void)
	: ScreenObj(gtk_window_new(GTK_WINDOW_TOPLEVEL), ScreenType::MAINWINDOW),
	VBox(gtk_box_new(GTK_ORIENTATION_VERTICAL, 4)),
	NodeScrolledWindow(gtk_scrolled_window_new(nullptr, nullptr)),
	TickerScrolledWindow(gtk_scrolled_window_new(nullptr, nullptr)),
	VerticalPane(gtk_paned_new(GTK_ORIENTATION_VERTICAL)),
	AccelGroup(gtk_accel_group_new()),
	MenuBar(gtk_menu_bar_new()),
	FileTag(gtk_menu_item_new_with_mnemonic("_File")),
	FileMenu(gtk_menu_new()),
	QuitItem(gtk_menu_item_new_with_mnemonic("_Quit")),
	RefreshItem(gtk_menu_item_new_with_mnemonic("_Refresh")),
	OrdersTag(gtk_menu_item_new_with_mnemonic("_Orders")),
	OrdersMenu(gtk_menu_new()),
	ServerTag(gtk_menu_item_new_with_mnemonic("_Server")),
	ServerMenu(gtk_menu_new()),
	HelpTag(gtk_menu_item_new_with_mnemonic("_Help")),
	HelpMenu(gtk_menu_new()),
	AboutItem(gtk_menu_item_new_with_mnemonic("_About")),
	NodeTreeView(gtk_tree_view_new()),
	NodeTreeStore(gtk_tree_store_new(MAINWINDOW_NODETREE_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
								G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING)),
	TickerTreeView(gtk_tree_view_new()),
	TickerTreeStore(gtk_tree_store_new(MAINWINDOW_TICKERTREE_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING)),
	StatusBar(gtk_statusbar_new()),
	StatusBarContextID(gtk_statusbar_get_context_id((GtkStatusbar*)this->StatusBar, "global")),
	NodeGroupBranches(),
	UnsortedNodeGroup()
{
	//Add VBox to window
	gtk_container_add((GtkContainer*)this->Window, this->VBox);

	//Set window parameters
	gtk_window_set_default_size((GtkWindow*)this->Window, 1024, 620);
	gtk_window_set_title((GtkWindow*)this->Window, "Volition Control");
	gtk_container_set_border_width((GtkContainer*)this->Window, 2);
	
	//This is NOT newly allocated and should not be deleted.
	const std::vector<uint8_t> &WindowIcon = GuiIcons::LookupIcon("ctlwmicon");
	
	gtk_window_set_icon((GtkWindow*)this->Window, GuiBase::LoadImageToPixbuf(WindowIcon.data(), WindowIcon.size()));
	
	//Setup menus
	gtk_menu_item_set_submenu((GtkMenuItem*)FileTag, this->FileMenu);
	gtk_menu_item_set_submenu((GtkMenuItem*)OrdersTag, this->OrdersMenu);
	gtk_menu_item_set_submenu((GtkMenuItem*)ServerTag, this->ServerMenu);
	gtk_menu_item_set_submenu((GtkMenuItem*)HelpTag, this->HelpMenu);


	//Make F5 give us refresh.
	gtk_widget_add_accelerator(this->RefreshItem, "activate", AccelGroup, GDK_KEY_F5, GdkModifierType(0), GTK_ACCEL_VISIBLE);
	
	gtk_menu_shell_append((GtkMenuShell*)this->FileMenu, this->RefreshItem);
	
	gtk_menu_shell_append((GtkMenuShell*)this->FileMenu, this->QuitItem);
	
	gtk_menu_shell_append((GtkMenuShell*)this->HelpMenu, this->AboutItem);

	//Append submenus to menu bar.
	gtk_menu_shell_append((GtkMenuShell*)this->MenuBar, this->FileTag);
	gtk_menu_shell_append((GtkMenuShell*)this->MenuBar, this->OrdersTag);
	gtk_menu_shell_append((GtkMenuShell*)this->MenuBar, this->ServerTag);
	gtk_menu_shell_append((GtkMenuShell*)this->MenuBar, this->HelpTag);

	///Compile list of menu items for orders menu.
	GuiMenus::PopulateMWNodeTreeMenus(this->OrdersMenu);
	GuiMenus::PopulateMWServerMenus(this->ServerMenu);
	
	//Accelerator
	gtk_window_add_accel_group((GtkWindow*)this->Window, this->AccelGroup);

	//Pack into the vbox
	gtk_box_pack_start((GtkBox*)this->VBox, this->MenuBar, false, false, 0);
	gtk_box_pack_start((GtkBox*)this->VBox, this->VerticalPane, true, true, 2);
	gtk_box_pack_start((GtkBox*)this->VBox, this->StatusBar, false, false, 0);

	//Scrolled windows
	gtk_scrolled_window_set_policy((GtkScrolledWindow*)this->NodeScrolledWindow, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_policy((GtkScrolledWindow*)this->TickerScrolledWindow, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	//Add scrolled windows to pane.
	gtk_paned_add1((GtkPaned*)this->VerticalPane, this->NodeScrolledWindow);
	gtk_paned_add2((GtkPaned*)this->VerticalPane, this->TickerScrolledWindow);
	
	//Icon panel
	gtk_container_add((GtkContainer*)this->NodeScrolledWindow, this->NodeTreeView);
	gtk_container_add((GtkContainer*)this->TickerScrolledWindow, this->TickerTreeView);
	
	gtk_widget_set_vexpand(this->TickerTreeView, true);
	gtk_widget_set_vexpand(this->TickerScrolledWindow, true);
	
	//Signals
	g_signal_connect((GObject*)this->Window, "destroy", (GCallback)GuiBase::ShutdownCallback, nullptr);
	g_signal_connect((GObject*)this->Window, "window-state-event", (GCallback)GuiMainWindow::MainWindowScreen::WindowResizeCallback, this);
	g_signal_connect((GObject*)this->QuitItem, "activate", (GCallback)GuiBase::ShutdownCallback, nullptr);
	g_signal_connect_swapped((GObject*)this->RefreshItem, "activate", (GCallback)GuiMainWindow::MainWindowScreen::RefreshCallback, this);

	void *(*AboutLaunchFunc)(void*) = [](void*)->void*
	{
		if (StaticAboutDialog) delete StaticAboutDialog;
		StaticAboutDialog = new GuiDialogs::AboutDialog();
		StaticAboutDialog->Show();
		return nullptr;
	};
	
	g_signal_connect((GObject*)this->AboutItem, "activate", (GCallback)AboutLaunchFunc, nullptr);
	

	this->InitNodeTreeView();
	this->InitTickerTreeView();
	
	this->LoadNodeTree();
	this->LoadTickerTree();
} 

void GuiMainWindow::MainWindowScreen::AddUnsortedNodeGroup(void)
{
	this->UnsortedNodeGroup = new GtkTreeIter();
	
	gtk_tree_store_append(this->NodeTreeStore, this->UnsortedNodeGroup, nullptr);
	gtk_tree_store_set(this->NodeTreeStore, this->UnsortedNodeGroup, MWNodeColumns::NAME,
						"<b><span foreground=\"#ff5500\">Unsorted nodes</span></b>", -1); //Just set the name.
}

void GuiMainWindow::MainWindowScreen::LoadNodeTree(void)
{
	this->NodeGroupBranches.clear();
	
	delete this->UnsortedNodeGroup;
	this->UnsortedNodeGroup = nullptr;
	
	gtk_tree_store_clear(this->NodeTreeStore);
	
	if (ClientTracker::GroupHasMember(VLString()))
	{
		this->AddUnsortedNodeGroup();
	}
	
	for (ClientTracker::RecordIter Iter = ClientTracker::GetIterator(); Iter; ++Iter)
	{
		this->AddNodeToTree(&*Iter);
	}

	gtk_tree_view_set_model((GtkTreeView*)this->NodeTreeView, (GtkTreeModel*)this->NodeTreeStore);
	gtk_tree_view_expand_all((GtkTreeView*)this->NodeTreeView);
}

GtkTreeIter *GuiMainWindow::MainWindowScreen::FindMsgInTickerPane(uint64_t MsgNumber)
{
	GtkTreeIter Start{};

	if (!gtk_tree_model_get_iter_first((GtkTreeModel*)this->TickerTreeStore, &Start))
	{
		return nullptr;
	}

	return FindMsgInTickerPaneSub(MsgNumber, Start);
}

GtkTreeIter *GuiMainWindow::MainWindowScreen::FindMsgInTickerPaneSub(uint64_t MsgNumber, const GtkTreeIter &Start)
{
	GtkTreeIter Iter = Start;

	do
	{

		if (gtk_tree_model_iter_has_child((GtkTreeModel*)this->TickerTreeStore, &Iter))
		{
			GtkTreeIter Child;
			gtk_tree_model_iter_children((GtkTreeModel*)this->TickerTreeStore, &Child, &Iter);

			GtkTreeIter *Check = FindMsgInTickerPaneSub(MsgNumber, Child);

			if (Check) return Check;

		}
		
		char *Buf{};
		
		gtk_tree_model_get((GtkTreeModel*)this->TickerTreeStore, &Iter, MWTickerColumns::MSG_NUMBER, &Buf, -1);

		VLString ID = Buf;
		g_free(Buf);
		
		if (strtoull(ID, nullptr, 10) == MsgNumber)
		{
			return new GtkTreeIter(Iter);
		}
	}
	while (gtk_tree_model_iter_next((GtkTreeModel*)this->TickerTreeStore, &Iter));


	return nullptr;
}

void GuiMainWindow::MainWindowScreen::LoadTickerTree(void)
{
	Ticker::Iterator Iter = Ticker::GetIterator();

	GtkTreeIter TreeIter{};

	VLString TimeFormat, Summary;

	GtkTreeIter *ExpandAfter = nullptr;
	
	for (; Iter; ++Iter)
	{
		//Already exists
		if (this->DisplayedTickerEntries.count(Iter->MsgNumber)) continue;
		
		if (Iter->OriginType == Ticker::ORIGIN_NODE)
		{
			if (!Iter->CmdIdent) //Unsolicited data, don't give it a group, just add it.
			{
				Ticker::BuildNodeMessageSummary(Iter, TimeFormat, Summary);

				gtk_tree_store_append(this->TickerTreeStore, &TreeIter, nullptr);
				goto SetEntryValue;
			}

			//It's part of an order we sent.

			if (!NodeMessageRoots.count(Iter->CmdIdent))
			{ //We need to create a new leaf for this order.
				gtk_tree_store_append(this->TickerTreeStore, &this->NodeMessageRoots[Iter->CmdIdent], nullptr);
				Ticker::BuildNodeRootMessageSummary(Iter, Summary);

				gtk_tree_store_set( this->TickerTreeStore, &this->NodeMessageRoots[Iter->CmdIdent], MWTickerColumns::SUMMARY, +Summary, -1);
				ExpandAfter = &this->NodeMessageRoots[Iter->CmdIdent];
			}

			gtk_tree_store_append(this->TickerTreeStore, &TreeIter, &this->NodeMessageRoots[Iter->CmdIdent]);

			Ticker::BuildNodeMessageSummary(Iter, TimeFormat, Summary);

			goto SetEntryValue;
		}

		if (Iter->OriginType == Ticker::ORIGIN_SERVER)
		{
			Ticker::BuildServerMessageSummary(Iter, TimeFormat, Summary);
		}
		else
		{
			Ticker::BuildSystemMessageSummary(Iter, TimeFormat, Summary);
		}

		gtk_tree_store_append(this->TickerTreeStore, &TreeIter, nullptr);

SetEntryValue:
		VLString MsgNumber = VLString::UintToString(Iter->MsgNumber);
		
		gtk_tree_store_set( this->TickerTreeStore,
				&TreeIter,
				MWTickerColumns::MSG_NUMBER,
				+MsgNumber,
				MWTickerColumns::TIME,
				+TimeFormat,
				MWTickerColumns::SUMMARY,
				+Summary,
				-1);
		this->DisplayedTickerEntries[Iter->MsgNumber] = TreeIter;

		if (ExpandAfter)
		{
			GtkTreePath *Path = gtk_tree_model_get_path((GtkTreeModel*)this->TickerTreeStore, ExpandAfter);

			if (Path)
			{
				gtk_tree_view_expand_row((GtkTreeView*)this->TickerTreeView, Path, TRUE);
				gtk_tree_path_free(Path);
			}

			ExpandAfter = nullptr;
		}
		
	}
}

bool GuiMainWindow::MainWindowScreen::AddGroupToTree(const char *GroupName)
{
	if (!this->NodeTreeStore) return false;
	
	gtk_tree_store_append(this->NodeTreeStore, &this->NodeGroupBranches[GroupName], nullptr);
	gtk_tree_store_set(this->NodeTreeStore, &this->NodeGroupBranches[GroupName], MWNodeColumns::NAME,
						+(VLString("<b><span foreground=\"#00AAEE\">Group: ") + GroupName + "</span></b>"), -1); //Just set the name.

	return true;
}

bool GuiMainWindow::MainWindowScreen::AddNodeToTree(const ClientTracker::ClientRecord *Record, const bool ForceGroupCheck)
{
	if (this->NodeTreeStore == nullptr) return false;

	GtkTreeIter TreeIter{};

	bool Existing = false;
	//Check if it already exists, and update it instead of add if it does.

	GtkTreeIter *Lookup = this->FindNodeInTree(Record->ID);
	
	if (Lookup)
	{
		if (ForceGroupCheck)
		{
			gtk_tree_store_remove(this->NodeTreeStore, Lookup);
			

			delete Lookup;
		}
		else
		{
			TreeIter = *Lookup;

			delete Lookup;
		
			Existing = true;
			goto UpdateExisting;
		}
	}
	
RestartGroupCheckLoop:
	//Check for empty groups.
	for (auto Iter = this->NodeGroupBranches.begin(); Iter != this->NodeGroupBranches.end(); ++Iter)
	{
		if (!ClientTracker::GroupHasMember(Iter->first))
		{
			gtk_tree_store_remove(this->NodeTreeStore, &Iter->second);
			this->NodeGroupBranches.erase(VLString(Iter->first));
			goto RestartGroupCheckLoop;
		}
	}
	
	//Do we still need the unsorted group?
	if (!ClientTracker::GroupHasMember(VLString()))
	{
		gtk_tree_store_remove(this->NodeTreeStore, this->UnsortedNodeGroup);
		delete this->UnsortedNodeGroup;
		this->UnsortedNodeGroup = nullptr;
	}
	
	if (Record->Group && !this->NodeGroupBranches.count(Record->Group))
	{ //Need a new root branch.
		this->AddGroupToTree(Record->Group);
	}
	
	if (!Record->Group && !this->UnsortedNodeGroup)
	{
		this->AddUnsortedNodeGroup();
	}
	
	gtk_tree_store_append(this->NodeTreeStore, &TreeIter, Record->Group ? &this->NodeGroupBranches[Record->Group] : this->UnsortedNodeGroup);

UpdateExisting:
	;
	//Build some crap we need.
	char CTBuf[256];
	char PingBuf[128];
	const VLString &StateString = Record->Online ? "<b><span foreground=\"#00dd00\">Online</span></b>"
									: "<span foreground=\"#dd0000\">Offline</span>";
	
	snprintf(PingBuf, sizeof PingBuf, "%dms", (int)Record->LastPingLatency);
	
	struct tm TimeBuf = *localtime(&Record->ConnectedTime);
	
	strftime(CTBuf, sizeof CTBuf, "%Y-%m-%d %I:%M:%S %p", &TimeBuf);
	
	gtk_tree_store_set(this->NodeTreeStore, &TreeIter,
						MWNodeColumns::ICON, GetPlatformIcon(*Record),
						MWNodeColumns::NAME, +Record->ID,
						MWNodeColumns::PLATFORM, +Record->PlatformString,
						MWNodeColumns::REVISION, +Record->NodeRevision,
						MWNodeColumns::IPADDR, +Record->IPAddr,
						MWNodeColumns::PINGLATENCY, Record->Online ? PingBuf : "",
						MWNodeColumns::STATE, +StateString,
						MWNodeColumns::CONNECT_TIME, CTBuf,
						-1);
	
	if (!Existing)
	{
		GtkTreePath *Path = gtk_tree_model_get_path((GtkTreeModel*)this->NodeTreeStore, Record->Group ? &this->NodeGroupBranches[Record->Group] : this->UnsortedNodeGroup);
		if (Path)
		{
			gtk_tree_view_expand_row((GtkTreeView*)this->NodeTreeView, Path, TRUE);
			gtk_tree_path_free(Path);
		}		
	}
	return true;
}

GtkTreeIter *GuiMainWindow::MainWindowScreen::FindNodeInTree(const char *NodeID)
{ //Returns one allocated on the heap, or nullptr if not found.
	std::vector<GtkTreeIter> Groups;
	Groups.reserve(this->NodeGroupBranches.size() + 1);

	if (this->UnsortedNodeGroup) Groups.push_back(*this->UnsortedNodeGroup);
	
	for (auto Iter = this->NodeGroupBranches.begin(); Iter != this->NodeGroupBranches.end(); ++Iter)
	{
		Groups.push_back(Iter->second);
	}

	///Now we have what we need to proceed.

	for (size_t Inc = 0u; Inc < Groups.size(); ++Inc)
	{ //For each group
		if (!gtk_tree_model_iter_has_child((GtkTreeModel*)this->NodeTreeStore, &Groups[Inc])) continue;

		GtkTreeIter ChildIter{};
		gtk_tree_model_iter_children((GtkTreeModel*)this->NodeTreeStore, &ChildIter, &Groups[Inc]);

		do
		{
			char *Buf{};
			
			gtk_tree_model_get((GtkTreeModel*)this->NodeTreeStore, &ChildIter, MWNodeColumns::NAME, &Buf, -1);

			VLString ID = Buf;
			g_free(Buf);

			if (ID == NodeID)
			{
				return new GtkTreeIter(ChildIter);
			}
		}
		while (gtk_tree_model_iter_next((GtkTreeModel*)this->NodeTreeStore, &ChildIter));
	}

	return nullptr;
		
}
	
void GuiMainWindow::MainWindowScreen::InitNodeTreeView(void)
{
	static VLString ColumnNames[MAINWINDOW_NODETREE_COLUMNS] = { "Icon", "ID", "Platform", "Revision", "IP", "Ping", "State", "Last Connected" };
	const char *ColumnTypes[MAINWINDOW_NODETREE_COLUMNS] = { "pixbuf", "markup", "text", "text", "text", "text", "markup", "text" };
	const gboolean ColumnExpands[MAINWINDOW_NODETREE_COLUMNS] = { FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE };
	GtkCellRenderer *Renderer{};
	GtkTreeViewColumn *Column{};
	
	for (size_t Inc = 0; Inc < sizeof ColumnNames / sizeof *ColumnNames; ++Inc)
	{
		Renderer = VLString(ColumnTypes[Inc]) == "pixbuf" ? gtk_cell_renderer_pixbuf_new() : gtk_cell_renderer_text_new();
		
		Column = gtk_tree_view_column_new_with_attributes(ColumnNames[Inc], Renderer, ColumnTypes[Inc], Inc, nullptr);
		gtk_tree_view_column_set_expand(Column, ColumnExpands[Inc]);
		gtk_tree_view_append_column((GtkTreeView*)this->NodeTreeView, Column);
	}

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection((GtkTreeView*)this->NodeTreeView), GTK_SELECTION_MULTIPLE);

	gtk_tree_view_columns_autosize((GtkTreeView*)this->NodeTreeView);
	gtk_tree_view_set_headers_clickable((GtkTreeView*)this->NodeTreeView, TRUE);
	gtk_tree_view_set_enable_search((GtkTreeView*)this->NodeTreeView, TRUE);
	gtk_tree_view_set_level_indentation((GtkTreeView*)this->NodeTreeView, 0);

	g_object_set((GObject*)this->NodeTreeView, "enable-grid-lines", TRUE, nullptr);
	
	//Attach the context menus to node tree view.
	g_signal_connect((GObject*)this->NodeTreeView, "button-press-event", (GCallback)OnNodeTreeRightClick, this);
	g_signal_connect((GObject*)this->NodeTreeView, "popup-menu", (GCallback)OnNodeTreeContextKey, this);
}

void GuiMainWindow::MainWindowScreen::InitTickerTreeView(void)
{
	static VLString ColumnNames[MAINWINDOW_TICKERTREE_COLUMNS] = { "Summary", "Msg #", "Time"};
	const char *ColumnTypes[MAINWINDOW_TICKERTREE_COLUMNS] = { "markup", "text", "text" };
	const gboolean ColumnExpands[MAINWINDOW_TICKERTREE_COLUMNS] = { TRUE, FALSE, FALSE };

	GtkCellRenderer *Renderer{};
	GtkTreeViewColumn *Column{};
	
	for (size_t Inc = 0; Inc < sizeof ColumnNames / sizeof *ColumnNames; ++Inc)
	{
		Renderer = gtk_cell_renderer_text_new();
		
		Column = gtk_tree_view_column_new_with_attributes(ColumnNames[Inc], Renderer, ColumnTypes[Inc], Inc, nullptr);
		gtk_tree_view_column_set_expand(Column, ColumnExpands[Inc]);
		gtk_tree_view_append_column((GtkTreeView*)this->TickerTreeView, Column);
	}

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection((GtkTreeView*)this->TickerTreeView), GTK_SELECTION_SINGLE);

	gtk_tree_view_columns_autosize((GtkTreeView*)this->TickerTreeView);
	gtk_tree_view_set_headers_clickable((GtkTreeView*)this->TickerTreeView, FALSE);
	gtk_tree_view_set_enable_search((GtkTreeView*)this->TickerTreeView, FALSE);
	gtk_tree_view_set_level_indentation((GtkTreeView*)this->TickerTreeView, 0);

	g_object_set((GObject*)this->TickerTreeView, "enable-grid-lines", TRUE, nullptr);
	g_object_set((GObject*)this->TickerTreeView, "headers-visible", TRUE, nullptr);

	gtk_tree_store_clear((GtkTreeStore*)this->TickerTreeStore);
	gtk_tree_view_set_model((GtkTreeView*)this->TickerTreeView, (GtkTreeModel*)this->TickerTreeStore);

	g_signal_connect_swapped((GObject*)this->TickerTreeView, "size-allocate", (GCallback)GuiMainWindow::MainWindowScreen::TickerAutoscrollCallback, this);
	//g_signal_connect_swapped((GObject*)this->TickerTreeStore, "row-changed", (GCallback)GuiMainWindow::MainWindowScreen::TickerAutoscrollCallback, this);
	g_signal_connect((GObject*)this->TickerTreeView, "row-activated", (GCallback)GuiMainWindow::MainWindowScreen::OnTickerMsgDoubleclick, nullptr);

}
	
static GdkPixbuf *GetPlatformIcon(const ClientTracker::ClientRecord &Ref)
{
	std::vector<VLString> *Breakup = Utils::SplitTextByCharacter(Ref.PlatformString, '.');
	
	//We really shouldn't encounter a time when we need option 2. And don't make PlatformString a reference!
	const VLString PlatformString = Breakup ? Breakup->at(0) : Ref.PlatformString;
	
	delete Breakup;
	
	const std::vector<uint8_t> &Icon = GuiIcons::LookupIcon(PlatformString);
	

	return GuiBase::LoadImageToPixbuf(Icon.data(), Icon.size());
}

static void SelectedNodesForEachFunc(GtkTreeModel *Model, GtkTreePath *Path, GtkTreeIter *Iter, gpointer UserData)
{
	if (gtk_tree_model_iter_has_child(Model, Iter))
	{ //They've selected a group, so they must want to apply this to all of the members.
		GtkTreeIter Children{};
		
		if (!gtk_tree_model_iter_children(Model, &Children, Iter)) return;
			
		do
		{
			SelectedNodesForEachFunc(Model, Path, &Children, UserData);
		} while (gtk_tree_model_iter_next(Model, &Children));
		
		return;
	}

	std::set<VLString> *const Output = static_cast<std::set<VLString>*>(UserData);

	char *Buffer = nullptr;
	gtk_tree_model_get(Model, Iter, GuiMainWindow::MWNodeColumns::NAME, &Buffer, -1);

	if (ClientTracker::Lookup(Buffer))
	{ //Another failsafe check in case we selected an empty group.
		Output->insert(Buffer);
	}

	g_free(Buffer);
}

std::set<VLString> *GuiMainWindow::MainWindowScreen::GetSelectedNodes(void)
{
	GtkTreeSelection *TreeSelection = gtk_tree_view_get_selection((GtkTreeView*)this->NodeTreeView);

	std::set<VLString> *RetVal = new std::set<VLString>;
	
	gtk_tree_selection_selected_foreach(TreeSelection, SelectedNodesForEachFunc, RetVal);

	return RetVal;
}

void GuiMainWindow::MainWindowScreen::RefreshCallback(MainWindowScreen *ThisPtr)
{
	ScriptScanner::ScanScriptsDirectory();
	GuiBase::NukeContainerChildren((GtkContainer*)ThisPtr->OrdersMenu);
	GuiMenus::PopulateMWNodeTreeMenus(ThisPtr->OrdersMenu);
	gtk_widget_show_all(ThisPtr->OrdersMenu);
	Interface::RequestIndex();
}


gboolean GuiMainWindow::MainWindowScreen::OnNodeTreeRightClick(GtkWidget *TreeView, GdkEventButton *Event, MainWindowScreen *ThisPointer)
{
	//Not the right kind
	if (!Event || Event->type != GDK_BUTTON_PRESS || Event->button != 3) return false;

	gtk_menu_popup_at_pointer((GtkMenu*)ThisPointer->OrdersMenu, (GdkEvent*)Event);

	VLScopedPtr<std::set<VLString>* > SelectedList { ThisPointer->GetSelectedNodes() };
	
	const bool ShouldSayHandled = SelectedList->size() > 1;
	
	return ShouldSayHandled;
}

gboolean GuiMainWindow::MainWindowScreen::OnNodeTreeContextKey(GtkWidget *TreeView, MainWindowScreen *ThisPointer)
{

	gtk_menu_popup_at_pointer((GtkMenu*)ThisPointer->OrdersMenu, nullptr);

	return true;
}

void GuiMainWindow::MainWindowScreen::Show(void)
{ //Hideous hack to make sure we can access the correct max position property.
	this->GuiBase::ScreenObj::Show();

	gtk_main_iteration_do(false);

	this->RecalculatePaneProportions();
}

void GuiMainWindow::MainWindowScreen::RecalculatePaneProportions(void)
{
	int MaxPosition = 0;

	g_object_get((GObject*)this->VerticalPane, "max-position", &MaxPosition, nullptr);

#ifdef DEBUG
	printf("GuiMainWindow::MainWindowScreen::RecalculatePaneProportions(): MaxPosition = %d\n", MaxPosition);
#endif
	gtk_paned_set_position((GtkPaned*)this->VerticalPane, MaxPosition / 3 * 2);
}

gboolean GuiMainWindow::MainWindowScreen::WindowResizeCallback(GtkWidget *Window, void *, MainWindowScreen *ThisPointer)
{
	/*I've been told that spinning the main loop inside a signal handler will break things.
	 * That has yet to happen, and despite my best efforts, it doesn't seem feasible to do this
	 * any better or less dangerous way.*/
	gtk_main_iteration_do(false);
	ThisPointer->RecalculatePaneProportions();

	return false;
}

gboolean GuiMainWindow::MainWindowScreen::TickerAutoscrollCallback(MainWindowScreen *ThisPointer, void*, void*)
{
	VLDEBUG("Entering autoscroll callback");
	GtkAdjustment *Coords = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow*)ThisPointer->TickerScrolledWindow);

	gtk_adjustment_set_value(Coords, gtk_adjustment_get_upper(Coords));

	return false;
}

void GuiMainWindow::MainWindowScreen::MsgDetailsDialogCallback(const uint64_t *KeyAddr)
{
	const uint64_t MsgNumber = *KeyAddr;

	//Delete ourselves.

	GuiDialogs::TextDisplayDialog *Us = static_cast<GuiDialogs::TextDisplayDialog*>(MsgInfoDialogs[MsgNumber]);

	MsgInfoDialogs.erase(MsgNumber);

	delete Us; //Goodbye, we're done here.
}

void GuiMainWindow::MainWindowScreen::OnTickerMsgDoubleclick(GtkTreeView *TreeView, GtkTreePath *Path, void *, void *UserData)
{
#ifdef DEBUG
	puts("Entered GuiMainWindow::MainWindowScreen::OnTickerMsgDoubleclick()");
#endif
	GtkTreeModel *Model = gtk_tree_view_get_model(TreeView);
	GtkTreeIter Selected;

	if (!gtk_tree_model_get_iter(Model, &Selected, Path)) return;

	char *Buf = nullptr;

	gtk_tree_model_get(Model, &Selected, MWTickerColumns::MSG_NUMBER, &Buf, -1);

	if (!Buf || !*Buf)
	{
		if (Buf) g_free(Buf);
		return;
	}

	const uint64_t MsgNumber = atoll(Buf);
	const VLString &MsgNumberText = Buf;
	
	g_free(Buf);

	const Ticker::TickerMessage *Msg = Ticker::LookupMsg(MsgNumber);

	if (!Msg) return;

	const VLString &WindowTitle = VLString("Message #") + MsgNumberText + " response";
	VLString LabelText;

	switch (Msg->OriginType)
	{
		case Ticker::ORIGIN_NODE:
			LabelText = VLString("Expanded details for ") + CommandCodeToString(Msg->CmdCode) + " command sent\nto node \"" + Msg->Origin + "\":";
			break;
		case Ticker::ORIGIN_SERVER:
			LabelText = "Expanded details for server message:";
			break;
		case Ticker::ORIGIN_SYSTEM:
			LabelText = "Expanded details for system message:";
			break;
		default:
			break;
	}
	
	MsgInfoDialogs[MsgNumber] = nullptr; //Auto-vivify so we have an address.
	const uint64_t *KeyAddr = &MsgInfoDialogs.find(MsgNumber)->first; //Now we can store that address to pass to our own function.

	const VLString &BoxText = Msg->Expanded ? Msg->Expanded : VLString("No further information provided for this message.");
	
	//Create the dialog now and store it.
	MsgInfoDialogs[MsgNumber] = new GuiDialogs::TextDisplayDialog(WindowTitle, LabelText, BoxText, (GuiDialogs::TextDisplayDialog::OnCompleteFunc)MsgDetailsDialogCallback, KeyAddr);

	MsgInfoDialogs[MsgNumber]->Show();
}

void GuiMainWindow::MainWindowScreen::Set_Clickable(const bool Value)
{
	gtk_widget_set_sensitive((GtkWidget*)this->MenuBar, Value);
	gtk_widget_set_sensitive((GtkWidget*)this->NodeTreeView, Value);
	gtk_widget_set_sensitive((GtkWidget*)this->TickerTreeView, Value);
}

void GuiMainWindow::MainWindowScreen::SetStatusBarText(const char *Text)
{
	gtk_statusbar_remove_all((GtkStatusbar*)this->StatusBar, this->StatusBarContextID);
	
	if (!Text) return;
	
	gtk_statusbar_push((GtkStatusbar*)this->StatusBar, this->StatusBarContextID, Text);
}
