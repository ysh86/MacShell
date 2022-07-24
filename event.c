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
#include "window.h"
#include "menu.h"
#include "shellutils.h"
#include "event.h"
#include "history.h"

/* This file contains code for handling events of various types, 
 * including: Mousedown, Keydown, activate, update */

short doLineEditCommand(c,iscmd,tempText)
char	c;
short	iscmd;
TEHandle tempText;
{	/* return true if we got a valid line editing command.
     * Carry out the editing function.  Keep in mind that the point may
     * be anywhere in the interactive window.
     * 
     */
    if (iscmd) { /* command key is down */
    	if (c=='u') {
    		doEraseLine(tempText);
    		return(1);
    	} 
    } else {
    	if (c==21) { /* control-u */
    		doEraseLine(tempText);
    		return(1);
    	}
    	if (c==30) { /* up-arrow */
    		pasteHistPrev(tempText);
    		return(1);
    	}
    	if (c==31) { /* down-arrow */
    		pasteHistNext(tempText);
    		return(1);
    	}
    }
	return(0);
}


int inProcEventLoop(blink)
	int	blink;
{
	/* The difference between this an the standard event loop is that
	 * any keyboard (non command key) events in the interactive window
	 * cause this to terminate with the key as the return value.
	 * This is designed to be called by utilities which need to get 
	 * their own keyboard input.  (eg. more)  Another difference is that 
	 * if the done flag gets set to true here, we have to force the
	 * issue with an ExitToShell.  
	 * The caller should watch the selection range when inserting characters, 
	 * and should put the selection range
	 * back at the end before returning. */
	EventRecord		event;
	WindowPtr		theWin,tempWin;
	TEHandle		theText;
	int				gotKey = FALSE;
	int				ch;
	
	gInProc = TRUE;
	readOnlyMenus(TRUE);
	
	while ((!gDone) && (!gotKey)) {
		if (isAppWindow(theWin = FrontWindow())) {
			theText = unpackTEH(theWin);
			gTextH = theText;
			if (theWin != diagnosticWin) {
				ChangeMouse(theText); 	/* update mouse icon */

				if ((theWin != interactiveWin) || ((insertAtEnd(theText)) && (blink))) { 
					TEIdle(theText);  		/* cursor blink */
				}
			}
		}
		HiliteMenu(0); 				/*unhilite all menus*/
		SystemTask();
		if (GetNextEvent(everyEvent, &event)) { 
			switch (event.what) {
				case mouseDown:
					HandleMouseDown(&event);
					break;
				case mouseUp:
					break;
				case keyDown:
				case autoKey:
					ch = event.message & charCodeMask;
					tempWin = FrontWindow();
					if (isAppWindow(tempWin) 
								&& (tempWin == interactiveWin)
								&& (!(event.modifiers & cmdKey))) {
						gotKey = TRUE;
					} else 	{
						HandleKeyDown(&event);
					}
					break;
				case nullEvent:
					break;
				case updateEvt:
					DoUpdateEvent(&event);
		    		break; 
				case activateEvt:
					DoActivateEvent(&event);
					break;
			}
		}
	}	
	
	gInProc = FALSE;
	readOnlyMenus(FALSE);

	if (gDone) {
		ExitToShell();
	} else {
		return(ch);
	}
}


doEventLoop()
{
	/* The main event loop */
	EventRecord		event;
	WindowPtr		theWin;
	TEHandle		theText;
	short			i;
	
	while (!gDone) {
		if (isAppWindow(theWin = FrontWindow())) {	
			theText = unpackTEH(theWin); 
			gTextH = theText;
			if (theWin != diagnosticWin) {
				ChangeMouse(theText); 	/* update mouse icon */
				if ((theWin != interactiveWin) || (insertAtEnd(theText)))
					TEIdle(theText);  		/* cursor blink */
			}
		}
		HiliteMenu(0); 				/*unhilite all menus*/
		SystemTask();
		if (GetNextEvent(everyEvent, &event)) { 
			switch (event.what) {
				case mouseDown:
					HandleMouseDown(&event);
					break;
				case mouseUp:
					break;
				case keyDown:
				case autoKey:
					HandleKeyDown(&event);
					break;
				case nullEvent:
					break;
				case updateEvt:
					DoUpdateEvent(&event);
		    		break; 
				case activateEvt:
					DoActivateEvent(&event);
					break;
				default:
					break;
			}
		}
	}	
}


