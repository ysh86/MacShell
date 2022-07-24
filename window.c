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
/* This file contains window and text edit stuff, including the "About
 * Macshell" window.  */

ControlHandle 		vScroll = (ControlHandle) 0L;

putCursorAtEnd(tempText)
	TEHandle	tempText;
{	/* put the cursor at the end of a TERecord */
	long		templ;
	templ = 0L;
	templ += (*tempText)->teLength;
	TESetSelect(templ, templ, tempText);
}


openDiagnosticWin()
{
	int				wt, wl, wb, wr, ww, wh, ht;
	static 	Rect	windowBounds;
	TEHandle		tempText;
	char			*title = (char *)"\pDiagnostics";

	wt = screenBits.bounds.top + 40;
	wl = screenBits.bounds.left + 10;
	wb = screenBits.bounds.bottom - 3;
	wr = screenBits.bounds.right - 30;
	/* if wr is bigger than x, scale back to maxWidth */
	if (wr > MAXWDTH) { wr = MAXWDTH; }
	ww = wr - wl;
	wh = wb - wt;
	
	/* Put it on the bottom half of the screen */
	ht = wh/3;
	wt = wb - ht + 10;

	SetRect(&windowBounds, wl, wt, wr, wb);
	
	diagnosticWin = createNewWindow(windowBounds);
	setDirtyBit(diagnosticWin);  /* this starts out dirty. */
		
	SetWTitle(diagnosticWin,(unsigned char *)title);	
	
}



openInteractiveWin()
{
	int				wt, wl, wb, wr, ww, wh, ht;
	static 	Rect	windowBounds;
	TEHandle		tempText;
	char			*startUpStr1start = "Welcome to MacShell (version ";
	char			*startUpStr1end = ")\r";
	char			*startUpStr2 = "For help type \"help\".\r";
	char			*title = (char *)"\pInteractive Session";

	wt = screenBits.bounds.top + 40;
	wl = screenBits.bounds.left + 10;
	wb = screenBits.bounds.bottom - 20;
	wr = screenBits.bounds.right - 30;
	/* if wr is bigger than x, scale back to maxWidth */
	if (wr > MAXWDTH) { wr = MAXWDTH; }
	ww = wr - wl;
	wh = wb - wt;
	
	/* Put it on the top half of the screen */
	ht = (wh/3)*2;
	wb = wt + ht;
	
	SetRect(&windowBounds, wl, wt, wr, wb);
	
	interactiveWin = createNewWindow(windowBounds);
	setDirtyBit(interactiveWin);  /* this starts life dirty. */
	
	tempText = unpackTEH(interactiveWin);
	
	SetWTitle(interactiveWin,(unsigned char *)title);	
	TEInsert(startUpStr1start, (long)strlen(startUpStr1start), tempText); 
	TEInsert(VERSION, (long)strlen(VERSION), tempText); 
	TEInsert(startUpStr1end, (long)strlen(startUpStr1end), tempText); 
	TEInsert(startUpStr2, (long)strlen(startUpStr2), tempText); 

}

