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


#ifndef VL_CTL_GUIMAINWINDOW_H
#define VL_CTL_GUIMAINWINDOW_H

#include "../libvolition/include/common.h"
#include "../libvolition/include/conation.h"
#include "gui_base.h"
#include "clienttracker.h"
#include <map>

#define MAINWINDOW_NODETREE_COLUMNS 8
#define MAINWINDOW_TICKERTREE_COLUMNS 3

namespace GuiMainWindow
{
	enum class MWNodeColumns
	{
		ICON = 0,
		NAME,
		PLATFORM,
		REVISION,
		IPADDR,
		PINGLATENCY,
		STATE,
		CONNECT_TIME,
	};
	
	enum class MWTickerColumns
	{
		SUMMARY,
		MSG_NUMBER,
		TIME,
	};
	
	class MainWindowScreen : public GuiBase::ScreenObj
	{
	private:
		GtkWidget *VBox;
		GtkWidget *NodeScrolledWindow;
		GtkWidget *TickerScrolledWindow;
		GtkWidget *VerticalPane;
		GtkAccelGroup *AccelGroup;
		GtkWidget *MenuBarAlign;
		GtkWidget *MenuBar;
		GtkWidget *FileTag;
		GtkWidget *FileMenu;
		GtkWidget *QuitItem;
		GtkWidget *RefreshItem;
		GtkWidget *OrdersTag;
		GtkWidget *OrdersMenu;
		GtkWidget *ServerTag;
		GtkWidget *ServerMenu;
		GtkWidget *HelpTag;
		GtkWidget *HelpMenu;
		GtkWidget *AboutItem;
		GtkWidget *NodeTreeView;
		GtkTreeStore *NodeTreeStore;
		GtkWidget *TickerTreeView;
		GtkTreeStore *TickerTreeStore;
		GtkWidget *StatusBar;
		guint StatusBarContextID;
		std::map<VLString, GtkTreeIter> NodeGroupBranches;
		GtkTreeIter *UnsortedNodeGroup;
		std::map<uint64_t, GtkTreeIter> NodeMessageRoots;
		std::map<uint64_t, GtkTreeIter> DisplayedTickerEntries;
		
		MainWindowScreen(const MainWindowScreen&);
		MainWindowScreen &operator=(const MainWindowScreen&);
		void InitNodeTreeView(void);
		void InitTickerTreeView(void);
		void RecalculatePaneProportions(void);

		GtkTreeIter *FindMsgInTickerPaneSub(uint64_t MsgNumber, const GtkTreeIter &Start);		
	public:
		MainWindowScreen(void);
		void LoadNodeTree(void);
		void LoadTickerTree(void);
		bool AddNodeToTree(const ClientTracker::ClientRecord *Record, const bool ForceGroupCheck = false);
		bool AddGroupToTree(const char *GroupName);
		void AddUnsortedNodeGroup(void);
		GtkTreeIter *FindNodeInTree(const char *NodeID);
		GtkTreeIter *FindMsgInTickerPane(uint64_t MsgNumber);
		
		std::vector<VLString> *GetSelectedNodes(void);
		static void RefreshCallback(MainWindowScreen *Ptr);
		static gboolean OnNodeTreeRightClick(GtkWidget *TreeView, GdkEventButton *Event, MainWindowScreen *ThisPointer);
		static gboolean OnNodeTreeContextKey(GtkWidget *TreeView, MainWindowScreen *ThisPointer);
		static void OnTickerMsgDoubleclick(GtkTreeView *TreeView, GtkTreePath *Path, void *, void *UserData);
		static gboolean WindowResizeCallback(GtkWidget *Window, void *, MainWindowScreen *ThisPointer);
		static gboolean TickerAutoscrollCallback(GtkWidget *ScrolledWindow, void *, MainWindowScreen *ThisPointer);
		static void MsgDetailsDialogCallback(const uint64_t *KeyAddr);
		virtual void Show(void); //Hideous hack.
		void Set_Clickable(const bool Value);
		
		void SetStatusBarText(const char *Text);

	};
		
}

#endif //VL_CTL_GUIMAINWINDOW_H
