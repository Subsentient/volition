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


/* Instead of getting bogged down in the infuriating details, focus on the unquestionably terrible big picture. -The Onion */

#include "../libvolition/include/common.h"
#include "../libvolition/include/conation.h"
#include "../libvolition/include/utils.h"
#include "gui_dialogs.h"
#include "gui_base.h"
#include "gui_icons.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <utility>

#ifndef CTL_REVISION
#define CTL_REVISION "(undefined)"
#endif

GuiDialogs::DialogBase::DialogBase(const DialogType InType, const VLString &Title) : Type(InType), Dismissed(), Window(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{ //Base class for all dialogs. Sets certain properties for them all.
	gtk_window_set_skip_taskbar_hint((GtkWindow*)this->Window, false);
	gtk_window_set_type_hint((GtkWindow*)this->Window, GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_container_set_border_width((GtkContainer*)this->Window, 4);
	gtk_window_set_resizable((GtkWindow*)this->Window, false);
	if (GuiIcons::IconsLoaded()) gtk_window_set_icon((GtkWindow*)this->Window, GuiBase::LoadImageToPixbuf(GuiIcons::LookupIcon("ctlwmicon").data(), GuiIcons::LookupIcon("ctlwmicon").size()));
	gtk_window_set_title((GtkWindow*)this->Window, Title);
}

GuiDialogs::DialogBase::~DialogBase(void)
{
	GuiBase::NukeWidget(this->Window);
}

void GuiDialogs::DialogBase::Hide(void)
{
	gtk_widget_hide(this->Window);
}

void GuiDialogs::DialogBase::Show(void)
{
	gtk_widget_show_all(this->Window);
}

GuiDialogs::SimpleTextDialog::SimpleTextDialog(const VLString &Title, const VLString &Prompt, OnCompleteFunc InFunc, const void *PassAlongWith)
	:	DialogBase(DIALOG_SIMPLETEXT, Title),
		Grid(gtk_grid_new()),
		Label(gtk_label_new(Prompt)),
		TextEntry(gtk_entry_new()),
		Button(gtk_button_new_with_mnemonic("_Accept")),
		AccelGroup(gtk_accel_group_new()),
		Func(InFunc),
		UserData(PassAlongWith)
{
	gtk_container_add((GtkContainer*)this->Window, this->Grid);

	gtk_grid_attach((GtkGrid*)this->Grid, this->Label, 0, 0, 1, 1);
	gtk_grid_attach((GtkGrid*)this->Grid, this->TextEntry, 0, 1, 1, 1);
	gtk_grid_attach((GtkGrid*)this->Grid, this->Button, 0, 2, 1, 1);

	gtk_widget_set_halign(this->Button, GTK_ALIGN_END);

	g_signal_connect_swapped((GObject*)this->Button, "clicked", (GCallback)SimpleTextDialog::Callback, this);

	gtk_window_add_accel_group((GtkWindow*)this->Window, this->AccelGroup);
	
	//The two enter keys
	gtk_widget_add_accelerator(this->Button, "clicked", this->AccelGroup, GDK_KEY_Return, GdkModifierType(0), GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(this->Button, "clicked", this->AccelGroup, GDK_KEY_KP_Enter, GdkModifierType(0), GTK_ACCEL_VISIBLE);
}
	
void *GuiDialogs::SimpleTextDialog::Callback(SimpleTextDialog *ThisPointer)
{
	//Get text from the entry buffer.
	ThisPointer->TextData = gtk_entry_get_text((GtkEntry*)ThisPointer->TextEntry);

	ThisPointer->Hide();
	ThisPointer->Dismissed = true;

	if (ThisPointer->Func) ThisPointer->Func(ThisPointer->UserData, ThisPointer->TextData);
	
	return nullptr;
}


GuiDialogs::LargeTextDialog::LargeTextDialog(const VLString &Title, const VLString &Prompt, OnCompleteFunc InFunc, const void *PassAlongWith)
	:	DialogBase(DIALOG_LARGETEXT, Title),
		Grid(gtk_grid_new()),
		Label(gtk_label_new(Prompt)),
		Separator(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)),
		ScrolledWindow(gtk_scrolled_window_new(nullptr, nullptr)),
		TextView(gtk_text_view_new()),
		Button(gtk_button_new_with_mnemonic("_Accept")),
		Func(InFunc),
		UserData(PassAlongWith)
{
	gtk_widget_set_size_request(this->Window, 500, 400);
	gtk_container_add((GtkContainer*)this->Window, this->Grid);
	gtk_container_add((GtkContainer*)this->ScrolledWindow, this->TextView);

	gtk_scrolled_window_set_policy((GtkScrolledWindow*)this->ScrolledWindow, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_widget_set_hexpand(this->ScrolledWindow, true);
	gtk_widget_set_vexpand(this->ScrolledWindow, true);
	
	gtk_widget_set_halign(this->Button, GTK_ALIGN_END);
	
	gtk_grid_attach((GtkGrid*)this->Grid, this->Label, 			0, 0, 1, 1);
	gtk_grid_attach((GtkGrid*)this->Grid, this->Separator,		0, 1, 1, 1);
	gtk_grid_attach((GtkGrid*)this->Grid, this->ScrolledWindow, 0, 2, 1, 1);
	gtk_grid_attach((GtkGrid*)this->Grid, this->Button, 		0, 3, 1, 1);

	g_signal_connect_swapped((GObject*)this->Button, "clicked", (GCallback)LargeTextDialog::Callback, this);
}
	
void *GuiDialogs::LargeTextDialog::Callback(LargeTextDialog *ThisPointer)
{
	//Get text from the entry buffer.
	GtkTextBuffer *Buffer = gtk_text_view_get_buffer((GtkTextView*)ThisPointer->TextView);
	GtkTextIter Start, End;

	gtk_text_buffer_get_start_iter(Buffer, &Start);
	gtk_text_buffer_get_end_iter(Buffer, &End);
	
	ThisPointer->TextData = gtk_text_buffer_get_text(Buffer, &Start, &End, true);

	ThisPointer->Hide();
	ThisPointer->Dismissed = true;

	//Don't put anything that accesses the this pointer after this!
	if (ThisPointer->Func)
	{
		ThisPointer->Func(ThisPointer->UserData, ThisPointer->TextData);
	}
	
	return nullptr;
}

GuiDialogs::FileSelectionDialog::FileSelectionDialog(const VLString &Title, const VLString &Prompt,
													const bool InAllowMultiple, OnCompleteFunc InFunc,
													const void *InUserData)
	: 	DialogBase(DIALOG_FILECHOOSER, Title),
		VBox(gtk_box_new(GTK_ORIENTATION_VERTICAL, 8)),
		Label(gtk_label_new(Prompt)),
		Separator1(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)),
		BrowseButton(gtk_button_new_with_mnemonic("_Browse")),
		CounterLabel(gtk_label_new("No files selected")),
		Separator2(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)),
		AcceptButton(gtk_button_new_with_mnemonic("_Accept")),
		Func(InFunc),
		UserData(InUserData),
		AllowMultiple(InAllowMultiple)
{	
	this->CurrentPaths.reserve(32); //Decent guess at a most likely common maximum.
	
	gtk_container_add((GtkContainer*)this->Window, this->VBox);
	
	gtk_widget_set_halign(this->AcceptButton, GTK_ALIGN_END);
	gtk_widget_set_halign(this->BrowseButton, GTK_ALIGN_CENTER);


	gtk_box_pack_start((GtkBox*)this->VBox, this->Label, false, false, 8);
	gtk_box_pack_start((GtkBox*)this->VBox, this->Separator1, false, false, 0);
	gtk_box_pack_start((GtkBox*)this->VBox, this->BrowseButton, false, false, 0);
	gtk_box_pack_start((GtkBox*)this->VBox, this->CounterLabel, false, false, 0);
	gtk_box_pack_start((GtkBox*)this->VBox, this->Separator2, false, false, 0);
	gtk_box_pack_start((GtkBox*)this->VBox, this->AcceptButton, false, false, 0);
	
	g_signal_connect_swapped((GObject*)this->BrowseButton, "clicked", (GCallback)FileSelectionDialog::BrowsePressedCallback, this);
	
	g_signal_connect_swapped((GObject*)this->AcceptButton, "clicked", (GCallback)FileSelectionDialog::AcceptPressedCallback, this);
	
	//Don't let us click Accept until we've chosen something.
	gtk_widget_set_sensitive(this->AcceptButton, false);
	
}

void *GuiDialogs::FileSelectionDialog::BrowsePressedCallback(FileSelectionDialog *ThisPointer)
{
	GtkWidget *Chooser = gtk_file_chooser_dialog_new("Select file(s)", (GtkWindow*)ThisPointer->Window,
								GTK_FILE_CHOOSER_ACTION_OPEN, "document-open",
								GTK_RESPONSE_ACCEPT, nullptr);

	gtk_file_chooser_set_select_multiple((GtkFileChooser*)Chooser, ThisPointer->AllowMultiple);

	gtk_dialog_run((GtkDialog*)Chooser);

	const size_t Capacity = ThisPointer->CurrentPaths.capacity();
	ThisPointer->CurrentPaths.clear();
	ThisPointer->CurrentPaths.reserve(Capacity);
	
	GSList *List = gtk_file_chooser_get_filenames((GtkFileChooser*)Chooser);
	
	if (List != nullptr)
	{ //We chose something so we can proceed.
		gtk_widget_set_sensitive(ThisPointer->AcceptButton, true);
	}
	
	for (GSList *Worker = List; Worker != nullptr; Worker = g_slist_next(Worker))
	{
		ThisPointer->CurrentPaths.push_back((const char*)Worker->data);
		g_free(Worker->data);
	}

	g_slist_free(List);

	GuiBase::NukeWidget(Chooser);
	
	VLString NewCounterData = std::to_string(ThisPointer->CurrentPaths.size()) + " files selected";

	gtk_label_set_text((GtkLabel*)ThisPointer->CounterLabel, NewCounterData);

	return nullptr;
}

void *GuiDialogs::FileSelectionDialog::AcceptPressedCallback(FileSelectionDialog *ThisPointer)
{
	ThisPointer->CurrentPaths.shrink_to_fit();

	ThisPointer->Hide();
	ThisPointer->Dismissed = true;

	if (ThisPointer->Func) ThisPointer->Func(ThisPointer->UserData, &ThisPointer->CurrentPaths);

	return nullptr;
}


GuiDialogs::TextDisplayDialog::TextDisplayDialog(const VLString &WindowTitle,
												const VLString &LabelText,
												const char *InData,
												TextDisplayDialog::OnCompleteFunc CallbackFunc,
												const void *InUserData)
											: DialogBase(DIALOG_TEXTDISPLAY, WindowTitle),
											VBox(gtk_box_new(GTK_ORIENTATION_VERTICAL, 8)),
											Label(gtk_label_new(LabelText)),
											Separator1(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)),
											ScrolledWindow(gtk_scrolled_window_new(nullptr, nullptr)),
											TextView(gtk_text_view_new()),
											Separator2(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)),
											CloseButton(gtk_button_new_with_mnemonic("_Dismiss")),
											AccelGroup(gtk_accel_group_new()),
											Data(InData),
											Func(CallbackFunc),
											UserData(InUserData)
{
	//This one, we DO want resizable, so we countermand our base class' order.
	gtk_window_set_resizable((GtkWindow*)this->Window, true);
	
	gtk_container_add((GtkContainer*)this->Window, this->VBox);

	gtk_container_add((GtkContainer*)this->ScrolledWindow, this->TextView);

	gtk_scrolled_window_set_policy((GtkScrolledWindow*)this->ScrolledWindow, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_widget_set_halign(this->CloseButton, GTK_ALIGN_END);
	
	gtk_box_pack_start((GtkBox*)this->VBox, this->Label, false, false, 8);
	gtk_box_pack_start((GtkBox*)this->VBox, this->Separator1, false, false, 0);
	gtk_box_pack_start((GtkBox*)this->VBox, this->ScrolledWindow, true, true, 0);
	gtk_box_pack_start((GtkBox*)this->VBox, this->Separator2, false, false, 0);
	gtk_box_pack_start((GtkBox*)this->VBox, this->CloseButton, false, false, 0);

	g_signal_connect_swapped((GObject*)this->CloseButton, "clicked", (GCallback)TextDisplayDialog::ClosePressedCallback, this);

	GtkTextBuffer *TextViewBuffer = gtk_text_view_get_buffer((GtkTextView*)this->TextView);

	gtk_text_buffer_set_text(TextViewBuffer, Data, Data.size());

	gtk_text_view_set_editable((GtkTextView*)this->TextView, FALSE);
	gtk_text_view_set_cursor_visible((GtkTextView*)this->TextView, FALSE);

	gtk_widget_set_size_request(this->Window, 500, 500);

	//Set accelerator for ESC.
	gtk_window_add_accel_group((GtkWindow*)this->Window, this->AccelGroup);
	gtk_widget_add_accelerator(this->CloseButton, "clicked", this->AccelGroup, GDK_KEY_Escape, GdkModifierType(0), GTK_ACCEL_VISIBLE);
}

void *GuiDialogs::TextDisplayDialog::ClosePressedCallback(TextDisplayDialog *ThisPointer)
{
	ThisPointer->Hide();
	ThisPointer->Dismissed = true;

	if (ThisPointer->Func)
	{
		ThisPointer->Func(ThisPointer->UserData);
	}

	return nullptr;
}

GuiDialogs::YesNoDialog::YesNoDialog(const VLString &WindowTitle, const VLString &Prompt, const bool UseBooleanNames, YesNoDialog::OnCompleteFunc CallbackFunc, const void *InUserData)
	: DialogBase(DIALOG_YESNODIALOG, WindowTitle),
	Grid(gtk_grid_new()),
	Label(gtk_label_new(Prompt)),
	Separator(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)),
	YesButton(gtk_button_new_with_mnemonic(UseBooleanNames ? "_True" : "_Yes")),
	NoButton(gtk_button_new_with_mnemonic(UseBooleanNames ? "_False" : "_No")),
	AccelGroup(gtk_accel_group_new()),
	Func(CallbackFunc),
	UserData(InUserData)
{
	gtk_container_add((GtkContainer*)this->Window, this->Grid);
	
	gtk_grid_attach((GtkGrid*)this->Grid, this->Label, 0, 0, 2, 1);
	gtk_grid_attach((GtkGrid*)this->Grid, this->Separator, 0, 1, 2, 1);
	gtk_grid_attach((GtkGrid*)this->Grid, this->YesButton, 0, 1, 1, 1);
	gtk_grid_attach((GtkGrid*)this->Grid, this->NoButton, 1, 1, 1, 1);

	
	//Set up y/n accelerators.
	gtk_window_add_accel_group((GtkWindow*)this->Window, this->AccelGroup);
	gtk_widget_add_accelerator(this->YesButton, "clicked", this->AccelGroup, GDK_KEY_y, GdkModifierType(0), GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(this->YesButton, "clicked", this->AccelGroup, GDK_KEY_Y, GdkModifierType(0), GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(this->NoButton, "clicked", this->AccelGroup, GDK_KEY_n, GdkModifierType(0), GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(this->NoButton, "clicked", this->AccelGroup, GDK_KEY_N, GdkModifierType(0), GTK_ACCEL_VISIBLE);

	g_signal_connect((GObject*)this->YesButton, "clicked", (GCallback)YesNoDialog::ChoiceMadeCallback, this);
	g_signal_connect((GObject*)this->NoButton, "clicked", (GCallback)YesNoDialog::ChoiceMadeCallback, this);
	
}

void *GuiDialogs::YesNoDialog::ChoiceMadeCallback(GtkWidget *Button, YesNoDialog *ThisPointer)
{
	ThisPointer->Hide();
	ThisPointer->Dismissed = true;
	
	ThisPointer->Value = Button == ThisPointer->YesButton;
	
#ifdef DEBUG
	printf("GuiDialogs::YesNoDialog::ChoiceMadeCallback(): Value of boolean is %s\n", ThisPointer->Value ? "true" : "false");
#endif

	if (ThisPointer->Func)
	{
		ThisPointer->Func(ThisPointer->UserData, ThisPointer->Value);
	}
	
	return nullptr;
}

GuiDialogs::CmdStatusDialog::CmdStatusDialog(const VLString &WindowTitle, const NetCmdStatus &Status, OnCompleteFunc CallbackFunc, const void *InUserData)
		: DialogBase(DIALOG_CMDSTATUS, WindowTitle),
		Grid(gtk_grid_new()),
		Label(gtk_label_new(Status.Msg)),
		StatusIcon(gtk_image_new_from_icon_name(GuiBase::MapStatusToIcon(Status), GTK_ICON_SIZE_DIALOG)),
		Button(gtk_button_new_with_mnemonic("_Dismiss")),
		AccelGroup(gtk_accel_group_new()),
		Func(CallbackFunc),
		UserData(InUserData)
{
	gtk_container_add((GtkContainer*)this->Window, this->Grid);

	gtk_widget_set_halign((GtkWidget*)this->Button, GTK_ALIGN_END);
	
	gtk_grid_attach((GtkGrid*)this->Grid, this->StatusIcon, 0, 0, 1, 1);
	gtk_grid_attach((GtkGrid*)this->Grid, this->Label, 1, 0, 1, 1);
	gtk_grid_attach((GtkGrid*)this->Grid, this->Button, 1, 2, 1, 2);


	gtk_widget_add_accelerator(this->Button, "clicked", this->AccelGroup, GDK_KEY_Return, GdkModifierType(0), GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(this->Button, "clicked", this->AccelGroup, GDK_KEY_KP_Enter, GdkModifierType(0), GTK_ACCEL_VISIBLE);

	g_signal_connect_swapped((GObject*)this->Button, "clicked", (GCallback)CmdStatusDialog::DismissedCallback, this);
}

void *GuiDialogs::CmdStatusDialog::DismissedCallback(CmdStatusDialog *ThisPointer)
{
	ThisPointer->Hide();
	ThisPointer->Dismissed = true;
	
	if (ThisPointer->Func)
	{
		ThisPointer->Func(ThisPointer->UserData);
	}
	
	return nullptr;
}


void *GuiDialogs::ProgressDialog::CompletionCallback(ProgressDialog *ThisPointer)
{
	ThisPointer->Hide();
	ThisPointer->Dismissed = true;
	
	if (ThisPointer->Func)
	{
		ThisPointer->Func(ThisPointer->UserData);
	}
	
	return nullptr;
}

void GuiDialogs::ProgressDialog::SetBarText(const char *NewText)
{
	gtk_progress_bar_set_text((GtkProgressBar*)this->ProgressBar, NewText);
}
void GuiDialogs::ProgressDialog::SetLabelText(const char *NewText)
{
	gtk_label_set_text((GtkLabel*)this->Label, NewText);
}

void GuiDialogs::ProgressDialog::Pulse(void)
{
	gtk_progress_bar_pulse((GtkProgressBar*)this->ProgressBar);
}

double GuiDialogs::ProgressDialog::GetFraction(void) const
{
	return gtk_progress_bar_get_fraction((GtkProgressBar*)this->ProgressBar);
}

void GuiDialogs::ProgressDialog::SetFraction(const double Value)
{
	gtk_progress_bar_set_fraction((GtkProgressBar*)this->ProgressBar, Value);
}
void GuiDialogs::ProgressDialog::IncreaseFraction(const double ToAdd)
{
	gtk_progress_bar_set_fraction((GtkProgressBar*)this->ProgressBar, gtk_progress_bar_get_fraction((GtkProgressBar*)this->ProgressBar) + ToAdd);
}

void GuiDialogs::ProgressDialog::SetPulseFraction(const double Value)
{
	gtk_progress_bar_set_pulse_step((GtkProgressBar*)this->ProgressBar, Value);
}

void GuiDialogs::ProgressDialog::ForceComplete(void)
{
	ProgressDialog::CompletionCallback(this);
}

GuiDialogs::ProgressDialog::ProgressDialog(const VLString &WindowTitle, const VLString &LabelText, const VLString &ProgressBarText, const bool UserCanDismissIn, OnCompleteFunc CallbackFunc, const void *UserDataIn)
		: DialogBase(DIALOG_PROGRESSDIALOG, WindowTitle),
		VBox(gtk_box_new(GTK_ORIENTATION_VERTICAL, 8)),
		Label(gtk_label_new(LabelText)),
		ProgressBar(gtk_progress_bar_new()),
		DismissButton(gtk_button_new_with_mnemonic("_Dismiss")),
		UserCanDismiss(UserCanDismissIn),
		Func(CallbackFunc),
		UserData(UserDataIn)
{
	gtk_container_add((GtkContainer*)this->Window, this->VBox);
	
	gtk_box_pack_start((GtkBox*)this->VBox, this->ProgressBar, true, true, 8);
	
	
	this->SetBarText(ProgressBarText);

	gtk_widget_set_halign(this->DismissButton, GTK_ALIGN_END);
	
	g_signal_connect_swapped((GObject*)this->DismissButton, "clicked", (GCallback)ProgressDialog::CompletionCallback, this);
	
	gtk_widget_set_sensitive(this->DismissButton, this->UserCanDismiss);
	
	gtk_widget_set_size_request(this->Window, 400, -1);

}

GuiDialogs::BinaryFlagsDialog::BinaryFlagsDialog(const VLString &WindowTitle,
												const VLString &LabelText,
												const std::vector<std::tuple<VLString, uint64_t, bool> > &FlagSpecs,
												const OnCompleteFunc InFunc,
												const void *InUserData)
		: DialogBase(DIALOG_BINARYFLAGS, WindowTitle),
		VBox(gtk_box_new(GTK_ORIENTATION_VERTICAL, 8)),
		Label(gtk_label_new(LabelText)),
		Separator(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)),
		AcceptButton(gtk_button_new_with_mnemonic("_Accept")),
		Func(InFunc),
		UserData(InUserData)
{
	gtk_container_add((GtkContainer*)this->Window, this->VBox);

	
	GtkWidget *BaseHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	
	gtk_widget_set_halign(this->AcceptButton, GTK_ALIGN_END);
	
	gtk_box_pack_start((GtkBox*)BaseHBox, this->Label, false, false, 8);
	gtk_box_pack_start((GtkBox*)BaseHBox, this->AcceptButton, false, false, 8);
	
	gtk_box_pack_start((GtkBox*)this->VBox, BaseHBox, false, false, 8);
	gtk_box_pack_start((GtkBox*)this->VBox, Separator, false, false, 8);
	
	this->ToggleButtons.reserve(FlagSpecs.size());
	
	for (size_t Inc = 0; Inc < FlagSpecs.size(); ++Inc)
	{
		this->ToggleButtons.emplace_back(gtk_toggle_button_new_with_label(std::get<0>(FlagSpecs[Inc])), std::get<1>(FlagSpecs[Inc]));

		if (std::get<2>(FlagSpecs[Inc]))
		{ //We're supposed to mark this one as pre-enabled.
			gtk_toggle_button_set_active((GtkToggleButton*)this->ToggleButtons.back().first, true);
		}
		
	}

	GtkWidget *CurrentHBox = nullptr;
	
	for (size_t Inc = 0, ButtonCounter = 0; Inc < FlagSpecs.size(); ++Inc, --ButtonCounter)
	{
		if (!ButtonCounter)
		{
			CurrentHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
			gtk_box_pack_start((GtkBox*)this->VBox, CurrentHBox, true, true, 8);
			
			ButtonCounter = BinaryFlagsDialog::MaxButtonsPerRow;
		}

		gtk_box_pack_start((GtkBox*)CurrentHBox, std::get<0>(this->ToggleButtons[Inc]), true, true, 8);
	}

	g_signal_connect_swapped((GObject*)this->AcceptButton, "clicked", (GCallback)BinaryFlagsDialog::AcceptPressedCallback, this);

	gtk_widget_grab_focus(this->AcceptButton);
}

void GuiDialogs::BinaryFlagsDialog::AcceptPressedCallback(BinaryFlagsDialog *ThisPointer)
{
	ThisPointer->Hide();
	ThisPointer->Dismissed = true;

	if (ThisPointer->Func) ThisPointer->Func(ThisPointer->UserData, ThisPointer);
}

uint64_t GuiDialogs::BinaryFlagsDialog::GetFlagValues(void) const
{
	uint64_t RetVal = 0u;

	for (size_t Inc = 0u; Inc < this->ToggleButtons.size(); ++Inc)
	{
		if (gtk_toggle_button_get_active((GtkToggleButton*)this->ToggleButtons[Inc].first))
		{
			RetVal |= this->ToggleButtons[Inc].second;
		}
	}

	return RetVal;
}

GuiDialogs::ArgSelectorDialog::ArgSelectorDialog(const VLString &WindowTitle, const VLString &LabelText,
												Conation::ConationStream *Stream,
												const std::vector<Conation::ArgType> *RequiredParameters,
												const Conation::ArgType AcceptedOptionals,
												const OnCompleteFunc InFunc,
												const void *InUserData,
												const bool AllowCmdCodeChange)
		: DialogBase(DIALOG_ARGSELECTOR, WindowTitle),
		VBox(gtk_box_new(GTK_ORIENTATION_VERTICAL, 8)),
		UserLabel(gtk_label_new(LabelText)),
		Separator(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)),
		AcceptButton(gtk_button_new_with_mnemonic("_Accept")),
		ArgAddMenu(gtk_menu_new()),
		ArgAddButton(gtk_button_new_with_label("Add")),
		StaticButtonsHBox(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8)),
		CmdCodeChoiceButton(gtk_button_new_with_label(CommandCodeToString(Stream ? Stream->GetCommandCode() : CommandCode()))),
		CmdCodeChoiceMenu(gtk_menu_new()),
		HBoxes({ { gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8), 0u }}),
		OriginalStream(Stream),
		Header(Stream ? Stream->GetHeader() : Conation::ConationStream::StreamHeader()),
		Func(InFunc),
		UserData(InUserData)
{ ///This class and all its helper functions and subordinates are going to make you cry blood.
	//So we're not reallocating constantly.
	if (Stream) this->ArgButtons.reserve(Stream->CountArguments() + RequiredParameters->size() + 10);
	
	
	if (RequiredParameters && RequiredParameters->size() != 0)
	{ //Gotta make sure we have all parameters filled in before they can do that.
		gtk_widget_set_sensitive(this->AcceptButton, false);
	}
	
	gtk_container_add((GtkContainer*)this->Window, this->VBox);
	
	
	gtk_box_pack_start((GtkBox*)this->StaticButtonsHBox, this->CmdCodeChoiceButton, true, true, 0);
	gtk_box_pack_start((GtkBox*)this->StaticButtonsHBox, this->ArgAddButton, true, true, 0);
	gtk_box_pack_start((GtkBox*)this->StaticButtonsHBox, this->AcceptButton, true, true, 0);
	
	g_signal_connect_swapped((GObject*)this->ArgAddButton, "clicked", (GCallback)ArgAddButtonCallback, this);
	g_signal_connect_swapped((GObject*)this->CmdCodeChoiceButton, "clicked", (GCallback)CmdCodeChoiceButtonCallback, this);
	g_signal_connect_swapped((GObject*)this->AcceptButton, "clicked", (GCallback)AcceptPressedCallback, this);
	
	gtk_box_pack_start((GtkBox*)this->VBox, this->HBoxes.back().first, true, true, 0);
	
	//Populate the first HBox. It's not used like the others
	gtk_box_pack_start((GtkBox*)this->HBoxes.back().first, this->UserLabel, false, false, 0);
	gtk_box_pack_start((GtkBox*)this->HBoxes.back().first, this->StaticButtonsHBox, true, true, 0);
	
	gtk_box_pack_start((GtkBox*)this->VBox, this->UserLabel, false, false, 0);
	gtk_box_pack_start((GtkBox*)this->VBox, this->Separator, false, false, 0);

	this->HBoxes.push_back({gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0), 0u});
	gtk_box_pack_start((GtkBox*)this->VBox, this->HBoxes.back().first, true, true, 0);

	
	VLScopedPtr<std::vector<Conation::ArgType>*> PreExistingArgs = Stream ? Stream->GetArgTypes() : nullptr;

	if (!PreExistingArgs) goto NoPreExistingArgs;

	//Populate the greyed-out pre-existing args buttons.
	for (size_t Inc = 0; Inc < PreExistingArgs->size(); ++Inc, ++this->HBoxes.back().second)
	{
		if (this->HBoxes.back().second > (ArgSelectorDialog::MaxButtonsPerRow - 1))
		{
			this->HBoxes.push_back({gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0), 0u});
			gtk_box_pack_start((GtkBox*)this->VBox, this->HBoxes.back().first, true, false, 0);
			this->HBoxes.back().second = 0u;
		}
		
		GtkWidget *const OurHBox = this->HBoxes.back().first;
		
		GtkWidget *const NewButton = gtk_button_new_with_label(VLString::UintToString(this->ArgButtons.size() + 1)
																+ ": " + ArgTypeToString(PreExistingArgs->at(Inc)));
		
		gtk_button_set_image((GtkButton*)NewButton, gtk_image_new_from_icon_name("gtk-yes", GTK_ICON_SIZE_BUTTON));
		gtk_button_set_image_position((GtkButton*)NewButton, GTK_POS_RIGHT);

		gtk_widget_set_sensitive(NewButton, false);

		gtk_box_pack_start((GtkBox*)OurHBox, NewButton, false, false, 0);
		
		this->ArgButtons.push_back({NewButton, Stream->PopArgument()});
	}
	
	Stream->Rewind();
	