HandleMouseDown(theEvent)
	EventRecord	*theEvent;
{
	WindowPtr		theWindow;
	int				windowCode, partcode, bogus,startValue,endValue;
	Rect			dragRect, growRect, tempRect;
	long			newSize;
	RgnHandle		contRgnHnd;
	TEHandle		tempText;
	WSHandle		tempWS;
	Point			eventPoint;
	ControlHandle	tempCH;

	windowCode = FindWindow(theEvent->where, &theWindow);
    switch (windowCode)
      {
	  case inSysWindow: 
	    break;
	    
	  case inMenuBar:
	    HandleMenu(MenuSelect(theEvent->where));
	    break;
	    
	  case inDrag:
	  	/* According to convention, we drag it and not select it if 
	  	 * the command key is down.  */
	  	if ((theWindow != FrontWindow()) && !(theEvent->modifiers & cmdKey)) { 
	  		SelectWindow(theWindow); 	  		
		} else {
	  		SetRect(&dragRect,
	  			screenBits.bounds.left +4,
	  			screenBits.bounds.top +24,
	  			screenBits.bounds.right -4,
	  			screenBits.bounds.bottom -4);

	  		DragWindow(theWindow, theEvent->where, &dragRect);
	  		/* updateThumbValues(theWindow); */
	  		break;
	  	}  
	  case inContent:
	  	if (theWindow != FrontWindow())
	  		SelectWindow(theWindow); 
	  	eventPoint = theEvent->where;
	  	GlobalToLocal(&eventPoint);
	  	partcode = FindControl(eventPoint,theWindow,&tempCH);
	  		/* We only have one control (vertical scroll).  
	  	 	 * Otherwise we should use GetCRefCon to figure out which one.*/
	  	tempText = unpackTEH(theWindow);
	  	if (tempText) { 
	  		if (PtInRect(eventPoint, &(*tempText)->viewRect)) {
	  			/* in interactive and diagnostic windows, allow the user to move
	  			 * the insertion point and selection range.  We'll add some restrictions
	  			 * in the cut, paste and keydown code to keep the text intact. */
	  			TEClick(eventPoint, (theEvent->modifiers & shiftKey), tempText);
				/* enable and disable  cut and clear menu entries in the interactive 
				 * window. */
				if (theWindow == interactiveWin) {
					if (insertAtEnd(tempText)) {
						readOnlyMenus(FALSE);
					} else {
						/* readOnlyMenus(TRUE); */
						interactiveROMenus();
					}
				}
	  		}
	  	} 
	  	if (partcode) {	/* in the scroll bar */
	  		if (partcode == inThumb) { 	/* on the "box" */
	  			startValue = GetCtlValue(tempCH);
	  			bogus = TrackControl(tempCH,eventPoint,0L); 
	  			endValue = GetCtlValue(tempCH);
	  			doThumb(startValue,endValue,tempText,tempCH);
	  		} else {	/* arrow or grey */
	  			/* TC7 (How do we cast the final arg?) */
#ifdef TC7
	  			bogus = TrackControl(tempCH,eventPoint,(ControlActionUPP)scrollGlue);
#else
	  			bogus = TrackControl(tempCH,eventPoint,scrollGlue);
#endif	  			
	  		}
	  			/* The following is not redundant in the case where a cut
	  			 * or clear operation leaves a chunk of blank space at the end
	  			 * of the document, and less than a viewRect full of text.
	  			 * In this case we want to disable the control after scrolling
	  			 * all the way back to the top of the document. */
	  		adjustScrollValues(tempText, tempCH);
	  	} else { /* if there were somewhere else it could be...*/
	  	}
	  	break;
	  	
	  case inGoAway:  
	  	if (TrackGoAway(theWindow, theEvent->where)){
		  /* If it's a diagnostic or interactive window, just hide it.
		   * if it's a script window, close the file */
		   if ((theWindow == diagnosticWin) || (theWindow == interactiveWin)) {
		   		HideWindow(theWindow);
		   	} else {
		   		closeScript(theWindow);
		   	}
		   	if (!isAppWindow(FrontWindow())) {
				/* disable some menus */
				adjustMenus(0); /* disable edit */
			}
		}
	  	break;
	  	 
	  case inGrow:
	  	/* If the size didn't change, we could avoid doing most of this */
	  	SetRect(&growRect,
	  		60,
	  		40,
	  		screenBits.bounds.right - screenBits.bounds.left -4,
	  		screenBits.bounds.bottom - screenBits.bounds.top -24);
	  	if(newSize = GrowWindow(theWindow,theEvent->where, &growRect)) {
	  		SizeWindow(theWindow, LoWord(newSize), HiWord(newSize), 0xff);
	  		EraseRect(&theWindow->portRect);
	  		InvalRect(&theWindow->portRect);
	  		moveScrollBar(theWindow); 		/* in window.c */
	  		ReSizeTE(theWindow);
	  		DrawGrowIcon(theWindow);
	  		adjustTEPos(theWindow); /* make sure the insert point is still visible */
	  		updateThumbValues(theWindow);	/* in window.c */
	  		/* adjustTEPos(theWindow); 		/* in window.c */
	  	}
	  	break;	
      }
}

