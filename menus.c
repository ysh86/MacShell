/* This file is part of MacShell. 
 *
 * MacShell is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.  No author or distributor accepts
 * responsibility to anyone for the consequences of using it or for
 * whether it serves any particular purpose, or works at all,
 * unless he says so in writing.
 *
 * Everyone is granted permission to copy and modify MacShell.  
 *
 * All or part of the source of MacShell may be used in other software, 
 * provided that:
 * 1. Such software is not sold at a cost higher than that which would 
 * normally be necessary to recoup media duplication and distribution
 * costs.
 * 2. If such software is distributed, its source code must also be made 
 * available under terms identical to those specified in this notice. 
 * Exceptions to this rule may be granted by the author in 
 * certain instances.  
 *
 * This notice must be preserved on all copies of this file. 
 *
 * As the author of MacShell, I would also like to request that any 
 * modifications, extensions, or bug fixes made to this source be 
 * forwarded back to me for possible inclusion in future versions of 
 * MacShell.
 * 
 * Thanks, and happy programming!
 * Fred Videon (fred@cs.washington.edu)
 */

#include "MacShell.h"
#include "menu.h"
#include "window.h"	
#include "shellutils.h"
#include "event.h"
/* This file contains code for all the menu stuff */

standardMenuSetup(MBARID, AppleMenuID)
short	MBARID;
short	AppleMenuID;
{
	Handle menuBar;
	menuBar = GetNewMBar(MBARID);			/* read menus into menu bar */
	if (!menuBar) { 
		/* DeathAlert(rUtilStrings, eNoMenuBar); */
	}
	SetMenuBar(menuBar);							/* install menus */
	DisposHandle((Handle)menuBar);
	AddResMenu(GetMHandle(AppleMenuID), 'DRVR');	/* add DA names to Apple menu */
	DrawMenuBar();
}

HandleMenu (mSelect)
	long	mSelect;
{
	int			menuID = HiWord(mSelect), n;
	int			menuItem = LoWord(mSelect);
	Str255		name;
	GrafPtr		savePort;
	WindowPtr	frontWindow;
	TEHandle	tempText;
	char		*Command;
	MenuHandle	menuH;
	char		*toolong = "Sorry, file too long. ";
	
	if(isAppWindow(frontWindow = FrontWindow())) {
		tempText = unpackTEH(frontWindow);
	} else {
		tempText = 0L;
	}
	
	switch (menuID)
	{
	case	mApple:
		if (menuItem == iAbout) {
			doAbout();
		} else {
			GetPort(&savePort);
			menuH = GetMHandle(mApple);
			GetItem(menuH, menuItem, name);
			OpenDeskAcc(name);
			SetPort(savePort);
		}
		break;
	
	case	mFile:
		switch (menuItem)
		  	{
		  	case iNew:
		  		doNewScript();
				adjustMenus(1); /* enable menus */
		  		break;
			case iOpen:
				doOpen();
				adjustMenus(1); /* enable menus */
				break;
			case iSave:
				if (isAppWindow(frontWindow))
					doSave(frontWindow);
  				break;
  			case iSaveAs:
  				if (isAppWindow(frontWindow))
  					doSaveAs(frontWindow);
  				break;	  			
			case iClose:
				if ((frontWindow == interactiveWin) || (frontWindow == diagnosticWin)){
					HideWindow(frontWindow);
				} else if (isAppWindow(frontWindow)){
					closeScript(frontWindow);
				}
				if (!isAppWindow(FrontWindow())) {
					/* disable some menus */
					adjustMenus(0); /* disable menus */
				}
				break;
			case iQuit:
				closeAndQuit();
				break;
		  	}
			break;
  				
	case	mEdit:
	  	switch (menuItem)
		  	{
		  	case	iCut: 
		  		if (tempText) {	
		  			if (frontWindow == interactiveWin) {
		  				/* if the selection range is such that a cut operation 
		  				 * won't mess with sacred interactive window entities
		  				 */
		  				if (allowDelete(tempText)) {
		  					TECut(tempText);
		  				} else {
		  					TECopy(tempText);	
		  				}	
		  			} else if (frontWindow == diagnosticWin) {
		  				TECopy(tempText);	 		 
		  			} else {
		  				TECut(tempText);
		  				ScrollInsertPt(tempText); /* put the insert pt back in view */ 
		  				updateThumbValues(frontWindow);
						setDirtyBit(frontWindow);
		  			}
					ZeroScrap();
					if (TEToScrap()) ZeroScrap();
				}
				break;
  			case	iCopy:
  				if (tempText) {
  					TECopy(tempText);
 					ZeroScrap();
					if (TEToScrap()) ZeroScrap();
				}
				break;
			case	iPaste:
				if (tempText) {	
					TEFromScrap();
					if (frontWindow == interactiveWin) {
					/* we allow the user to paste into the interactive
					 * window even if the selection range isn't beyond
					 * the prompt. */
						if (!allowPaste(tempText)) {
							putCursorAtEnd(tempText);
							readOnlyMenus(FALSE);
						} 
						adjustTEPos(interactiveWin);
						updateThumbValues(interactiveWin);
						pasteToInteractive(tempText);
						setDirtyBit(frontWindow);
					} else if (frontWindow != diagnosticWin) {
						if (((*tempText)->teLength + TEGetScrapLen()) >= (30*1024)) {
							errAlert(toolong);
						} else {
  							TEPaste(tempText);
		  					ScrollInsertPt(tempText);  
		  					updateThumbValues(frontWindow);
		  					setDirtyBit(frontWindow);
		  				}
		  			}
		  		}
				break;
			case	iClear:	
				if (tempText) {			
					if ((frontWindow == interactiveWin) || (frontWindow == diagnosticWin)) {
		  				/* We really should let the user clear as long as the selection 
		  				 * range doesn't go beyone the current command line. */
		  			} else {
		  				TEDelete(tempText); 
		  				ScrollInsertPt(tempText); 
		  				updateThumbValues(frontWindow);
		  				setDirtyBit(frontWindow);
		  			}
		  		}
				break;				
			}
			break;
	case mWindow:
		switch (menuItem)
			{
			case iInteractive:
				ShowWindow(interactiveWin);
				SelectWindow(interactiveWin);
				adjustMenus(1); /* enable menus */
				break; /* show, bring to front */
			case iDiagnostic:
				ShowWindow(diagnosticWin);
				SelectWindow(diagnosticWin);
				adjustMenus(1); /* enable menus */
				break; /* show, bring to front */
			default:
				/* script window */
				showScript(menuItem);
				break;
			}
		break;

	}		
}/* end HandleMenu */

