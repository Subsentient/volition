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


#ifndef VL_CTL_GUIDIALOGS_H
#define VL_CTL_GUIDIALOGS_H

#include "../libvolition/include/common.h"
#include "../libvolition/include/conation.h"
#include <gtk/gtk.h>
#include <vector>
#include <list>
#include <utility>
#include <tuple>

namespace GuiDialogs
{
	enum DialogType : uint8_t
	{
		DIALOG_NONE,
		DIALOG_SIMPLETEXT,
		DIALOG_LARGETEXT,
		DIALOG_FILECHOOSER,
		DIALOG_TEXTDISPLAY,
		DIALOG_YESNODIALOG,
		DIALOG_CMDSTATUS,
		DIALOG_PROGRESSDIALOG,
		DIALOG_ARGSELECTOR,
		DIALOG_BINARYFLAGS,
		DIALOG_ABOUT,
		DIALOG_MAX
	};

	class DialogBase
	{ //Some things are interoperable with the GuiBase ScreenObj, but most things are NOT.
	private:
		DialogBase &operator=(const DialogBase &Ref);
		DialogBase(const DialogBase &Ref);

		DialogType Type; //What kind of dialog it is
	public:		
		DialogBase(const DialogType Type, const VLString &Title);
		virtual ~DialogBase(void);
		virtual void Show(void);
		virtual void Hide(void);
		
		inline DialogType GetDialogType(void) const { return this->Type; }
		inline bool GetDismissed(void) const { return this->Dismissed; }
		
		typedef void (*OnCompleteFunc)(const void *UserData); //Make sure we can safely cast to this and have it work.

	protected:
		bool Dismissed;
		GtkWidget *Window;
	};


	class SimpleTextDialog : public DialogBase
	{
	public:
		typedef void (*OnCompleteFunc)(const void *UserData, const VLString &TextEntryResult);

	private:
		GtkWidget *Grid;
		GtkWidget *Label;
		GtkWidget *TextEntry;
		GtkWidget *Button;
		GtkAccelGroup *AccelGroup;
		OnCompleteFunc Func;
		const void *UserData;
		VLString TextData;
		
		SimpleTextDialog &operator=(const SimpleTextDialog &Ref);
		SimpleTextDialog(const SimpleTextDialog &Ref);
		static void *Callback(SimpleTextDialog *ThisPointer);
	public:
		inline const VLString &GetTextData(void) const { return this->TextData; }
		SimpleTextDialog(const VLString &Title, const VLString &Prompt, OnCompleteFunc Func, const void *PassAlongWith);

	};

	class LargeTextDialog : public DialogBase
	{
	public:
		typedef void (*OnCompleteFunc)(const void *UserData, const char *Text);

	private:
		GtkWidget *Grid;
		GtkWidget *Label;
		GtkWidget *Separator;
		GtkWidget *ScrolledWindow;
		GtkWidget *TextView;
		GtkWidget *Button;
		OnCompleteFunc Func;
		const void *UserData;
		VLString TextData;
		
		LargeTextDialog &operator=(const LargeTextDialog &Ref);
		LargeTextDialog(const LargeTextDialog &Ref);
		static void *Callback(LargeTextDialog *ThisPointer);

	public:
		inline const VLString &GetTextData(void) const { return this->TextData; }
		LargeTextDialog(const VLString &Title, const VLString &Prompt, OnCompleteFunc Func, const void *UserData);

	};


	class FileSelectionDialog : public DialogBase
	{
	public:
		typedef void (*OnCompleteFunc)(const void *UserData, const std::vector<VLString> *Paths);
	private:
		GtkWidget *VBox;
		GtkWidget *Label;
		GtkWidget *Separator1;
		GtkWidget *BrowseButton;
		GtkWidget *CounterLabel;
		GtkWidget *Separator2;
		GtkWidget *AcceptButton;

		std::vector<VLString> CurrentPaths; //Where we store what's currently selected in the file chooser.
		
		OnCompleteFunc Func;
		const void *UserData;
		bool AllowMultiple; //Allow multiple file selections?
		
		FileSelectionDialog &operator=(const FileSelectionDialog &);
		FileSelectionDialog(const FileSelectionDialog &);

		static void *AcceptPressedCallback(FileSelectionDialog *ThisPointer);
		static void *BrowsePressedCallback(FileSelectionDialog *ThisPointer);
		
	public:
		inline const std::vector<VLString> &GetPaths(void) const { return this->CurrentPaths; }
		FileSelectionDialog(const VLString &Title, const VLString &Prompt, const bool AllowMultiple, OnCompleteFunc Func, const void *UserData);
	};