HandleKeyDown(theEvent)
	EventRecord		*theEvent;
{
	char		c;
	char		*arr;
	char		*toolong = "Sorry, file too long. ";
	char		*err = "Command line too long. \r";
	int			i, j, ready;
	Handle		tempTH;
	WindowPtr	tempWin;
	TEHandle	tempText;
	
	
	c = theEvent->message & charCodeMask;
	
	if ((theEvent->modifiers & cmdKey) && !(lineEditCmd(theEvent))) {
			/* we assume all illegal menu items have been disabled */
			HandleMenu(MenuKey(c)); 	/* do menu item */
	} else {
		if (isAppWindow(tempWin = FrontWindow())) {
			tempText = unpackTEH(tempWin);
		} else {
			return;
		}
		setDirtyBit(tempWin);
		if (tempWin == interactiveWin) { 
			if (tempText) {
				/* make the cursor bounce to the
				 * end if it's not past the prompt on the last line. */
				adjustTEPos(interactiveWin);
				updateThumbValues(interactiveWin);
				if (!insertAtEnd(tempText)) {
					putCursorAtEnd(tempText);
					readOnlyMenus(FALSE);
				}
				MoveHHi((Handle)tempText); /* move it to the top of the heap (since it's locked for awhile) */
				HLock((Handle)tempText);
				if (doLineEditCommand(c,(short)(theEvent->modifiers & cmdKey),tempText)){
				
				} else if (checkNameCompletion(c)) {
					/* do file name completion */
					doNameCompletion(tempText);				
				} else if (c == 13 ) {  /* user hit the return key*/
					putCursorAtEnd(tempText);
					/* pick out a command line */
					tempTH = (Handle)(*tempText)->hText; /* get a handle to the chars */
					HLock((Handle)tempTH);
					i = ((*tempText)->teLength -1); 	/* get the TE Record length */
					while ((*tempTH)[i] != 13) 		/* go back to beginning of line */
						i--;
					i += strlen(PROMPT); /* add the length of the prompt */
					i++; 					/* add one for the return char itself */
					j = ((*tempText)->teLength) - i; 	/* get the length of the string*/
					arr=NewPtr(255);
					if ((j > 0) && (j < 254)) {
		 				BlockMove(&((*tempTH)[i]),arr,(long)j); 		/* extract the command line */	
		 			} else {
		 				if (j >= 254)
		 					showDiagnostic(err);
		 				j = 0;
		 			}
		 			HUnlock((Handle)tempTH); 					/* done with tempTH */
		 			arr[j] = 0;					
	 				TEKey(c, tempText); 	/* put the return in */
	 				adjustTEPos(interactiveWin); /* hmm, doesn't cause any scroll... */
	 				doCallCmd(tempText,&arr);
	 				DisposPtr((Ptr)arr);
				} else if (c == 8) {  					/* user hit the delete key */
					/* don't let the prompt be deleted*/
					if (allowDelete(tempText)) {
						TEKey(c, tempText);
					}				
				} else { TEKey(c, tempText); } 		/* any other key, do it  */				
				HUnlock((Handle)tempText);
			}
		} else  if (tempWin == diagnosticWin) { /* if it's a diagnostic window */ 
			/* can't type into the diagnostic window at all*/
		} else { /* it's a scripting window */
			if (((*tempText)->teLength >= (30*1024 - 1)) && (c != 8)) {
				errAlert(toolong);
			} else {
				TEKey(c, tempText);
				ScrollInsertPt(tempText);
				updateThumbValues(tempWin);
			}
		}
	}

}

short allowDelete(teh)
	TEHandle	teh;
{ /* Check whether or not the delete key will mess with sacred entities in
   * our interactive window.  If so, return false, otherwise true */
	Handle		texth;
	short		sstart,send,i;
	
	sstart = (*teh)->selStart;
	send =   (*teh)->selEnd;
	
	texth = (Handle)(*teh)->hText; /* get a handle to the chars */
	HLock((Handle)texth);
	i = ((*teh)->teLength -1);  	/* points to eol */
	while ((*texth)[i] != 13) 			/* go back to beginning of line */
		i--;
	i++; 							/* add one for the return char itself */
	i = i + strlen(PROMPT); 		/* add the length of the prompt */
	HUnlock((Handle)texth); 				/* done with "str" */	
	
	/* if the selection range is one character or more, allow the delete,
	 * even if the range start is right after the prompt.  this operation
	 * will just remove the chars in the range, but none preceeding it */
	if ((sstart > i) || ((sstart==i) && (sstart != send))) return(TRUE);
	else return(FALSE);

}

