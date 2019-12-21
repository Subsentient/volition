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


#include <gtk/gtk.h>
#include "gui_menus.h"
#include "gui_base.h"
#include "gui_mainwindow.h"
#include "orders.h"
#include "clienttracker.h"
#include "ticker.h"
#include "scriptscanner.h"

#include <list>

#define NODE_SCRIPT_SUBMENU_NAME "Scripts"

//Types
struct NodeMenuSpecsStruct
{
	VLString Name;
	CommandCode CmdCode;
};

struct ServerMenuSpecsStruct
{
	VLString Name;
	CommandCode CmdCode;
	bool NeedNodes;
};

NodeMenuSpecsStruct NodeMenuSpecs[] =
	{
		{ "~Working directory" },
		{ "Get working directory", CMDCODE_A2C_GETCWD },
		{ "Change working directory", CMDCODE_A2C_CHDIR },
		{ },
		{ "~Processes" },
		{ "Get host processes", CMDCODE_A2C_GETPROCESSES },
		{ "Kill host process(es)", CMDCODE_A2C_KILLPROCESS },
		{ },
		{ "~Shell and host programs" },
		{ "Execute command on host", CMDCODE_A2C_EXECCMD },
		{ }, //Separator
		{ "~Files" },
		{ "Delete files on host", CMDCODE_A2C_FILES_DEL },
		{ "Copy files on host", CMDCODE_A2C_FILES_COPY },
		{ "Move files on host", CMDCODE_A2C_FILES_MOVE },
		{ "Upload files to host", CMDCODE_A2C_FILES_PLACE },
		{ "Download files from host", CMDCODE_A2C_FILES_FETCH },
		{ "List directory on host", CMDCODE_A2C_LISTDIRECTORY },
		{ },
		{ "~Jobs" },
		{ "Get running jobs", CMDCODE_B2C_GETJOBSLIST },
		{ "Kill job ID", CMDCODE_B2C_KILLJOBID },
		{ "Kill jobs by command code", CMDCODE_B2C_KILLJOBBYCMDCODE },
		{ },
		{"~Node controls" },
		{ "Shut down node", CMDCODE_A2C_SLEEP },
		{ "Forceably update binary", CMDCODE_B2C_USEUPDATE },
	};

ServerMenuSpecsStruct ServerMenuSpecs[] =
	{
		{ "~Node updates" },
		{ "Set binary for platform", CMDCODE_A2S_PROVIDEUPDATE },
		{ "Drop binary for platform", CMDCODE_A2S_REMOVEUPDATE },
		{ },
		{ "~Grouping" },
		{ "Change group", CMDCODE_A2S_CHGNODEGROUP, true },
		{ "Forget selected nodes", CMDCODE_A2S_FORGETNODE, true },
		{ },
		{ "~Vault" },
		{ "Download from vault", CMDCODE_B2S_VAULT_FETCH },
		{ "Add new item to vault", CMDCODE_B2S_VAULT_ADD },
		{ "Update item in vault", CMDCODE_B2S_VAULT_UPDATE },
		{ "Delete item from vault", CMDCODE_B2S_VAULT_DROP },
		{ "List vault contents", CMDCODE_A2S_VAULT_LIST },
		{ },
		{ "~Global configuration" },
		{ "Set global config key", CMDCODE_A2S_SETSCONFIG },
		{ "Fetch global config key", CMDCODE_A2S_GETSCONFIG },
		{ "Drop global config key", CMDCODE_A2S_DROPCONFIG },
		{ },
		{ "~Authorization tokens" },
		{ "Add token", CMDCODE_A2S_AUTHTOKEN_ADD },
		{ "Revoke token", CMDCODE_A2S_AUTHTOKEN_REVOKE },
		{ "Change token privileges", CMDCODE_A2S_AUTHTOKEN_CHG },
		{ "List installed tokens", CMDCODE_A2S_AUTHTOKEN_LIST },
		{ "Get nodes using token", CMDCODE_A2S_AUTHTOKEN_USEDBY },
		{ },
		{ "~Logs" },
		{ "Tail log", CMDCODE_A2S_SRVLOG_TAIL },
		{ "Get log size", CMDCODE_A2S_SRVLOG_SIZE },
		{ "Delete log", CMDCODE_A2S_SRVLOG_WIPE },
		{ },
		{ "~Routines" },
		{ "Create routine", CMDCODE_A2S_ROUTINE_ADD, true },
		{ "Delete routine", CMDCODE_A2S_ROUTINE_DEL },
		{ "List routines", CMDCODE_A2S_ROUTINE_LIST },
		{ "Change routine schedule", CMDCODE_A2S_ROUTINE_CHG_SCHD },
		{ "Change routine targets", CMDCODE_A2S_ROUTINE_CHG_TGT, true },
		{ "Change routine flags", CMDCODE_A2S_ROUTINE_CHG_FLAG },
		{ },
		{ "~Advanced" },
		{ "Compile custom stream", CMDCODE_INVALID },
	};

