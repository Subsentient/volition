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


#ifndef VL_CTL_ORDERS_H
#define VL_CTL_ORDERS_H

/*
                                                              `.-:////:-.                           
                                                       .:+shmNMMMMMMMMMMMMdy:                       
                                                 `-+ydMMMMMMMMMMMMMMMMMMMMMMMN/                     
                                            `/sdNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMh`                   
                                          -hMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMm-                  
                                         -NMMMMMMNMMMNNNNNNNNNNNMMMMMMMMMMMMMMMMMN-                 
                                         sMMMMMNMNNNNNNNNNNNNNNNNMMMMMMMMMMMMMMMMMN.                
                                         sNNNNNNNNNNNNNNNNNNNNNNMMMMMMMMMMMMMMMMMMMNysooo+/-        
                                         oNNNNNNNNNNNNNNNNNNNNNMMMMMMMMMMMMMMMMMMMMMMMMMMMMM-       
                                         mNNmNNNNNNNNNNNNNNMNMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMM`       
                                        -NNNmmmNNNNNNMNMNMNMNMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMN        
                                        hNmmNmmmmNNNNMNMNMNNMNNMMMMMMMMMMMMMMMMMMMMMMMMMMMM/        
                                       -NmmmmmmmNmNNNNNNNNNNNNNMMMMMMMMMMMMMMMMMMMMMMMMMMMs         
                                       ymmmNNmNmNNNNNNNNNNNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMm.       
                                      .mmmmNNNNNNNNNNNNNNNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM+      
                                   ./yNMMNNNNNNmmmmmNNNNNNNNMMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMd`    
                                `+dMMMMMMNNNNNNmmmmNNNNNNNNNNNNNNNMMMMMMMMMMMMMMMMMMMMMMNMMMMMMm`   
                       ```.`  -yMMMMMMNNNNNNNNNNNNNNNNNNNNNNNNMMMMMMMMMMMMMMMMMMMMMMMMNNmhdMMMMMh   
            `````.::---:+s/:ohMMMMMMMMNNMNNMNNNNNNNNNNMMMMNMMMNNNmmdddhhddmNMMMMMMMMMNmddyomMMMMM/  
         `...`````.:+:.-+yy--oyNMMMMMMMMMMMMMNMMMMNNMMMNNNmmddhhhyyyssooooshmNMMMMMMMNdssyshMMMMMh  
        ...--:-.```..:..:syo.-/yMMMMMMMNMMMNNMNMNNNmmmmmdhhssoooo++++++/++oshmNMMMNNNMNds+yyMMMMMN  
      `.-.```.--::..--/.:oyd/-oddmmNmNmmmNNNNmdhsoossssssoo+++++oo///::///++shmNMMNNNMNhosodMMMMMM  
     .----.......:+--:++:oh/...         -yysooo+//:::::/os+:+hhss+/::--://+ooshmNMNmNNhsyysNMMMMMs  
    .---/+yyss/---os::/sys:.-.          .++yy/oys/-.-::::::-:-----..---::/+oosydmNNmNNhsosdNMMMMN.  
   .--::/+shddhy+::/hyos+.-:.           --------....-::-.............---:/+ossydmNNNNmsosdmmmNMM/   
  `-----:+yys/+osyhdh+:-.-/`           ............-::--.............---:/+ssyydmmNNNNddmmmmmmN+    
  -----:+oos+::--:::::--:o.           `...........--::/:--....``...----::+osyyhdmmNNNmmmmmddmmo     
  ------:+o+/-.....--:/+s:            `....`.......-:::-:o+.......---:::/+osyhhmmmmNNmmdmmdddm/     
  ------:///:-......-/+s:              ......-:/--:/oso+//---------::://+ooshhdmmmNNNNmmmmddmm-     
  `------://:-...--:/o+`                .----/o/--:::---:://++oo/::::////++syhddmmNNmNNmmmmmmd      
   `-:-::////:---://:.                   -:::+soo/::::////+o+yo/+/--:///:/+oyhhdmmNNNNNmmmmmm+      
     `.:/++++::--.                        :::-:o+:/s+:-:-::+/+/:-:--://:::/osyydmNmNNMNNmmmmh       
         `..`                             `/::-----/::::::://:---.--/://///oosyhdmNNNMNNNmmd`       
                                           /+//:------:::----...--:/++++//++ooshdNNMMMMNNNd.        
                                            soo+/:-............-:///ossooooossydmNNMMMMMNs          
                                            .dyooo/----...-::::+syyssyyyyyyhyydmNNMMMMMd-           
                                             -hyhhs+::---::++syydmmmhhyhhyyhddmNMMMMMd/             
                                              .hmddyo+/+/+osyhmNNNNmdhyyhyhdmNNMMMMm+               
                                                +mNNdddhyyhdmNNNNmmdyyyhhddmNMMMmo.                 
                                                 .+dmmmmdhhdmmmddhyyyhddmNNMMds:                    
                                                    -+ssoosssohhhyhdmmNNNds:`                       
                                                       `-/+syyhddmNmhs/-                            
                                                             ```..                                  
                                                                                                    
*/

#include "../libvolition/include/common.h"
#include "../libvolition/include/conation.h"
#include "gui_dialogs.h"
#include "gui_menus.h"
#include "scriptscanner.h"

#include <list>
#include <queue>

namespace Orders
{
	struct CurrentOrderStruct
	{
		CommandCode CmdCode;
		uint64_t CmdIdent;
	
		std::vector<GuiDialogs::DialogBase*> Dialogs;
		std::set<VLString> DestinationNodes;
	
		//member functions
		void Clear(void);
		bool Init(const CommandCode CmdCode, const std::set<VLString> *DestinationNodes);
		bool Finalize(void);
	};

	typedef bool (*ScriptStateFuncPtr)(const char *ScriptName, const std::set<VLString> *DestinationNodes);
	
	bool SendNodeScriptLoadOrder(const char *ScriptName, const std::set<VLString> *DestinationNodes);
	bool SendNodeScriptUnloadOrder(const char *ScriptName, const std::set<VLString> *DestinationNodes);
	bool SendNodeScriptReloadOrder(const char *ScriptName, const std::set<VLString> *DestinationNodes);
	bool SendNodeScriptFuncOrder(ScriptScanner::ScriptInfo::ScriptFunctionInfo *FuncInfo, const std::set<VLString> *DestinationNodes);
	
	extern CurrentOrderStruct CurrentOrder;

}

#endif //VL_CTL_ORDERS_H