WindowPtr	createNewWindow(bounds)
	Rect	bounds;
{
	int				wwdth, whgth, sbarl, sbarr, sbarb, lspace=0;
	Rect			scrollBounds;
	WindowPtr		storage;
	WindowPtr		aWindow;
	TEHandle		tempText;
	WSHandle		tempWS;
	ControlHandle	vScroll;
	Rect			destRect, viewRect,vScrollRect;

	aWindow = 0L;
	
	wwdth = bounds.right - bounds.left;
	whgth = bounds.bottom -  bounds.top;
	sbarl = wwdth - SBARWIDTH + 1;
	sbarr = wwdth + 1;
	sbarb = whgth - SBARWIDTH + 2;
		
	SetRect(&scrollBounds, -1, sbarl, sbarb, sbarr);
	
	storage = (WindowPtr)NewPtr(sizeof(WindowRecord));
	if (storage) {
		aWindow = NewWindow(storage, &bounds,"\p", FALSE, documentProc, 
			(WindowPtr)(-1L), 1, 0L);
	}
	
	if (aWindow) {	
		SetPort(aWindow);
	
		TextFont(monaco);
		TextSize(9);
	
		BlockMove(&aWindow->portRect, &viewRect, (Size)sizeof(Rect)); 	
		viewRect.right -= 16;
		viewRect.bottom -= 16;	
		BlockMove(&viewRect, &destRect, (Size)sizeof(Rect));
	/* 	InsetRect(&destRect, 4, 4); */
		InsetRect(&destRect, 4, 2);
	
		tempText = TENew(&destRect, &viewRect); /* make TE handle */
		SetLineHeight((short *)&lspace, tempText); /*set up line spacing */
		gTextH = tempText;
		
		/* under TC7 this is clickLoop (or is that just my copy?)*/
#ifdef TC7
		(*tempText)->clickLoop = (ProcPtr) ClikLoop;
#else
		(*tempText)->clikLoop = (ProcPtr) ClikLoop;
#endif		
		DrawGrowIcon(aWindow);

		/* Draw vertical scroll */
		SetRect(&vScrollRect, 	(*aWindow).portRect.right-15,
								(*aWindow).portRect.top-1,
								(*aWindow).portRect.right+1,
								(*aWindow).portRect.bottom-14);
		vScroll = NewControl(aWindow,&vScrollRect,"\p",1L,0,0,0,
						scrollBarProc,VERT);

		tempWS = (WSHandle) NewHandle(sizeof(WindowStuff)); /* set up WS handle */	
		(*tempWS)->theTEH = tempText;
		((*tempWS)->fname)[0] = 0;
		(*tempWS)->vRef = 0;
		(*tempWS)->vers = 0;
		(*tempWS)->dirty = FALSE; /* initial state is clean */
		(*tempWS)->theCH = vScroll;					
		SetWRefCon(aWindow,(long)tempWS);
	} else {
		DisposPtr((Ptr)storage);
	}
	return(aWindow);
}

setDirtyBit(window)
	WindowPtr	window;
{
	WSHandle	tempWS;
	tempWS = (WSHandle)GetWRefCon(window);
	if ((*tempWS)->dirty != 1L) {
		(*tempWS)->dirty = 1L;
		SetWRefCon(window,(long)tempWS);	
	}
}

windowListInsert(wp)
	WindowPtr	wp;
{ /* add a new node to the end of the window list */
	WLPtr	newnode;
	WLPtr	tempnode;
	
	newnode = (WLPtr)NewPtr(sizeof(WLRec));
	newnode->next = 0L;
	newnode->wptr = wp;
	
	tempnode = &gWLRoot;
	while (tempnode->next != 0L) {
		tempnode = (WLPtr)tempnode->next;
	}
	tempnode->next = (Ptr)newnode;
	newnode->prev = (Ptr)tempnode;
}

int	isAppWindow(window)
	WindowPtr window;
{
	if (window)
		return(((WindowPeek)window)->windowKind >= userKind);
	else 
		return(FALSE);

}


windowListDelete(wp)
	WindowPtr	wp;
{ /* delete a node from the window list.
   * We assume it's really in the list. */
	WLPtr	tempnode;
		
	tempnode = &gWLRoot;
	while (tempnode->wptr != wp) {
		tempnode = (WLPtr)tempnode->next;
	}
	if (tempnode->prev != 0L)
		((WLPtr)(tempnode->prev))->next = tempnode->next;
	if (tempnode->next != 0L)
		((WLPtr)(tempnode->next))->prev = tempnode->prev;
	DisposPtr((Ptr)tempnode);
}


SetLineHeight(spacing, hTE)
	short		*spacing;
	TEHandle	hTE;
{
	FontInfo	fontStuff;
	short		extra;
	
	SetPort((*hTE)->inPort);
	
	TextFont((*hTE)->txFont);
	TextSize((*hTE)->txSize);
	
	GetFontInfo(&fontStuff);
	
	(*hTE)->fontAscent = fontStuff.ascent;
	(*hTE)->lineHeight = fontStuff.ascent + fontStuff.descent + fontStuff.leading;
	
	if((*spacing) == singleSp)  return;
	else if ((*spacing) == oneandhalfSp) extra = (*hTE)->lineHeight/2;	
	else if ((*spacing) == doubleSp) extra = (*hTE)->lineHeight;
	(*hTE)->lineHeight += extra;
	(*hTE)->fontAscent += extra;
	return;
	
}

