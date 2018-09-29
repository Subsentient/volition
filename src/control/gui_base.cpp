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
#include <gdk/gdkkeysyms.h>

#include "../libvolition/include/common.h"

#include "gui_base.h"
#include "gui_icons.h"
#include "main.h"

//Globals
static GuiBase::ScreenRegistry Registry;

//Prototypes

//Function definitions

GdkPixbuf *GuiBase::LoadImageToPixbuf(const uint8_t *FileBuffer, const size_t Size)
{
	GdkPixbufLoader *Ldr = gdk_pixbuf_loader_new();

	gdk_pixbuf_loader_write(Ldr, FileBuffer, Size, NULL);
	gdk_pixbuf_loader_close(Ldr, NULL);

	return gdk_pixbuf_loader_get_pixbuf(Ldr);
}

void GuiBase::NukeContainerChildren(GtkContainer *Container)
{
	if (!Container) return;
	
	GList *Children = gtk_container_get_children(Container);
	GList *Worker = Children;
	
	if (!GTK_IS_CONTAINER(Container))
	{
		return;
	}
	
	
	for (; Worker; Worker = g_list_next(Worker))
	{
		if (GTK_IS_CONTAINER((GtkWidget*)Worker->data))
		{
			NukeContainerChildren((GtkContainer*)Worker->data);
		}
		
		gtk_widget_destroy((GtkWidget*)Worker->data);
	}
	
	g_list_free(Children);
}

void GuiBase::NukeWidget(GtkWidget *Widgy)
{
	if (!Widgy) return;
	
	NukeContainerChildren((GtkContainer*)Widgy);
		
	gtk_widget_destroy(Widgy);
}

void GuiBase::ShutdownCallback(GtkWidget *Widget, gpointer Stuff)
{
	Main::TerminateLink();
	exit(0);
}

GuiBase::ScreenObj::ScreenObj(GtkWidget *InWindow, const ScreenType InType) : ObjType(InType), Window(InWindow)
{
	Registry.Add(this);
}

GuiBase::ScreenObj::~ScreenObj(void)
{
	Registry.Delete(this);
	NukeWidget(this->Window);
}

void GuiBase::ScreenObj::Show(void)
{
	gtk_widget_show_all(this->Window);
}
void GuiBase::ScreenObj::Hide(void)
{
	gtk_widget_hide(this->Window);
}


GuiBase::LoadingScreen::LoadingScreen(const VLString &Message)
		: ScreenObj(gtk_window_new(GTK_WINDOW_TOPLEVEL), ScreenType::LOADING),
		Icon(gtk_image_new_from_pixbuf(LoadImageToPixbuf(GuiIcons::LookupIcon("ctlsplash").data(), GuiIcons::LookupIcon("ctlsplash").size()))),
		ProgressBar(gtk_progress_bar_new()),
		VBox(gtk_vbox_new(FALSE, 5))
{
	gtk_window_set_resizable((GtkWindow*)this->Window, false);
	gtk_window_set_decorated((GtkWindow*)this->Window, false);
	gtk_window_set_title((GtkWindow*)this->Window, "Initializing Volition Control...");
	gtk_window_set_default_size((GtkWindow*)this->Window, 256, 192);
	gtk_container_set_border_width((GtkContainer*)this->Window, 2);

	//g_signal_connect((GObject*)this->Window, "destroy", (GCallback)ShutdownCallback, nullptr);

	gtk_window_set_icon((GtkWindow*)this->Window, LoadImageToPixbuf(GuiIcons::LookupIcon("ctlwmicon").data(), GuiIcons::LookupIcon("ctlwmicon").size()));
	
	gtk_progress_bar_set_text((GtkProgressBar*)this->ProgressBar, Message);
	
	gtk_container_add((GtkContainer*)this->Window, this->VBox);

	gtk_box_pack_start((GtkBox*)this->VBox, this->Icon, FALSE, TRUE, 0);
	gtk_box_pack_start((GtkBox*)this->VBox, this->ProgressBar, TRUE, TRUE, 0);
}

void GuiBase::LoadingScreen::SetStatusText(const char *Text)
{
	gtk_progress_bar_set_text((GtkProgressBar*)this->ProgressBar, Text);
}

void GuiBase::LoadingScreen::PulseProgressBar(void)
{
	gtk_progress_bar_pulse((GtkProgressBar*)this->ProgressBar);
}

double GuiBase::LoadingScreen::GetProgressBarFraction(void)
{
	return gtk_progress_bar_get_fraction((GtkProgressBar*)this->ProgressBar);
}

void GuiBase::LoadingScreen::SetProgressBarFraction(const double Value)
{
	gtk_progress_bar_set_fraction((GtkProgressBar*)this->ProgressBar, Value);
}
void GuiBase::LoadingScreen::IncreaseProgressBarFraction(const double ToAdd)
{
	gtk_progress_bar_set_fraction((GtkProgressBar*)this->ProgressBar, gtk_progress_bar_get_fraction((GtkProgressBar*)this->ProgressBar) + ToAdd);
}

void GuiBase::LoadingScreen::SetProgressBarPulseFraction(const double Value)
{
	gtk_progress_bar_set_pulse_step((GtkProgressBar*)this->ProgressBar, Value);
}