short allowPaste(teh)
	TEHandle	teh;
{ /* Check whether or not a paste operation will mess with sacred entities in
   * our interactive window.  If so, return false, otherwise true */
	Handle		texth;
	short		sstart,send,i;
	
	sstart = (*teh)->selStart;
	send =   (*teh)->selEnd;
	
	texth = (Handle)(*teh)->hText; /* get a handle to the chars */
	HLock((Handle)texth);
	i = ((*teh)->teLength -1);  	/* points to eol */
	while ((*texth)[i] != 13) 			/* go back to beginning of line */
		i--;
	i++; 							/* add one for the return char itself */
	i = i + strlen(PROMPT); 		/* add the length of the prompt */
	HUnlock((Handle)texth); 				/* done with "str" */	
	
	/* as long as the start of the selection range is past the prompt,
	 * paste is ok. */
	if (sstart >= i) return(TRUE);
	else return(FALSE);

}


DoUpdateEvent(theEvent)
	EventRecord		*theEvent;
{
	WindowPtr		tempWindow, oldPort;
	TEHandle		tempText;
	WSHandle		tempWS;
	
	
	GetPort(&oldPort);
	tempWindow = (WindowPtr)theEvent->message;
	SetPort(tempWindow);
	tempText = unpackTEH(tempWindow);	
	BeginUpdate(tempWindow);
	HLock((Handle)tempText); 
	/* EraseRect(&(*tempText)->viewRect); */
	/* TEUpdate(&(*tempText)->viewRect, tempText); */
	TEUpdate(&tempWindow->portRect,tempText);
	HUnlock((Handle)tempText);
	DrawGrowIcon(tempWindow);
	DrawControls(tempWindow);
	EndUpdate(tempWindow);
	SetPort(oldPort); 
}


DoActivateEvent(theEvent)
	EventRecord		*theEvent;
{
	/* Can we assume we only get our own windows here ?? */
	WindowPtr		tempWindow;
	TEHandle		tempText;
	WSHandle		tempWS;
	ControlHandle	tempCH;
	
	tempWindow = (WindowPtr)theEvent->message;
	SetPort(tempWindow);
	tempWS = (WSHandle)GetWRefCon(tempWindow);
	tempText = (*tempWS)->theTEH;	
	tempCH = (*tempWS)->theCH;
	
	if(theEvent->modifiers & activeFlag) {
		/* activate */
		TEActivate(tempText);
		ShowControl(tempCH); 
		DrawGrowIcon(tempWindow);
		
		/* only the diagnostic window really needs this: */
		if (tempWindow == diagnosticWin) {
			updateThumbValues(tempWindow);
		}
		if ((tempWindow == diagnosticWin) || 
			(gInProc && (tempWindow == interactiveWin))) {
			readOnlyMenus(TRUE);
		} else {
			readOnlyMenus(FALSE);		
		}			

	} else {
		/* deactivate */
		TEDeactivate(tempText);
		HideControl(tempCH);
		DrawGrowIcon(tempWindow);
	}
}

short lineEditCmd(event)
	EventRecord		*event;
	
{ /* return true if the key event is a line editing command. This must not
   * claim any key events which should drive menu commands.
   * We only really care about claiming keys modified by the command
   * key here, since this is only used to make sure we don't invoke
   * a menu routine.
   */
   	char	c;
   	
   	c = event->message & charCodeMask;
	if(event->modifiers & cmdKey) {
		if (c == 'u') return(1); /* command-u */
	} else {
		if (c == 21) return(1); /* control-u */
	}
	return(0);
}


short doEraseLine(teh)
	TEHandle	teh;
{
	short	s,e;
	Handle	texth;

	texth = (Handle)(*teh)->hText; /* get a handle to the chars */
	s = e = ((*teh)->teLength);  	/* points to eol */
	while ((*texth)[s] != 13) 			/* go back to beginning of line */
		s--;
	s++; 							/* add one for the return char itself */
	s = s + strlen(PROMPT); 		/* add the length of the prompt */
	
	TEDeactivate(teh); 
	
	TESetSelect((long)(s),(long)e,teh);
	TECut(teh);
	ZeroScrap();
	if (TEToScrap()) ZeroScrap();

	TEActivate(teh);

}
