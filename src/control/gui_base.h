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


#ifndef VL_CTL_GUIBASE_H
#define VL_CTL_GUIBASE_H

#include <stdint.h>
#include <stddef.h>
#include <gtk/gtk.h>

#include <list>

#include "../libvolition/include/common.h"

namespace GuiBase
{
	//Types
	class ScreenObj
	{
	public:
		enum class ScreenType { BASE, LOGIN, LOADING, MAINWINDOW };
	private:
		ScreenType ObjType;
		ScreenObj &operator=(const ScreenObj &Ref);
		ScreenObj(const ScreenObj &Ref);
	public:
		virtual ~ScreenObj(void);
		ScreenObj(GtkWidget *InWindow, const ScreenType InType);
		inline ScreenType GetScreenType(void) const { return this->ObjType; }

		virtual void Show(void);
		virtual void Hide(void);
	protected:
		GtkWidget *Window;
	};
		
	class LoadingScreen : public ScreenObj
	{
	private:
		GtkWidget *Icon;
		GtkWidget *ProgressBar;
		GtkWidget *VBox;

		//Prevent operations we shouldn't be doing.
		LoadingScreen &operator=(const LoadingScreen &Ref);
		LoadingScreen(const LoadingScreen &Ref);
	public:
		LoadingScreen(const VLString &Message = "Starting Volition Control");
		void SetStatusText(const char *Text);
		void SetProgressBarFraction(const double Value);
		void SetProgressBarPulseFraction(const double Value);
		void IncreaseProgressBarFraction(const double ToAdd);
		double GetProgressBarFraction(void);
		void PulseProgressBar(void);
	};
	
	class LoginScreen : public ScreenObj
	{
	private:
		GtkWidget *Icon;
		GtkWidget *Grid;
		GtkAccelGroup *AccelGroup;
		GtkWidget *GreetingLabel;
		GtkWidget *UsernameLabel;
		GtkWidget *PasswordLabel;
		GtkWidget *UsernameBox;
		GtkWidget *PasswordBox;
		GtkWidget *ConnectButton;

		//Prevent operations we shouldn't be doing.
		LoginScreen &operator=(const LoginScreen &Ref);
		LoginScreen(const LoginScreen &Ref);
	public:
		LoginScreen(void);
		VLString GetUsernameField(void) const;
		VLString GetPasswordField(void) const;
	};


	class ScreenRegistry
	{ //Doesn't allow two screens of the same type to exist there.
		std::map<ScreenObj::ScreenType, ScreenObj*> Screens;
		bool Add(ScreenObj *Screen);
		bool Delete(ScreenObj *Screen);

	public:
		ScreenObj *Lookup(const ScreenObj::ScreenType Type) const;
		friend class ScreenObj;
		friend bool PurgeScreen(const ScreenObj::ScreenType);
	};
			
	//Prototypes	
	GdkPixbuf *LoadImageToPixbuf(const uint8_t *FileBuffer, const size_t Size);
	void NukeContainerChildren(GtkContainer *Container);
	void NukeWidget(GtkWidget *Widgy);
	void ShutdownCallback(GtkWidget *Widget, gpointer Stuff);
	VLString MapStatusToIcon(const NetCmdStatus &Status);

	ScreenObj *LookupScreen(const ScreenObj::ScreenType Type);
	bool PurgeScreen(const ScreenObj::ScreenType Type);
}
#endif //VL_CTL_GUIBASE_H