GuiBase::LoginScreen::LoginScreen(void)
		: ScreenObj(gtk_window_new(GTK_WINDOW_TOPLEVEL), ScreenType::LOGIN),
		Icon(gtk_image_new_from_pixbuf(LoadImageToPixbuf(GuiIcons::LookupIcon("ctlsplash").data(), GuiIcons::LookupIcon("ctlsplash").size()))),
		Table(gtk_table_new(5, 2, false)),
		AccelGroup(gtk_accel_group_new()),
		GreetingLabel(gtk_label_new("Volition Control Login")),
		UsernameLabel(gtk_label_new("Username")),
		PasswordLabel(gtk_label_new("Password")),
		UsernameBox(gtk_entry_new()),
		PasswordBox(gtk_entry_new()),
		ButtonAlign(gtk_alignment_new(1.0, 0.5, 0.01, 0.01)),
		ConnectButton(gtk_button_new_with_mnemonic("_Connect"))
{
	gtk_window_set_resizable((GtkWindow*)this->Window, FALSE);
	gtk_window_set_title((GtkWindow*)this->Window, "Volition Control Login");
	gtk_window_set_default_size((GtkWindow*)this->Window, 256, 192);
	gtk_window_add_accel_group((GtkWindow*)this->Window, this->AccelGroup);

	gtk_container_set_border_width((GtkContainer*)this->Window, 2);

	g_signal_connect((GObject*)this->Window, "destroy", (GCallback)NukeWidget, nullptr);

	gtk_window_set_icon((GtkWindow*)this->Window, LoadImageToPixbuf(GuiIcons::LookupIcon("ctlwmicon").data(), GuiIcons::LookupIcon("ctlwmicon").size()));

	gtk_container_add((GtkContainer*)this->Window, Table);

	//Icon and greeting
	gtk_table_attach((GtkTable*)this->Table, this->Icon, 0, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)this->Table, this->GreetingLabel, 0, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	//username label and box
	gtk_table_attach((GtkTable*)this->Table, this->UsernameLabel, 0, 1, 2, 3, GtkAttachOptions(GTK_SHRINK | GTK_EXPAND), GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)this->Table, this->UsernameBox, 1, 2, 2, 3, GTK_SHRINK, GTK_FILL, 0, 0);
	
	//password label and box
	gtk_table_attach((GtkTable*)this->Table, this->PasswordLabel, 0, 1, 3, 4, GtkAttachOptions(GTK_SHRINK | GTK_EXPAND), GTK_FILL, 0, 0);
	gtk_table_attach((GtkTable*)this->Table, this->PasswordBox, 1, 2, 3, 4, GTK_SHRINK, GTK_FILL, 0, 0);

	gtk_entry_set_visibility((GtkEntry*)this->PasswordBox, false);

	gtk_container_add((GtkContainer*)this->ButtonAlign, this->ConnectButton);
	
	//Connect button
	gtk_table_attach((GtkTable*)this->Table, this->ButtonAlign, 1, 2, 4, 5, GtkAttachOptions(GTK_EXPAND | GTK_FILL), GtkAttachOptions(GTK_SHRINK | GTK_EXPAND), 0, 0);
	gtk_widget_add_accelerator(this->ConnectButton, "clicked", this->AccelGroup, GDK_KEY_Return, GdkModifierType(0), GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(this->ConnectButton, "clicked", this->AccelGroup, GDK_KEY_KP_Enter, GdkModifierType(0), GTK_ACCEL_VISIBLE);

	g_signal_connect_swapped((GObject*)this->ConnectButton, "clicked", (GCallback)Main::InitiateLogin, this);
	
}

VLString GuiBase::LoginScreen::GetUsernameField(void) const
{
	return gtk_entry_get_text((GtkEntry*)this->UsernameBox);
}

VLString GuiBase::LoginScreen::GetPasswordField(void) const
{
	return gtk_entry_get_text((GtkEntry*)this->PasswordBox);
}


bool GuiBase::ScreenRegistry::Add(ScreenObj *Screen)
{
	//Check for duplicates.
	auto Iter = this->List.begin();

	for (; Iter != this->List.end(); ++Iter)
	{
		if (*Iter == Screen)
		{
			return false;
		}
	}


	this->List.push_back(Screen);

	return true;
}


bool GuiBase::ScreenRegistry::Delete(ScreenObj *Screen)
{
	//Check for duplicates.
	auto Iter = this->List.begin();

	for (; Iter != this->List.end(); ++Iter)
	{
		if (*Iter == Screen)
		{
			this->List.erase(Iter);
			return true;
		}
	}

	return false;
}

GuiBase::ScreenObj *GuiBase::ScreenRegistry::Lookup(const ScreenObj::ScreenType Type) const
{
	auto Iter = this->List.begin();

	for (; Iter != this->List.end(); ++Iter)
	{
		if ((*Iter)->GetScreenType() == Type)
		{
			return *Iter;
		}
	}

	return nullptr;
}


GuiBase::ScreenObj *GuiBase::LookupScreen(const ScreenObj::ScreenType Type)
{
	return Registry.Lookup(Type);
}

bool GuiBase::PurgeScreen(const ScreenObj::ScreenType Type)
{
	ScreenObj *BaseObj = LookupScreen(Type);

	if (!BaseObj) return false;
	
	Registry.Delete(BaseObj);
	delete BaseObj;

	return true;
}

VLString GuiBase::MapStatusToIcon(const NetCmdStatus &Status)
{
	switch (Status.Status)
	{
		case STATUS_WARN:
			return GTK_STOCK_DIALOG_WARNING;
		case STATUS_OK:
			return GTK_STOCK_DIALOG_INFO;
		case STATUS_ACCESSDENIED:
			return GTK_STOCK_DIALOG_AUTHENTICATION;
		default:
			return GTK_STOCK_DIALOG_ERROR;
	}
}