NoPreExistingArgs:

	if (RequiredParameters)
	{
		//Populate required argument buttons.
		for (size_t Inc = 0; Inc < RequiredParameters->size(); ++Inc, ++this->HBoxes.back().second)
		{
			if (this->HBoxes.back().second > (ArgSelectorDialog::MaxButtonsPerRow - 1))
			{
				this->HBoxes.push_back({gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0), 0u });
				gtk_box_pack_start((GtkBox*)this->VBox, this->HBoxes.back().first, true, false, 0);
			}
			
			GtkWidget *const OurHBox = this->HBoxes.back().first;
			
			GtkWidget *const NewButton = gtk_button_new_with_label(VLString::UintToString(this->ArgButtons.size() + 1)
																+ ": " + ArgTypeToString(RequiredParameters->at(Inc)));
			
			gtk_button_set_image((GtkButton*)NewButton, gtk_image_new_from_icon_name("gtk-no", GTK_ICON_SIZE_BUTTON));
			gtk_button_set_image_position((GtkButton*)NewButton, GTK_POS_RIGHT);

			gtk_box_pack_start((GtkBox*)OurHBox, NewButton, false, false, 0);
			
			g_signal_connect((GObject*)NewButton, "clicked", (GCallback)ArgButtonClickedCallback, this);
			
			this->ArgButtons.push_back({NewButton, nullptr});
			
		}
	}
	
	//Populate add button menu.
	for (Conation::ArgTypeUint Shift = 1; Shift && Shift <= Conation::ARGTYPE_MAXPOPULATED; Shift <<= 1)
	{
		if ((Shift & AcceptedOptionals) == 0)
		{
#ifdef DEBUG
			puts(VLString("GuiDialogs::ArgSelectorDialog::ArgSelectorDialog(): Not supported, arg ") + ArgTypeToString(static_cast<Conation::ArgType>(Shift)));
#endif
			continue; //Not permitted.
		}
#ifdef DEBUG
		else puts(VLString("GuiDialogs::ArgSelectorDialog::ArgSelectorDialog(): Supported, arg ") + ArgTypeToString(static_cast<Conation::ArgType>(Shift)));
#endif
		GtkWidget *NewItem = gtk_menu_item_new_with_label(ArgTypeToString(static_cast<Conation::ArgType>(Shift)));
		
		gtk_widget_show(NewItem); //Strangely, this is necessary. Wtf?
		
		gtk_menu_shell_append((GtkMenuShell*)this->ArgAddMenu, NewItem);
		
		g_signal_connect((GObject*)NewItem, "activate", (GCallback)ArgAddMenuCallback, this);
	}

	if (AllowCmdCodeChange)
	{
		//Populate command code changing menu.
		for (CommandCodeUint Inc = 1; Inc < CMDCODE_MAX; ++Inc)
		{
	
			GtkWidget *NewItem = gtk_menu_item_new_with_label(CommandCodeToString(static_cast<CommandCode>(Inc)));
			
			gtk_widget_show(NewItem);
			
			gtk_menu_shell_append((GtkMenuShell*)this->CmdCodeChoiceMenu, NewItem);
			
			g_signal_connect((GObject*)NewItem, "activate", (GCallback)CmdCodeChoiceMenuCallback, this);
		}
	}
	else
	{
		gtk_widget_set_sensitive((GtkWidget*)this->CmdCodeChoiceButton, FALSE);
	}
	
	if (!AcceptedOptionals)
	{ //No use anyways.
		gtk_widget_set_sensitive(this->ArgAddButton, false);
	}
	
	if (this->ArgButtons.size() > 0) gtk_widget_grab_focus(this->ArgButtons[0].first);
}