//Prototypes
static void HandleServerMenuClick(const ServerMenuSpecsStruct *Specs);
static void HandleNodeMenuClick(const NodeMenuSpecsStruct *Specs);

//Function definitions
bool GuiMenus::IsNodeOrder(const CommandCode CmdCode)
{
	if (CmdCode == CMDCODE_INVALID) return false;
	
	for (size_t Inc = 0u; Inc < sizeof NodeMenuSpecs / sizeof *NodeMenuSpecs; ++Inc)
	{
		if (NodeMenuSpecs[Inc].CmdCode == CmdCode) return true;
	}

	return false;
}

static void HandleNodeMenuClick(const NodeMenuSpecsStruct *Specs)
{
	GuiMainWindow::MainWindowScreen *Screen = static_cast<GuiMainWindow::MainWindowScreen*>(GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::MAINWINDOW));

	if (!Screen) return;

	std::vector<VLString> *DestinationNodes = Screen->GetSelectedNodes();

	if (!DestinationNodes) return;
	
ResetLoop:
	for (size_t Inc = 0u; Inc < DestinationNodes->size(); ++Inc)
	{
		if (!ClientTracker::Lookup(DestinationNodes->at(Inc))->Online)
		{
			DestinationNodes->erase(DestinationNodes->begin() + Inc);
			goto ResetLoop;
		}
	}

	if (!DestinationNodes->size())
	{
		Ticker::AddSystemMessage("No nodes selected to transmit order.");
		delete DestinationNodes;
		return;
	}
	
	Orders::CurrentOrder.Init(Specs->CmdCode, DestinationNodes);

	delete DestinationNodes;
}

static void HandleServerMenuClick(const ServerMenuSpecsStruct *Specs)
{
	GuiMainWindow::MainWindowScreen *Screen = static_cast<GuiMainWindow::MainWindowScreen*>(GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::MAINWINDOW));

	if (!Screen) return;
	
	std::vector<VLString> *DestinationNodes = Specs->NeedNodes ? Screen->GetSelectedNodes() : nullptr;

	//For server orders we could conceivably want to talk about offline nodes.
	if (DestinationNodes != nullptr)
	{
		if (!DestinationNodes->size())
		{
			Ticker::AddSystemMessage("No nodes selected to transmit order.");
			
			return;
		}
	}
	
	Orders::CurrentOrder.Init(Specs->CmdCode, DestinationNodes);
}

static void ScriptStateSub(ScriptScanner::ScriptInfo *Info, Orders::ScriptStateFuncPtr Function)
{
	GuiMainWindow::MainWindowScreen *Screen = static_cast<GuiMainWindow::MainWindowScreen*>(GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::MAINWINDOW));

	if (!Screen) return;
	
	std::vector<VLString> *DestinationNodes = Screen->GetSelectedNodes();
	
	if (!DestinationNodes) return;
	