moveScrollBar(whichWin) /* move vertical scroll control */
	WindowPtr		whichWin;
{
	Rect			vScrollRect;
	WSHandle		tempWS;
	ControlHandle	tempCH;
	
	
	SetRect(&vScrollRect, 	(*whichWin).portRect.right-15,
							(*whichWin).portRect.top-1,
							(*whichWin).portRect.right+1,
							(*whichWin).portRect.bottom-14);
							
	tempWS = (WSHandle)GetWRefCon(whichWin);
	tempCH = (*tempWS)->theCH;
	
	(*tempCH)->contrlRect = vScrollRect;
							
}


pascal void scrollGlue(tempCH,partcode) 
	ControlHandle		tempCH;
	int					partcode;
{
	int			startValue;	
	short		lineHeight, viewHeight, nLines, textHeight;
	short		viewTop, viewBot, destTop, destBot, delta;
	TEHandle	theText;
	WindowPtr	theWin;
	
	theWin = FrontWindow();
	theText = unpackTEH(theWin);
	
	viewTop = ((*theText)->viewRect).top;
	viewBot = ((*theText)->viewRect).bottom;
	destTop = ((*theText)->destRect).top;
	destBot = ((*theText)->destRect).bottom;
	lineHeight = (*theText)->lineHeight;
	viewHeight = viewBot-viewTop;
	nLines = (*theText)->nLines;

	textHeight = (lineHeight * nLines);
	

	startValue = GetCtlValue(tempCH);
	
	switch((partcode)) {
	case inUpButton:
		/* if destrect top is not below viewrect top then... */
		if (destTop <= viewTop) {
			if ((viewTop - destTop) < lineHeight) delta = viewTop - destTop + 2;
			else delta = lineHeight;
			SetCtlValue(tempCH, startValue - delta);
			vertScrollContents(delta);
		}
		break;
	case inDownButton: /* we shouldn't be able to scroll past the last line */
		if (destTop + textHeight - viewHeight > 0) {
			if ((destTop + textHeight - viewHeight) < lineHeight)
				delta = destTop + textHeight - viewHeight;
			else delta = lineHeight;
			SetCtlValue(tempCH, startValue + delta);
			vertScrollContents(-delta);
		}
		break;
	case inPageUp: 
		if (destTop <= viewTop) {
			if ((viewTop - destTop) < viewHeight) delta = viewTop - destTop + 2;
			else delta = viewHeight;
			vertScrollContents(delta);
			SetCtlValue(tempCH, startValue - delta);
		}
		break;
	case inPageDown:
		if (destTop + textHeight - viewHeight > 0) {
			if ((destTop + textHeight - viewHeight) < viewHeight)
				delta = destTop + textHeight - viewHeight;
			else delta = viewHeight;
			vertScrollContents(-delta);
			SetCtlValue(tempCH, startValue + delta);
		}
		break;
	}
}

vertScrollContents(howmuch)
	int		howmuch;
{
	WindowPtr	theWin;
	TEHandle	theText;
	
	theWin = FrontWindow();
	theText = unpackTEH(theWin);
	TEScroll(0,howmuch,theText);
}


doThumb(start,end,tempText,tempCtl)
	int				start,end;
	TEHandle		tempText;
	ControlHandle	tempCtl;
{	/* called from the event loop */
	int			lineHeight, delta, max, min;
	int			viewTop, viewBot, viewHeight, nLines, textHeight, destTop;
	
	max = GetCtlMax(tempCtl);
	min = GetCtlMin(tempCtl);

	viewTop = ((*tempText)->viewRect).top;
	viewBot = ((*tempText)->viewRect).bottom;
	viewHeight = viewBot-viewTop;
	nLines = (*tempText)->nLines;
	lineHeight = (*tempText)->lineHeight;
	textHeight = (lineHeight * nLines);
	destTop = ((*tempText)->destRect).top;

	if (end == max) {
		delta =  viewHeight - destTop - textHeight;
	} else if (end == min) {
		delta = viewTop - destTop + 2;
	} else {
		delta = start - end;
	}
	
	TEScroll(0,delta,tempText);
}