void GuiDialogs::ArgSelectorDialog::ArgAddMenuCallback(GtkWidget *MenuItem, ArgSelectorDialog *ThisPointer)
{
	const VLString Name = gtk_menu_item_get_label((GtkMenuItem*)MenuItem);
	
	const Conation::ArgType Type = StringToArgType(Name);

#ifdef DEBUG
	puts(VLString("GuiDialogs::ArgSelectorDialog::ArgAddMenuCallback(): Got arg type ") + Name);
#endif

	if (Type == Conation::ARGTYPE_NONE) //Invalid
	{
		return;
	}
	
	if (ThisPointer->HBoxes.back().second > (ArgSelectorDialog::MaxButtonsPerRow - 1))
	{
		ThisPointer->HBoxes.push_back({gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0), 0u});
		gtk_box_pack_start((GtkBox*)ThisPointer->VBox, ThisPointer->HBoxes.back().first, true, false, 0);
	}
	
	++ThisPointer->HBoxes.back().second;
	
	GtkWidget *const OurHBox = ThisPointer->HBoxes.back().first;
	
	GtkWidget *const NewButton = gtk_button_new_with_label(VLString::UintToString(ThisPointer->ArgButtons.size() + 1)
															+ ": " + Name);

	gtk_button_set_image((GtkButton*)NewButton, gtk_image_new_from_icon_name("gtk-no", GTK_ICON_SIZE_BUTTON));
	gtk_button_set_image_position((GtkButton*)NewButton, GTK_POS_RIGHT);
	gtk_box_pack_start((GtkBox*)OurHBox, NewButton, false, false, 0);
	
	g_signal_connect((GObject*)NewButton, "clicked", (GCallback)ArgButtonClickedCallback, ThisPointer);

	ThisPointer->ArgButtons.push_back({NewButton, nullptr});
	
	gtk_widget_set_sensitive(ThisPointer->AcceptButton, false);
	
	ThisPointer->Show(); //Necessary. We're only ever gonna reach here if we're already visible anyways.
}