	class TextDisplayDialog : public DialogBase
	{
	public:
		typedef void (*OnCompleteFunc)(const void *UserData);
	private:
		GtkWidget *VBox;
		GtkWidget *Label;
		GtkWidget *Separator1;
		GtkWidget *ScrolledWindow;
		GtkWidget *TextView;
		GtkWidget *Separator2;
		GtkWidget *CloseButton;
		GtkAccelGroup *AccelGroup;
		
		VLString Data;

		OnCompleteFunc Func;
		const void *UserData;

		TextDisplayDialog &operator=(const TextDisplayDialog &);
		TextDisplayDialog(const TextDisplayDialog &);

		static void *ClosePressedCallback(TextDisplayDialog *ThisPointer);

	public:
		TextDisplayDialog(  const VLString &WindowTitle,
							const VLString &LabelText,
							const char *Data,
							OnCompleteFunc CallbackFunc = nullptr,
							const void *UserData = nullptr);
	};

	class AboutDialog : public DialogBase
	{
	private:
		GtkWidget *VBox;
		GtkWidget *Label;
		GtkWidget *Icon;
		GtkWidget *Button;

		AboutDialog &operator=(const AboutDialog &);
		AboutDialog(const AboutDialog &);
	public:
		AboutDialog(void);
	};
		
	
	class YesNoDialog : public DialogBase
	{
	public:
		typedef void (*OnCompleteFunc)(const void *UserData, const bool Value);
	private:
		GtkWidget *Grid;
		GtkWidget *Label;
		GtkWidget *Separator;
		GtkWidget *YesButton;
		GtkWidget *NoButton;
		GtkAccelGroup *AccelGroup;
		
		bool Value;
		
		OnCompleteFunc Func;
		const void *UserData;
		
		YesNoDialog &operator=(const YesNoDialog &);
		YesNoDialog(const YesNoDialog &);
		
		static void *ChoiceMadeCallback(GtkWidget *ChosenButton, YesNoDialog *ThisPointer);
	public:
		inline bool GetValue(void) const { return this->Value; }
		YesNoDialog(const VLString &WindowTitle, const VLString &Prompt, const bool UseBooleanNames, OnCompleteFunc CallbackFunc = nullptr, const void *UserData = nullptr);
	};
	
	class CmdStatusDialog : public DialogBase
	{
	public:
		typedef void (*OnCompleteFunc)(const void *UserData);
		
	private:
		GtkWidget *Grid;
		GtkWidget *Label;
		GtkWidget *StatusIcon;
		GtkWidget *Button;
		GtkAccelGroup *AccelGroup;
		
		OnCompleteFunc Func;
		const void *UserData;
		
		CmdStatusDialog &operator=(const CmdStatusDialog&);
		CmdStatusDialog(const CmdStatusDialog &);
		static void *DismissedCallback(CmdStatusDialog *ThisPointer);

	public:
		CmdStatusDialog(const VLString &WindowTitle, const NetCmdStatus &Status, OnCompleteFunc CallbackFunc = nullptr, const void *UserData = nullptr);
	};
	
	class ProgressDialog : public DialogBase
	{
	public:
		typedef void (*OnCompleteFunc)(const void *UserData);
	private:
		GtkWidget *VBox;
		GtkWidget *Label;
		GtkWidget *ProgressBar;
		GtkWidget *DismissButton;
		
		bool UserCanDismiss;
		
		OnCompleteFunc Func;
		const void *UserData;
		
		ProgressDialog(const ProgressDialog &);
		ProgressDialog &operator=(const ProgressDialog &);
		static void *CompletionCallback(ProgressDialog *CompletionCallback);
		
	public:
		ProgressDialog(const VLString &WindowTitle, const VLString &LabelText, const VLString &ProgressBarText, const bool UserCanDismiss, OnCompleteFunc CallbackFunc = nullptr, const void *UserData = nullptr);
		
		void SetBarText(const char *NewText);
		void SetLabelText(const char *NewText);
		void Pulse(void);
		double GetFraction(void) const;
		void SetFraction(const double Value);
		void IncreaseFraction(const double Value);
		void SetPulseFraction(const double Value);
		void ForceComplete(void);
	};
	