updateThumbValues(theWin)
	WindowPtr	theWin;
{ /* called from a variety of places to get thumb values in
   * line with reality */
	WSHandle		tempWS;
	ControlHandle	tempCH;
	TEHandle		tempTE;
	
	tempWS = (WSHandle)GetWRefCon(theWin);
	tempCH = (*tempWS)->theCH;
	tempTE = (*tempWS)->theTEH;
	adjustScrollValues(tempTE,tempCH);
	
}

ReSizeTE(tempWindow)
	WindowPtr	tempWindow;
{
	Rect			tempRect;
	TEHandle		tempText;
	WSHandle		tempWS;
	
	tempText = unpackTEH(tempWindow);
	
	BlockMove(&tempWindow->portRect, &tempRect, (Size)sizeof(Rect)); /* get the new size */
	
	tempRect.right -= 16;   /* adjust for the scroll bar */
	tempRect.bottom -= 16;
	
	BlockMove(&tempRect, &(*tempText)->viewRect, (Size)sizeof(Rect)); /* write viewRect */
	
	tempRect.left += 4;  /* adjust margins for destRect */
	tempRect.top = (*tempText)->destRect.top; 
	tempRect.right -= 4;
	tempRect.bottom = (*tempText)->destRect.bottom;
	
	BlockMove(&tempRect, &(*tempText)->destRect, (Size)sizeof(Rect)); 
	
	/* This causes a big performance hit for longer files... 
	 * Maybe we could avoid it in some cases. */
	/* TECalText(tempText); /* Recalculate line starts, update text wrap. */

}

adjustTEPos(tempWindow)			
/* puts the prompt of an interactive window back in the viewrect after a 
 * grow operation, or after a command dumps some text into the window... */
	WindowPtr	tempWindow;
{
	int				position,nLines,line;
	int				linePos; /* should it be unsigned? */
	int				lineHeight, viewHeight, newBot, delta;
	int				viewTop, viewBot, destTop;
	unsigned int	textHeight;
	TEHandle		tempText;
	WSHandle		tempWS;
	
	tempText = unpackTEH(tempWindow);
	
	nLines = (*tempText)->nLines;
	position = (*tempText)->selEnd;
	viewTop = ((*tempText)->viewRect).top;
	viewBot = ((*tempText)->viewRect).bottom;
	destTop = ((*tempText)->destRect).top;
	lineHeight = (*tempText)->lineHeight;
	viewHeight = viewBot-viewTop;
	/* textHeight = position - destTop; This one isn't quite right...??? */
	
	/* line = 1;
	while((position > (*tempText)->lineStarts[line]) && (line <= nLines)) line += 1; */
	/* textHeight = (lineHeight * (line+1)); */
	
	textHeight = (lineHeight * nLines);
	linePos = destTop + textHeight;
	newBot = viewBot; 
	if ((tempWindow == interactiveWin) || (tempWindow == diagnosticWin)) {
		if (textHeight > viewHeight)  {
			/* 	negative delta -> down arrow */
			delta = newBot - linePos;	
			TEScroll(0,delta,tempText);
		} 
	} else { /* assume we never get here with anyone else's windows */
		
	}
	
}

ScrollInsertPt(hTE)  
/* Use this when we do keydown (and perhaps cut/paste) in a script window  */
/* Scroll as necessary to put the insert point back in the viewRect. */
	TEHandle		hTE;
{
	TEPtr			pTE;
	int				position, line, nLines, lineHeight;
	int				linePos;
	int				viewTop, viewBot, delta;
	
	HLock((Handle)hTE);
	pTE = *hTE;
	
	nLines = pTE->nLines;
	position = pTE->selEnd;
	viewTop = (pTE->viewRect).top;
	viewBot = (pTE->viewRect).bottom;
	lineHeight = pTE->lineHeight;
	
	line = 1;
	while((position > pTE->lineStarts[line]) && (line <= nLines)) line += 1;
	/* do the following because a line with just a return and no other chars
	 * doesn't get a line start, yet we want the return to trigger a 
	 * scroll */
	if ((*(pTE->hText))[position-1] == RET) line++; 
	
	linePos = (pTE->destRect).top + pTE->lineHeight * (line);
	
	if(linePos < (viewTop + lineHeight)) {
		delta = (viewTop + lineHeight) -linePos;
		TEScroll(0, delta, hTE);	/* positive value of delta */	
	} else if (linePos > viewBot) {
		delta = viewBot - linePos;
		TEScroll(0, delta, hTE); /* negative value of delta */		
	}
	
	HUnlock((Handle)hTE);	
}