GuiDialogs::ArgSelectorDialog::~ArgSelectorDialog(void)
{
	for (size_t Inc = 0; Inc < this->ArgButtons.size(); ++Inc)
	{
		delete this->ArgButtons[Inc].second;
	}
	
	for (size_t Inc = 0; Inc < this->ValueDialogs.size(); ++Inc)
	{
		delete this->ValueDialogs[Inc].first;
	}
}

void GuiDialogs::ArgSelectorDialog::ArgAddButtonCallback(ArgSelectorDialog *ThisPointer)
{
	gtk_menu_popup_at_widget((GtkMenu*)ThisPointer->ArgAddMenu, ThisPointer->ArgAddButton, {}, {}, nullptr);
}

void GuiDialogs::ArgSelectorDialog::CmdCodeChoiceButtonCallback(ArgSelectorDialog *ThisPointer)
{
	gtk_menu_popup_at_widget((GtkMenu*)ThisPointer->CmdCodeChoiceMenu, ThisPointer->CmdCodeChoiceButton, {}, {}, nullptr);
}

void GuiDialogs::ArgSelectorDialog::ArgButtonClickedCallback(GtkWidget *Button, ArgSelectorDialog *ThisPointer)
{
	VLString CountString = gtk_button_get_label((GtkButton*)Button);
	
	char *Seek = const_cast<char*>(strchr(CountString, ':'));
	
	if (!Seek) return;
	
	*Seek = '\0';
	
	Seek += sizeof ": " - 1;
	
	
	const Conation::ArgType Type = StringToArgType(Seek);
	
	
	if (!Utils::StringAllNumeric(CountString) || Type == Conation::ARGTYPE_NONE) return;
	
	size_t Count = VLString::StringToUint(CountString);
	
	if (!Count) return;
	Count -= 1;
	
	if (ThisPointer->ArgButtons[Count].first != Button)
	{
#ifdef DEBUG
		puts("GuiDialogs::ArgSelectorDialog::ArgButtonClickedCallback(): Recalculated button indice in ArgButtons doesn't match! Aborting!");
#endif
		return;
	}

#ifdef DEBUG
	puts("GuiDialogs::ArgSelectorDialog::ArgButtonClickedCallback(): Successfully recalculated button position.");
#endif

	DialogBase *NewDialog = nullptr;
	
	ThisPointer->ValueDialogs.push_back({ nullptr, {} });
	
	ValueDialogPassStruct &Struct = ThisPointer->ValueDialogs.back().second;

	switch (Type)
	{
		case Conation::ARGTYPE_STRING:
		{
			NewDialog = new LargeTextDialog("Enter a string", "Enter a string for the value of the selected button.",
											(LargeTextDialog::OnCompleteFunc)ValueDialogCompletedCallback,
											&Struct);
			break;
		}
		case Conation::ARGTYPE_FILEPATH:
		{
			NewDialog = new SimpleTextDialog("Enter a file path", "Enter a file path for the value of the selected button.",
											(SimpleTextDialog::OnCompleteFunc)ValueDialogCompletedCallback,
											&Struct);
			break;
		}
		case Conation::ARGTYPE_INT32:
		case Conation::ARGTYPE_INT64:
		case Conation::ARGTYPE_UINT32:
		case Conation::ARGTYPE_UINT64:
		{
			NewDialog = new SimpleTextDialog("Enter an integer", "Enter an integer for the value of the selected button.",
											(SimpleTextDialog::OnCompleteFunc)ValueDialogCompletedCallback,
											&Struct);
			break;
		}
		case Conation::ARGTYPE_BOOL:
		{
			NewDialog = new YesNoDialog("Select value", "Select the boolean value for the value of the selected button.",
										true,  (YesNoDialog::OnCompleteFunc)ValueDialogCompletedCallback,
										&Struct);
			break;
		}
		case Conation::ARGTYPE_ODHEADER:
		{
			NewDialog = new SimpleTextDialog("Enter an ODHeader", "Enter an ODHeader in the format "
											"e.g. \"MyOrigin,MyDestination\" for the value of the selected button.",
											(SimpleTextDialog::OnCompleteFunc)ValueDialogCompletedCallback,
											&Struct);
			break;
		}
		case Conation::ARGTYPE_NETCMDSTATUS:
		{
			NewDialog = new SimpleTextDialog("Enter a NetCmdStatus", "Enter a NetCmdStatus in the format "
											"e.g. \"STATUS_OK,My personalized message\" for the value of the selected button.",
											(SimpleTextDialog::OnCompleteFunc)ValueDialogCompletedCallback,
											&Struct);
			break;
		}
		case Conation::ARGTYPE_SCRIPT:
		{
			NewDialog = new FileSelectionDialog("Choose a script file", "Choose a script file for the value of the selected button.",
												false, (FileSelectionDialog::OnCompleteFunc)ValueDialogCompletedCallback,
												&Struct);
			break;
		}
		case Conation::ARGTYPE_FILE:
		{
			NewDialog = new FileSelectionDialog("Choose a file", "Choose a file for the value of the selected button.",
												false, (FileSelectionDialog::OnCompleteFunc)ValueDialogCompletedCallback,
												&Struct);
			break;
		}
		case Conation::ARGTYPE_BINSTREAM:
		{
			NewDialog = new FileSelectionDialog("Choose a file", "Choose a file to use as a binary stream for the value of the selected button.",
												false, (FileSelectionDialog::OnCompleteFunc)ValueDialogCompletedCallback,
												&Struct);
			break;
		}
		default:
			break;
	}
	
	Struct.ThisValueDialog = ThisPointer->ValueDialogs.back().first = NewDialog;
	Struct.ThisPointer = ThisPointer;
	Struct.ArgButtonsElement = &ThisPointer->ArgButtons[Count];
	
	NewDialog->Show();
}