showScript(id) /* show window */
	int		id;
{ /* figure out which window was selected in the window menu, and put it in front. 
   * MenuID 4 corresponds to the first window on the window list, etc. */
	WLPtr	anode;
	int		i;
	
	anode = &gWLRoot;
	i = 3;
	while (i != id) {
		anode = (WLPtr)anode->next;
		i++;
	}
	
	SelectWindow(anode->wptr);

}

adjustMenus(enable)
	int	enable;
{ /* adjust selected menu items.  Disable when there are no application
   * windows, and enable when there are. */
  /* In particular this set should be used to disable things when we
   * close the last app window, or to enable things when we open or select 
   * a window other than the diagnostic window or the Interactive window
   * during "more" etc. */
	MenuHandle	menuH;
	int			count,i;

	menuH = GetMHandle(mEdit);
	count = CountMItems(menuH);
	i = 1;
	if (enable) {
		while (i <= count) { /* enable the entire edit menu */
			EnableItem(menuH,i);
			i++;
		}
		menuH = GetMHandle(mFile);
		EnableItem(menuH,iSave);
		EnableItem(menuH,iSaveAs);
		EnableItem(menuH,iClose);		
	} else {
		while (i <= count) { /* disable the entire edit menu */
			DisableItem(menuH,i);
			i++;
		}
		menuH = GetMHandle(mFile);
		DisableItem(menuH,iSave);
		DisableItem(menuH,iSaveAs);
		DisableItem(menuH,iClose);			
	}
}

readOnlyMenus(ro)
	int	ro;
{ /* Adjust edit menu when a read-only window comes in front. */
	MenuHandle	menuH;
	menuH = GetMHandle(mEdit);

	if (ro) {
		DisableItem(menuH,iCut);
		DisableItem(menuH,iPaste);
		DisableItem(menuH,iClear);	
	} else {
		EnableItem(menuH,iCut);
		EnableItem(menuH,iPaste);
		EnableItem(menuH,iClear);		
	}		
}

interactiveROMenus()
{ /* This is like ReadOnlyMenus(TRUE), except that we leave paste enabled.
   * so we can allow the user to paste to the insertion point no matter 
   * where the current selection range is. */
	MenuHandle	menuH;
	menuH = GetMHandle(mEdit);

	DisableItem(menuH,iCut);
	/* DisableItem(menuH,iPaste); */
	DisableItem(menuH,iClear);	
}

redrawWinMenu()
{ /* Delete the window menu, and dispose of it's handle.
   * Get a clean copy from the resource manager, and rebuild and
   * redraw it based on the info in the window List. */
    MenuHandle		mh;
	WLPtr			tempnode;
	Str255			title;
    
    mh = GetMHandle(mWindow);
	DeleteMenu(mWindow);
	ReleaseResource((Handle)mh);
	
	mh = GetMenu(mWindow);
	InsertMenu(mh,0);
	
	tempnode = &gWLRoot;
	while (tempnode->next != 0L) {
		tempnode = (WLPtr)tempnode->next;
		GetWTitle(tempnode->wptr,(unsigned char *)&title);
		AppendMenu(mh,title);
	}
	
	DrawMenuBar();

}