ResetLoop:
	for (size_t Inc = 0; Inc < DestinationNodes->size(); ++Inc)
	{
		if (!ClientTracker::Lookup(DestinationNodes->at(Inc))->Online)
		{
			DestinationNodes->erase(DestinationNodes->begin() + Inc);
			goto ResetLoop;
		}
	}
	
	if (!DestinationNodes->size())
	{
		Ticker::AddSystemMessage("No online nodes selected to transmit order.");
		delete DestinationNodes;
		return;
	}
	
	Function(Info->ScriptName, DestinationNodes);
	
	delete DestinationNodes;
}

static void HandleScriptFuncClick(ScriptScanner::ScriptInfo::ScriptFunctionInfo *FuncInfo)
{
	GuiMainWindow::MainWindowScreen *Screen = static_cast<GuiMainWindow::MainWindowScreen*>(GuiBase::LookupScreen(GuiBase::ScreenObj::ScreenType::MAINWINDOW));

	if (!Screen) return;
	
	std::vector<VLString> *DestinationNodes = Screen->GetSelectedNodes();
	
	if (!DestinationNodes) return;
ResetLoop:
	for (size_t Inc = 0; Inc < DestinationNodes->size(); ++Inc)
	{
		if (!ClientTracker::Lookup(DestinationNodes->at(Inc))->Online)
		{
			DestinationNodes->erase(DestinationNodes->begin() + Inc);
			goto ResetLoop;
		}
	}
	
	if (!DestinationNodes->size())
	{
		Ticker::AddSystemMessage("No online nodes selected to transmit order.");
		delete DestinationNodes;
		return;
	}
	
	Orders::SendNodeScriptFuncOrder(FuncInfo, DestinationNodes);
}

static void HandleScriptLoadClick(ScriptScanner::ScriptInfo *Info)
{
	ScriptStateSub(Info, Orders::SendNodeScriptLoadOrder);
}
static void HandleScriptUnloadClick(ScriptScanner::ScriptInfo *Info)
{
	ScriptStateSub(Info, Orders::SendNodeScriptUnloadOrder);
}
static void HandleScriptReloadClick(ScriptScanner::ScriptInfo *Info)
{
	ScriptStateSub(Info, Orders::SendNodeScriptReloadOrder);
}

static bool PopulateMWScriptSubmenu(GtkWidget *Menu)
{
	const std::map<VLString, ScriptScanner::ScriptInfo*> &Scripts = ScriptScanner::GetKnownScripts();
	
	for (auto Iter = Scripts.begin(); Iter != Scripts.end(); ++Iter)
	{
		ScriptScanner::ScriptInfo *Info = Iter->second;
		
		GtkWidget *TopItem = gtk_menu_item_new_with_label(VLString("Script ") + Info->ScriptName + ", \"" + Info->ScriptDescription + "\"");
		
		GtkWidget *Submenu = gtk_menu_new();
		
		gtk_menu_item_set_submenu((GtkMenuItem*)TopItem, Submenu);
	
		GtkWidget *LoadItem = gtk_menu_item_new_with_label("Load script");
		GtkWidget *UnloadItem = gtk_menu_item_new_with_label("Unload script");
		GtkWidget *ReloadItem = gtk_menu_item_new_with_label("Reload script");
		
		g_signal_connect_swapped((GObject*)LoadItem, "activate", (GCallback)HandleScriptLoadClick, Info);
		g_signal_connect_swapped((GObject*)UnloadItem, "activate", (GCallback)HandleScriptUnloadClick, Info);
		g_signal_connect_swapped((GObject*)ReloadItem, "activate", (GCallback)HandleScriptReloadClick, Info);
		
		gtk_menu_shell_append((GtkMenuShell*)Submenu, LoadItem);
		gtk_menu_shell_append((GtkMenuShell*)Submenu, UnloadItem);
		gtk_menu_shell_append((GtkMenuShell*)Submenu, ReloadItem);
		gtk_menu_shell_append((GtkMenuShell*)Submenu, gtk_separator_menu_item_new());
		
		for (auto Iter = Info->Functions.begin(); Iter != Info->Functions.end(); ++Iter)
		{
			GtkWidget *NewItem = gtk_menu_item_new_with_label(VLString("Run function ") + Iter->second->Name + ": \"" + Iter->second->Description + "\"");
			
			g_signal_connect_swapped((GObject*)NewItem, "activate", (GCallback)HandleScriptFuncClick, Iter->second);
			
			gtk_menu_shell_append((GtkMenuShell*)Submenu, NewItem);
			
		}
		gtk_menu_shell_append((GtkMenuShell*)Menu, TopItem);
	}
	return true;
}