adjustScrollValues(teHndl, ctl)
	TEHandle 		teHndl;
	ControlHandle 	ctl;
{	/* cribbed from CShell, courtesy of Apple Computer.*/
        Boolean front;
        short   textPix, viewPix;
        short   max, oldMax, value, oldValue, destTop;

        front = ((*ctl)->contrlOwner == FrontWindow());

        oldValue = GetCtlValue(ctl);
        oldMax   = GetCtlMax(ctl);
		textPix = ((*teHndl)->lineHeight) * ((*teHndl)->nLines);
        viewPix = (*teHndl)->viewRect.bottom - (*teHndl)->viewRect.top;
        destTop = (*teHndl)->destRect.top;
       	max = textPix - viewPix;
       	if ((max <= 0) && (destTop < 2)) {
       		max = 2 - destTop;
       	}
        if (max < 0) max = 0;
        if (max != oldMax) {
                if (front) SetCtlMax(ctl, max);
                else       (*ctl)->contrlMax = max;
        }

        value = (*teHndl)->viewRect.top  - (*teHndl)->destRect.top;
        if (value < 0)   value = 0;
        if (value > max) value = max;
        if (value != oldValue) {
                if (front) SetCtlValue(ctl, value);
                else       (*ctl)->contrlValue = value;
        }
}


adjustTESize(tempText)
	TEHandle	tempText;
{	
	long			sStart, sEnd, templ;
	/* cut out all but the last "scrollback" bytes from a TE record */
	templ = 0L;
	templ += scrollback;
	if ((*tempText)->teLength > templ) {
		sStart = 0L;
		sEnd = (*tempText)->teLength - templ;
		TEDeactivate(tempText); 
		TESetSelect(sStart,	sEnd, tempText);
		TEDelete(tempText);
		TESetSelect(templ, templ, tempText);
		TEActivate(tempText);
	}
}


ChangeMouse(tempText)
	TEHandle	tempText;
{
	Point	MousePt;
	if (FrontWindow() == (WindowPtr) (*tempText)->inPort) {
		GetMouse(&MousePt);
		if (PtInRect(MousePt,&(*tempText)->viewRect))
			SetCursor(*iBeam);
		else
			SetCursor(&arrow);
	} 
}

TEHandle	unpackTEH(theWin)
	WindowPtr	theWin;
{
	TEHandle	tempText;
	WSHandle	tempWS;
	
	tempWS = (WSHandle)GetWRefCon(theWin);
	return((*tempWS)->theTEH);
}

	
adjustTESizeTo(size,tempText)
	TEHandle	tempText;
	int			size;
{	/* cut it back to "size" bytes */
	long			sStart, sEnd, templ;
	templ = 0L;
	templ += size;
	if ((*tempText)->teLength > templ) {
		sStart = 0L;
		sEnd = (*tempText)->teLength - templ;
		TEDeactivate(tempText); 
		TESetSelect(sStart,	sEnd, tempText);
		TEDelete(tempText);
		TESetSelect(templ, templ, tempText);
		TEActivate(tempText);
	}
}



int	insertAtEnd(tempText)		/* find out if the insertion point is at the end */
	TEHandle		tempText;
{
	int		sStart, sEnd, textEnd, cCount;
	Handle	TextH;
	
	sStart = (*tempText)->selStart;
	sEnd =   (*tempText)->selEnd;
	textEnd = (*tempText)->teLength;
	
	/* Return true if beginning of selection range is beyond the prompt
	 * We assume this will only be called when the last line has a prompt */
	
	TextH = (*tempText)->hText;
	/* number of characters in the current line=>cCount */ 
	HLock((Handle)TextH);
	cCount = 1;
	while ((*TextH)[textEnd-cCount] != RET) cCount++;
	HUnlock((Handle)TextH);
	if (sStart > textEnd - cCount + strlen(PROMPT))
		return(1);
	else
		return(0);
}