void GuiDialogs::ArgSelectorDialog::ValueDialogCompletedCallback(const void *UserData)
{
	const ArgSelectorDialog::ValueDialogPassStruct *Struct = static_cast<const ValueDialogPassStruct*>(UserData);
	
	std::pair<GtkWidget*, Conation::ConationStream::BaseArg*> *ArgButtonsElement = Struct->ArgButtonsElement;
	
	//We're using the button label itself to get the info we need. Interesting huh?
	const Conation::ArgType Type = StringToArgType(strchr(gtk_button_get_label((GtkButton*)ArgButtonsElement->first), ' ') + 1);
	
	if (Type == Conation::ARGTYPE_NONE)
	{
#ifdef DEBUG
		puts("GuiDialogs::ArgSelectorDialog::ValueDialogCompletedCallback(): Type is invalid, did you weird up the button labels?");
#endif
		return;
	}
	
	//Reference to a pointer, lol
	Conation::ConationStream::BaseArg *&NewValue = ArgButtonsElement->second;
	
	if (NewValue)
	{ //Replacing an old one?
		delete NewValue;
	}
	
	NewValue = Conation::ConationStream::NewArgStructFromType(Type);

	switch (Type)
	{
		case Conation::ARGTYPE_BOOL:
		{
			YesNoDialog *Dialog = static_cast<YesNoDialog*>(Struct->ThisValueDialog);
			

			static_cast<Conation::ConationStream::BoolArg*>(NewValue)->Value = Dialog->GetValue();
			break;
		}
		case Conation::ARGTYPE_STRING:
		{ //We assume here that we always want a multiline dialog.
			LargeTextDialog *Dialog = static_cast<LargeTextDialog*>(Struct->ThisValueDialog);
			
			static_cast<Conation::ConationStream::StringArg*>(NewValue)->String = Dialog->GetTextData();
			break;
		}
		case Conation::ARGTYPE_FILEPATH:
		{ //We assume here that we always want a multiline dialog.
			SimpleTextDialog *Dialog = static_cast<SimpleTextDialog*>(Struct->ThisValueDialog);
			
			static_cast<Conation::ConationStream::StringArg*>(NewValue)->String = Dialog->GetTextData();
			break;
		}
		case Conation::ARGTYPE_INT32:
		{
			SimpleTextDialog *Dialog = static_cast<SimpleTextDialog*>(Struct->ThisValueDialog);
			
			const VLString &NumText = Dialog->GetTextData();
			const int32_t Num = VLString::StringToInt(NumText);
			
			static_cast<Conation::ConationStream::Int32Arg*>(NewValue)->Value = Num;
			break;
		}
		case Conation::ARGTYPE_UINT32:
		{
			SimpleTextDialog *Dialog = static_cast<SimpleTextDialog*>(Struct->ThisValueDialog);
			
			const VLString &NumText = Dialog->GetTextData();
			const int32_t Num = VLString::StringToUint(NumText);
			
			static_cast<Conation::ConationStream::Uint32Arg*>(NewValue)->Value = Num;
			break;
		}
		case Conation::ARGTYPE_INT64:
		{
			SimpleTextDialog *Dialog = static_cast<SimpleTextDialog*>(Struct->ThisValueDialog);
			
			const VLString &NumText = Dialog->GetTextData();
			const int64_t Num = VLString::StringToInt(NumText);
			
			static_cast<Conation::ConationStream::Int64Arg*>(NewValue)->Value = Num;
			break;
		}
		case Conation::ARGTYPE_UINT64:
		{
			SimpleTextDialog *Dialog = static_cast<SimpleTextDialog*>(Struct->ThisValueDialog);
			
			const VLString &NumText = Dialog->GetTextData();
			const int64_t Num = VLString::StringToUint(NumText);
			
			static_cast<Conation::ConationStream::Uint64Arg*>(NewValue)->Value = Num;
			break;
		}
		case Conation::ARGTYPE_SCRIPT:
		{
			FileSelectionDialog *Dialog = static_cast<FileSelectionDialog*>(Struct->ThisValueDialog);
			
			uint64_t FileSize = 0;
			
			const VLString &Path = Dialog->GetPaths()[0];
			
			if (!Utils::GetFileSize(Path, &FileSize))
			{
#ifdef DEBUG
				puts("GuiDialogs::ArgSelectorDialog::ValueDialogCompletedCallback(): Unable to get file size of script path. Aborting.");
#endif
				delete NewValue;
				NewValue = nullptr;
				return;
			}
			
			VLString ScriptBuf(FileSize + 1);
			
			if (!Utils::Slurp(Path, ScriptBuf.GetBuffer(), FileSize + 1))
			{
#ifdef DEBUG
				puts("GuiDialogs::ArgSelectorDialog::ValueDialogCompletedCallback(): Unable to slurp script file!");
#endif
				delete NewValue;
				NewValue = nullptr;
				return;
			}
			
			static_cast<Conation::ConationStream::ScriptArg*>(NewValue)->String = ScriptBuf;
			
			break;
		}
		case Conation::ARGTYPE_ODHEADER:
		{
			SimpleTextDialog *Dialog = static_cast<SimpleTextDialog*>(Struct->ThisValueDialog);
			
			VLString Text = Dialog->GetTextData();
			
			char *Seek = const_cast<char*>(strchr(Text, ','));
			
			if (!Seek)
			{
#ifdef DEBUG
				puts("GuiDialogs::ArgSelectorDialog::ValueDialogCompletedCallback(): ODHeader entered improperly. Aborting.");
#endif
				delete NewValue;
				NewValue = nullptr;
				return;
			}
			
			*Seek = '\0';
			++Seek;
			
			static_cast<Conation::ConationStream::ODHeaderArg*>(NewValue)->Hdr.Origin = Text;
			static_cast<Conation::ConationStream::ODHeaderArg*>(NewValue)->Hdr.Destination = Seek;
			
			break;
		}
		case Conation::ARGTYPE_NETCMDSTATUS:
		{
			SimpleTextDialog *Dialog = static_cast<SimpleTextDialog*>(Struct->ThisValueDialog);
			
			VLString Text = Dialog->GetTextData();
			
			char *Seek = const_cast<char*>(strchr(Text, ','));
			
			if (Seek) *Seek++ = '\0';
			
			const StatusCode Code = StringToStatusCode(Text);
			
			if (Code == STATUS_INVALID)
			{
#ifdef DEBUG
				puts("GuiDialogs::ArgSelectorDialog::ValueDialogCompletedCallback(): NetCmdStatus entered improperly. Aborting.");
#endif
				delete NewValue;
				NewValue = nullptr;
				return;
			}
			
			const VLString Msg = Seek;
			
			static_cast<Conation::ConationStream::NetCmdStatusArg*>(NewValue)->Code = Code;
			static_cast<Conation::ConationStream::NetCmdStatusArg*>(NewValue)->Msg = Msg;
			
			break;
		}
		case Conation::ARGTYPE_BINSTREAM:
		case Conation::ARGTYPE_FILE:
		{
			FileSelectionDialog *Dialog = static_cast<FileSelectionDialog*>(Struct->ThisValueDialog);
			
			const VLString Path = Dialog->GetPaths()[0];
			
			uint64_t FileSize = 0;
			
			uint8_t *Buf = nullptr;
			
			//Suck my comma operator.
			if (!Utils::GetFileSize(Path, &FileSize) || ((Buf = new uint8_t[FileSize]), !Utils::Slurp(Path, Buf, FileSize)))
			{
#ifdef DEBUG
				puts(VLString("GuiDialogs::ArgSelectorDialog::ValueDialogCompletedCallback(); Failed to slurp file path ") + Path);
#endif
				delete NewValue;
				NewValue = nullptr;
				return;
			}
			
			if (NewValue->Type == Conation::ARGTYPE_FILE)
			{
				static_cast<Conation::ConationStream::FileArg*>(NewValue)->Filename = Utils::StripPathFromFilename(Path);
			}
			
			static_cast<Conation::ConationStream::BinStreamArg*>(NewValue)->Data = Buf;
			static_cast<Conation::ConationStream::BinStreamArg*>(NewValue)->DataSize = FileSize;
			static_cast<Conation::ConationStream::BinStreamArg*>(NewValue)->MustFree = true;
			
			break;
		}
		default:
			break;
	}
	
	//GtkWidget *OldImage = gtk_button_get_image((GtkButton*)ArgButtonsElement->first);
	
	gtk_button_set_image((GtkButton*)ArgButtonsElement->first, gtk_image_new_from_icon_name("gtk-yes", GTK_ICON_SIZE_BUTTON));
	
	if (Struct->ThisPointer->AllValuesFilled())
	{
		gtk_widget_set_sensitive(Struct->ThisPointer->AcceptButton, true);
	}
	
	VLString ButtonCountString = gtk_button_get_label((GtkButton*)ArgButtonsElement->first);

	*strchr(ButtonCountString.GetBuffer(), ':') = '\0';
	
	const size_t ButtonOffset = VLString::StringToUint(ButtonCountString) - 1;
	
	if (Struct->ThisPointer->ArgButtons[ButtonOffset].first != ArgButtonsElement->first)
	{
#ifdef DEBUG
		puts("GuiDialogs::ArgSelectorDialog::ValueDialogCompletedCallback(): Failed to change button focus because cannot recalculate button position.");
#endif
		return;
	}
	
	if (ButtonOffset + 1 == Struct->ThisPointer->ArgButtons.size())
	{ //Last button, select the accept button.
		gtk_widget_grab_focus(Struct->ThisPointer->AcceptButton);
	}
	else
	{
		gtk_widget_grab_focus(Struct->ThisPointer->ArgButtons[ButtonOffset + 1].first);
	}
	
	///Now why the fuck is this saying it's not a widget?
	//gtk_widget_destroy(OldImage);
}