bool GuiMenus::PopulateMWNodeTreeMenus(GtkWidget *Menu)
{
	for (size_t Inc = 0u; Inc < sizeof NodeMenuSpecs / sizeof *NodeMenuSpecs; ++Inc)
	{
		if (!NodeMenuSpecs[Inc].Name)
		{
			gtk_menu_shell_append((GtkMenuShell*)Menu, gtk_separator_menu_item_new());
			continue;
		}

		
		GtkWidget *NewItem = *NodeMenuSpecs[Inc].Name == '~' ? gtk_menu_item_new() : gtk_menu_item_new_with_label(NodeMenuSpecs[Inc].Name);

		gtk_menu_shell_append((GtkMenuShell*)Menu, NewItem);

		if (*NodeMenuSpecs[Inc].Name == '~')
		{
			gtk_widget_set_sensitive((GtkWidget*)NewItem, false);
			GtkWidget *const Label = gtk_label_new(NodeMenuSpecs[Inc].Name + 1);
			gtk_widget_set_halign(Label, GTK_ALIGN_CENTER);
			gtk_widget_set_valign(Label, GTK_ALIGN_CENTER);
			gtk_container_add((GtkContainer*)NewItem, Label);
		}
		else
		{
			g_signal_connect_swapped((GObject*)NewItem, "activate", (GCallback)HandleNodeMenuClick, &NodeMenuSpecs[Inc]);
		}
	}
	
	if (ScriptScanner::GetKnownScripts().size() != 0)
	{ //No scripts? Don't draw the Scripts menu.
		GtkWidget *ScriptItem = gtk_menu_item_new_with_label(NODE_SCRIPT_SUBMENU_NAME);
		GtkWidget *Submenu = gtk_menu_new();
		
		gtk_menu_item_set_submenu((GtkMenuItem*)ScriptItem, Submenu);
		
		PopulateMWScriptSubmenu(Submenu);
		
		gtk_menu_shell_append((GtkMenuShell*)Menu, gtk_separator_menu_item_new());
		gtk_menu_shell_append((GtkMenuShell*)Menu, ScriptItem);
	}
	
	return true;
}

bool GuiMenus::PopulateMWServerMenus(GtkWidget *Menu)
{
	for (size_t Inc = 0u; Inc < sizeof ServerMenuSpecs / sizeof *ServerMenuSpecs; ++Inc)
	{
		if (!ServerMenuSpecs[Inc].Name)
		{
			gtk_menu_shell_append((GtkMenuShell*)Menu, gtk_separator_menu_item_new());
			continue;
		}
		
		
		GtkWidget *NewItem = *ServerMenuSpecs[Inc].Name == '~' ? gtk_menu_item_new() : gtk_menu_item_new_with_label(ServerMenuSpecs[Inc].Name);

		gtk_menu_shell_append((GtkMenuShell*)Menu, NewItem);

		if (*ServerMenuSpecs[Inc].Name == '~')
		{
			gtk_widget_set_sensitive((GtkWidget*)NewItem, false);

			GtkWidget *const Label = gtk_label_new(ServerMenuSpecs[Inc].Name + 1);

			gtk_widget_set_halign(Label, GTK_ALIGN_CENTER);
			gtk_widget_set_valign(Label, GTK_ALIGN_CENTER);
			
			gtk_container_add((GtkContainer*)NewItem, Label);
		}
		else
		{
			g_signal_connect_swapped((GObject*)NewItem, "activate", (GCallback)HandleServerMenuClick, &ServerMenuSpecs[Inc]);
		}
	}
	
	return true;
}