doAbout()
{
	int		result;
/*	Rect		creditR,lineR;
	GrafPtr		port;
	WindowPtr	creditW;
	EventRecord	anEvent;
*/
	char		*line1=(char *)"\pMacShell - Copyright Fred Videon, 1992-96";
	char		*line2=(char *)"\pIt is prohibited to charge money for this application.";
	char		*line3=(char *)"\pEmail bug reports to fred@cs.washington.edu";
	char		*line4=(char *)"\pUse this application at your own risk.";
	
	ParamText((unsigned char *)line1,(unsigned char *)line2,(unsigned char *)line3,(unsigned char *)line4);	
	result = Alert(128,0L); 


/*	GetPort(&port);
	SetRect(&lineR,5,5,365,20);
	SetRect(&creditR,65,120,435,220);
	creditW = NewWindow((WindowPeek) 0L, &creditR, "\1x",0xff, dBoxProc,
		(WindowPtr)-1L,0xff,0);
	SetPort(creditW);
	
	TextSize(12);
	TextFont(0);
	TextBox(line1,strlen(line1),&lineR,1);
	OffsetRect(&lineR,0,20);
	TextBox(line2,strlen(line2),&lineR,1);
	OffsetRect(&lineR,0,20);
	TextBox(line3,strlen(line3),&lineR,1);
	OffsetRect(&lineR,0,20);
	TextBox(line4,strlen(line4),&lineR,1);

	do {
		GetNextEvent(everyEvent,&anEvent);
	} while (anEvent.what != mouseDown);
	
	DisposeWindow(creditW);
	
	SetPort(port);
	return;	
*/

}

ClikLoop()
{
	asm {
			movem.l	D1-D7/A0-A6,-(A7)	;D0 and A0 need not be saved.
			jsr	CClikLoop				;Execute clikLoop.
			movem.l	(A7)+,D1-D7/A0-A6	;restore the world as it was 
			MOVEQ.L		#1,D0
			RTS
		}
}

char CClikLoop()
{
	Rect	*viewR, *destR;
	Point	mousePt;
	int		lineHeight, destBottom, delta, startValue;
	RgnHandle		rgn;
	WindowPtr		window, oldPort;
	WSHandle		tempWS;
	ControlHandle	tempCH;

	HLock((Handle)gTextH);

	GetPort(&oldPort);
	SetPort(window = (WindowPtr)(*gTextH)->inPort);
	
	viewR = &(*gTextH)->viewRect;

	GetMouse(&mousePt);
	
	if(!PtInRect(mousePt, viewR)) {
		destR = &(*gTextH)->destRect;
		lineHeight = (*gTextH)->lineHeight;
		tempWS = (WSHandle)GetWRefCon(window);
		tempCH = (*tempWS)->theCH;
		startValue = GetCtlValue(tempCH);
		
		rgn = NewRgn();
		GetClip(rgn);
		ClipRect(&(window->portRect));
			/* The clipRgn is set to protect everything outside viewRect.
			** This doesn't work very well when we want to change
			** the scrollbars.  Save the old and open it up.
			*/

		if (mousePt.v > viewR->bottom) {
			destBottom = destR->top + ((*gTextH)->nLines)*lineHeight;
			if (viewR->bottom < destBottom) {
				if ((destBottom - viewR->bottom) < lineHeight)
					delta = destBottom - viewR->bottom;
				else delta = lineHeight;
				SetCtlValue(tempCH, startValue + delta);
				TEScroll(0,-delta,gTextH);
			}
		} else if ((mousePt.v < viewR->top) &&
				  (viewR->top > destR->top)) {

			if ((viewR->top - destR->top) < lineHeight) 
				delta = viewR->top - destR->top + 2;
			else delta = lineHeight;
			SetCtlValue(tempCH, startValue - delta); 
			TEScroll(0,delta,gTextH); 
		}
		SetClip(rgn);		/* restore clip */
		DisposeRgn(rgn);
			/* Make Mr. clipRgn happy again. */

	} 
	SetPort(oldPort);

	HUnlock((Handle)gTextH);
}