	class ArgSelectorDialog : public DialogBase
	{ //This class is absolute cancer, from head to toe. It's among the worst code I've ever written.
		//It'll give your hernia's aneurysm a seizure.
	public:
		typedef void (*OnCompleteFunc)(const void *UserData, const ArgSelectorDialog *ThisPointer);
		static const size_t MaxButtonsPerRow = 4u;		
	private:
		struct ValueDialogPassStruct
		{
			ArgSelectorDialog *ThisPointer;
			std::pair<GtkWidget*, Conation::ConationStream::BaseArg*> *ArgButtonsElement;
			DialogBase *ThisValueDialog;
		};
		
		///Data members.
		GtkWidget *VBox;
		GtkWidget *UserLabel;
		GtkWidget *Separator;
		GtkWidget *AcceptButton;
		GtkWidget *ArgAddMenu;
		GtkWidget *ArgAddButton;
		GtkWidget *StaticButtonsHBox;
		GtkWidget *CmdCodeChoiceButton;
		GtkWidget *CmdCodeChoiceMenu;
		
		//Widget arrays
		std::vector<std::pair<GtkWidget*, size_t> > HBoxes;
		
		std::vector<std::pair<GtkWidget*, Conation::ConationStream::BaseArg*> > ArgButtons;
		
		Conation::ConationStream *OriginalStream;
		Conation::ConationStream::StreamHeader Header;
		
		std::vector<std::pair<DialogBase*, ValueDialogPassStruct> > ValueDialogs;
		OnCompleteFunc Func;
		
		const void *UserData;
		
		//Private helper functions
		static void ArgAddButtonCallback(ArgSelectorDialog *ThisPointer);
		static void ArgAddMenuCallback(GtkWidget *MenuItem, ArgSelectorDialog *ThisPointer);
		static void ArgButtonClickedCallback(GtkWidget *ArgButton, ArgSelectorDialog *ThisPointer);
		static void CmdCodeChoiceButtonCallback(ArgSelectorDialog *ThisPointer);
		static void CmdCodeChoiceMenuCallback(GtkWidget *MenuItem, ArgSelectorDialog *ThisPointer);
		static void *AcceptPressedCallback(ArgSelectorDialog *ThisPointer);
		static void ValueDialogCompletedCallback(const void *UserData);
		
		static GuiDialogs::DialogBase *NewDialogForType(const Conation::ArgType Type);
		
		ArgSelectorDialog(const ArgSelectorDialog &) = delete;
		ArgSelectorDialog(ArgSelectorDialog &&) = delete;
		ArgSelectorDialog &operator=(const ArgSelectorDialog &) = delete;
		
		bool AllValuesFilled(void) const;
		inline void SetUserData(const void *NewUserData) { this->UserData = NewUserData; }
	public:
		ArgSelectorDialog(const VLString &WindowTitle, const VLString &LabelText,
						Conation::ConationStream *Stream,
						const std::vector<Conation::ArgType> *RequiredParameters = nullptr, //Idk why you'd want this, but why not
						const Conation::ArgType AcceptedOptionals = Conation::ARGTYPE_NONE,
						const OnCompleteFunc InFunc = nullptr,
						const void *InUserData = nullptr,
						const bool AllowCmdCodeChange = false);
		~ArgSelectorDialog(void);
		
		std::vector<Conation::ConationStream::BaseArg*> *GetValues(void) const;
		Conation::ConationStream *CompileToStream(void) const;
		Conation::ConationStream *GetOriginalStream(void) const { return this->OriginalStream; }
		Conation::ConationStream::StreamHeader GetHeader(void) const { return this->Header; }
	};

	class BinaryFlagsDialog : public DialogBase
	{
	public:
		typedef void (*OnCompleteFunc)(const void *UserData, BinaryFlagsDialog *ThisPointer);
	private:
		static const size_t MaxButtonsPerRow = 4;
		
		GtkWidget *VBox;
		GtkWidget *Label;
		GtkWidget *Separator;
		GtkWidget *AcceptButton;
		
		std::vector<std::pair<GtkWidget*, uint64_t> > ToggleButtons;

		OnCompleteFunc Func;
		const void *UserData;
		
		BinaryFlagsDialog(BinaryFlagsDialog &) = delete;
		BinaryFlagsDialog &operator=(BinaryFlagsDialog &) = delete;

		static void AcceptPressedCallback(BinaryFlagsDialog *ThisPointer);
	public:
		BinaryFlagsDialog(const VLString &WindowTitle,
						const VLString &LabelText,
						const std::vector<std::tuple<VLString, uint64_t, bool> > &FlagSpecs,
						const OnCompleteFunc InFunc = nullptr,
						const void *InUserData = nullptr);
						
		uint64_t GetFlagValues(void) const;
	};
	
}
#endif //VL_CTL_GUIDIALOGS_H
