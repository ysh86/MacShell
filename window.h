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
 
/* 
 * Data structures for windows
 */
struct _WStuff {
	TEHandle		theTEH;
	ControlHandle	theCH;
	int				vRef;		/* wd of file */
	char			fname[255];	/* file name */
	int				vers;		/* version */
	int				dirty;
};
#define	WindowStuff struct _WStuff
typedef	WindowStuff	*WSPtr;
typedef	WSPtr		*WSHandle;


/* prototypes fro window stuff */
extern 		openDiagnosticWin(void);
extern 		openInteractiveWin(void);
extern 		WindowPtr	createNewWindow(Rect bounds);
extern 		setDirtyBit(WindowPtr window);
extern 		windowListInsert(WindowPtr wp);
extern 		int	isAppWindow(WindowPtr window);
extern 		windowListDelete(WindowPtr wp);
extern 		SetLineHeight(short *spacing, TEHandle hTE);
extern 		moveScrollBar(WindowPtr whichWin); 
extern 		pascal void scrollGlue(ControlHandle tempCH,int partcode); 
extern 		vertScrollContents(int howmuch);
extern 		doThumb(int start,int end,TEHandle tempText,ControlHandle tempCtl);
extern 		updateThumbValues(WindowPtr theWin);
extern 		ReSizeTE(WindowPtr tempWindow);
extern 		adjustTEPos(WindowPtr tempWindow);		
extern 		ScrollInsertPt(TEHandle hTE);  
extern 		adjustScrollValues(TEHandle teHndl, ControlHandle ctl);
extern 		adjustTESize(TEHandle tempText);
extern 		ChangeMouse(TEHandle tempText);
extern 		TEHandle	unpackTEH(WindowPtr theWin);
extern 		adjustTESizeTo(int size,TEHandle tempText);
extern 		int	insertAtEnd(TEHandle tempText);
extern 		doAbout(void);
extern		putCursorAtEnd(TEHandle tempText);
extern		ClikLoop(void);
extern		char CClikLoop(void);
extern		showAllWindows(void);