bool GuiDialogs::ArgSelectorDialog::AllValuesFilled(void) const
{
	for (size_t Inc = 0; Inc < this->ArgButtons.size(); ++Inc)
	{
		if (!this->ArgButtons[Inc].second) return false;
	}

	return true;
}

std::vector<Conation::ConationStream::BaseArg*> *GuiDialogs::ArgSelectorDialog::GetValues(void) const
{
	std::vector<Conation::ConationStream::BaseArg*> *RetVal = new std::vector<Conation::ConationStream::BaseArg*>;
	
	RetVal->reserve(this->ArgButtons.size());
	
	for (size_t Inc = 0; Inc < this->ArgButtons.size(); ++Inc)
	{
		RetVal->push_back(this->ArgButtons[Inc].second);
	}
	
	return RetVal;
}

Conation::ConationStream *GuiDialogs::ArgSelectorDialog::CompileToStream(void) const
{
	Conation::ConationStream *RetVal = new Conation::ConationStream(this->Header, nullptr); //This should be safe as long as Hdr.StreamArgsSize is zero.
	
	for (size_t Inc = 0; Inc < this->ArgButtons.size(); ++Inc)
	{
		RetVal->PushArgument(this->ArgButtons[Inc].second);
	}
	
	return RetVal;
}

void *GuiDialogs::ArgSelectorDialog::AcceptPressedCallback(ArgSelectorDialog *ThisPointer)
{
	ThisPointer->Hide();
	ThisPointer->Dismissed = true;
	
	if (ThisPointer->Func) ThisPointer->Func(ThisPointer->UserData, ThisPointer);
	
	return nullptr;
}

void GuiDialogs::ArgSelectorDialog::CmdCodeChoiceMenuCallback(GtkWidget *MenuItem, ArgSelectorDialog *ThisPointer)
{
	const VLString NewCodeText = gtk_menu_item_get_label((GtkMenuItem*)MenuItem);
	const CommandCode NewCode = StringToCommandCode(NewCodeText);

	ThisPointer->Header.CmdCode = NewCode;

	gtk_button_set_label((GtkButton*)ThisPointer->CmdCodeChoiceButton, +NewCodeText);
}

GuiDialogs::AboutDialog::AboutDialog(void)
	: DialogBase(DIALOG_ABOUT, "About Volition Control"),
	VBox(gtk_box_new(GTK_ORIENTATION_VERTICAL, 8)),
	Label(gtk_label_new("")),
	Icon(gtk_image_new_from_pixbuf(GuiBase::LoadImageToPixbuf(GuiIcons::LookupIcon("ctlabout").data(), GuiIcons::LookupIcon("ctlabout").size()))),
	Button(gtk_button_new_with_mnemonic("_Dismiss"))
{
	const VLString AboutString
	{
		VLString() +
		"Volition Control Program -- revision " CTL_REVISION " using Conation protocol version " + VLString::IntToString(Conation::PROTOCOL_VERSION) + "\n\n"
		
		"Volition is highly advanced software for controlling a network of subordinate computers.\n\n"
		
		"Volition is free software: you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation, either version 3 of the License, or\n"
		"(at your option) any later version.\n\n"
		
		"Volition is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		"GNU General Public License for more details.\n\n"
		
		"You should have received a copy of the GNU General Public License\n"
		"along with Volition.  If not, see <https://www.gnu.org/licenses/>."

	};
	gtk_label_set_text((GtkLabel*)this->Label, AboutString);
	
	gtk_container_add((GtkContainer*)this->Window, this->VBox);
	
	gtk_widget_set_halign(this->Button, GTK_ALIGN_END);
	
	gtk_box_pack_start((GtkBox*)this->VBox, this->Icon, true, true, 0);
	gtk_box_pack_start((GtkBox*)this->VBox, this->Label, true, true, 0);
	gtk_box_pack_start((GtkBox*)this->VBox, this->Button, true, true, 0);
	
	void* (*DismissedCallback)(AboutDialog *) = [](AboutDialog *ThisPointer)->void*
	{
		ThisPointer->Dismissed = true;
		ThisPointer->Hide();
		return nullptr;
	};
	
	g_signal_connect_swapped((GObject*)this->Button, "clicked", (GCallback)DismissedCallback, this);
	
}
