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
/* This file contains code for the file menu stuff: open, new, save, 
 * save as, close */

doNewScript() /* Read: newFile */
{
/*  If we're not to the maximum, set up a new untitled window. */
	Rect		bounds;
	int			wr;
	WindowPtr	wp;
	char		title[255];
	char		*toomany = "Sorry, no more windows.";
	MenuHandle	mh;
	
	if (countScriptWins == MAXSCRIPTS) {
		errAlert(toomany);
		return;
	}
	totalScripts++;
	countScriptWins++;
	/* only the first five get staggered nicely...*/
	
	/* should limit bounds.right in case we're on a 2 page monitor */
	wr = screenBits.bounds.right - 40;
	if (wr > MAXWDTH) { wr = MAXWDTH; }

	SetRect(&bounds, 	screenBits.bounds.left + 20 + (countScriptWins * 5),
						screenBits.bounds.top + 40 + (countScriptWins * 10),
						wr + (countScriptWins * 5),
						screenBits.bounds.bottom - 20 + (countScriptWins * 5));

	sprintf(title,"Untitled%ld",(long)totalScripts);
	wp = createNewWindow(bounds);
	CtoPstr(title);
	SetWTitle(wp,(unsigned char *)title);
	windowListInsert(wp);
	mh = GetMHandle(mWindow);	
	AppendMenu(mh,(unsigned char *)title);
	ShowWindow(wp);
}

openFile(fname, vref, vers,wptrp)
char	*fname;
int		vref;
short	vers;
WindowPtr *wptrp;
{
 	char			*toolong = "Sorry, file too long.";
 	char			*memerr = "Sorry, out of memory.";
 	Rect			bounds;
 	int				numTypes,wr;
 	long			textLength,size;
	ParamBlockRec	pb;
	OSErr			theErr;
	WSHandle		tempWS;
	WindowPtr		wp;
	TEHandle		tempTE;
	Handle			tempTH;
	CursHandle		watchH;
	Ptr				textPtr;
	MenuHandle		mh;
		
	pb.ioParam.ioNamePtr = (StringPtr)fname;
	pb.ioParam.ioVRefNum = vref;
	pb.ioParam.ioVersNum = vers;
	pb.ioParam.ioPermssn = fsRdPerm;
	pb.ioParam.ioMisc = 0L;
	
	theErr = PBOpen(&pb,FALSE);
	if (theErr) {
 		showFileDiagnostic(fname,theErr);
 		*wptrp = NULL;
 		return;
	}
	
	theErr = PBGetEOF(&pb,FALSE);
	if (theErr) {
 		showFileDiagnostic(fname,theErr);
 		PBClose(&pb,FALSE);
 		*wptrp = NULL;
 		return;
	}
	
	if ((textLength = (long)pb.ioParam.ioMisc) >= 30*1024) {
		errAlert(toolong);
		PBClose(&pb,FALSE);
 		*wptrp = NULL;
 		return;
	}

	totalScripts++;
	countScriptWins++;
	
	/* in case we're on a 2-page monitor */
	wr = screenBits.bounds.right - 40;
	if (wr > MAXWDTH) { wr = MAXWDTH; }

	SetRect(&bounds, 	screenBits.bounds.left + 20 + (countScriptWins * 5),
						screenBits.bounds.top + 40 + (countScriptWins * 10),
						wr + (countScriptWins * 5),
						screenBits.bounds.bottom - 20 + (countScriptWins * 5));
	wp = createNewWindow(bounds);
	SetWTitle(wp,(unsigned char *)fname);	
	
	tempWS = (WSHandle) GetWRefCon(wp);
	tempTE = (*tempWS)->theTEH;
	tempTH = (*tempTE)->hText;
	
	SetHandleSize(tempTH,textLength);
	size = GetHandleSize(tempTH);
	
	if (size != textLength) {
		errAlert(memerr);
		PBClose(&pb,FALSE);
 		DisposeWindow(wp);
		countScriptWins--;
 		*wptrp = NULL;
 		return;
	}
	
	watchH = GetCursor(watchCursor);
	SetCursor(*watchH);
	
	HLock((Handle)tempTH);
	textPtr = *tempTH;
	
	pb.ioParam.ioBuffer = textPtr;
	pb.ioParam.ioReqCount = textLength;
	pb.ioParam.ioPosMode = fsAtMark;
	pb.ioParam.ioPosOffset = 0L;

	theErr = PBRead(&pb,FALSE);
	if ((theErr) && (theErr != eofErr)) {
		showFileDiagnostic(fname,theErr);
	}
		
	PBClose(&pb,FALSE);
	HUnlock((Handle)tempTH);
	
	TECalText(tempTE); /* calculates line starts */
	(*tempTE)->teLength = textLength;
	
	HLock((Handle)tempWS);
	pstrcpy((*tempWS)->fname,fname);

	(*tempWS)->vRef = vref;
	(*tempWS)->vers = vers;
	(*tempWS)->dirty = FALSE; 
 	SetWRefCon(wp, (long)tempWS);
 	HUnlock((Handle)tempWS);
	
	updateThumbValues(wp);
	
	windowListInsert(wp);
	mh = GetMHandle(mWindow);	
	AppendMenu(mh,(unsigned char *)(*tempWS)->fname);

	SetCursor(&arrow);
	
 	*wptrp = wp;
 	return;
	
}


doOpen()
{ 
/* If we're not over the window limit, get the file, and open it.  
 * If it's over 32k, bail out. Open a new window, Read the file into
 * its TERec. set the WRefCon data. */
 	char			*toomany = "Sorry, no more windows.";
 	Point			dialogLoc;
 	char			*prompt = (char *)"\pOpen file...";
 	Str255			defaultName;
 	int				numTypes;
	SFReply			reply;
	OSErr			theErr;
	WindowPtr		wp;
	
	SetPt(&dialogLoc,100,80);

	if (countScriptWins == MAXSCRIPTS) {
		errAlert(toomany);
		return;
	}
 	numTypes = -1;  /* all types */
	SFGetFile(dialogLoc,(unsigned char *)prompt,0L,numTypes,0L,0L,&reply);
	if (!reply.good) return;
	
	openFile((char *)&reply.fName, reply.vRefNum, (short)reply.version,&wp);
	if (wp != 0L) ShowWindow(wp);
	
}

int doSaveAs(window)
	WindowPtr	window;
{
 /* We can do this with any window.  */
 /* returns false (0) if not saved for any reason. */
 	Point			dialogLoc;
 	char			*prompt = (char *)"\pSave file as...";
 	char			fname[256];
 	Str255			defaultName;
	SFReply			reply;
	ParamBlockRec	pb;
	OSErr			theErr;
	WSHandle		tempWS;
	FInfo			finfo;
	
	SetPt(&dialogLoc,100,80);
	GetWTitle(window,(unsigned char *)&defaultName);
	SFPutFile(dialogLoc,(unsigned char *)prompt,defaultName,0L,&reply);
	if (!reply.good) return(0);
	
	/* if file exists, shouldn't try to create it again. */
	
	pstrcpy(fname,(char *)&reply.fName);
	
	pb.ioParam.ioCompletion = 0L;
	pb.ioParam.ioNamePtr = (StringPtr)fname;
	pb.ioParam.ioVRefNum = reply.vRefNum;
	pb.ioParam.ioVersNum = reply.version;
	
	theErr = PBCreate(&pb,FALSE); 	
 	if ((theErr) && (theErr != dupFNErr)) {
 		showFileDiagnostic((char *)"\p",theErr);
 		return(0);
 	}
 	
 	/* set file info */
 	pb.fileParam.ioFDirIndex = 0;
 	theErr = PBGetFInfo(&pb,FALSE);
 	if (theErr) {
 		showFileDiagnostic(fname,theErr);
 	}
 	pb.fileParam.ioFlFndrInfo.fdType = textFileType;
 	pb.fileParam.ioFlFndrInfo.fdCreator = docCreator;
 	theErr = PBSetFInfo(&pb,FALSE);
 	if (theErr) {
 		showFileDiagnostic(fname,theErr);
 	}
 	PBFlushFile(&pb,FALSE);
 	
 	/* put file name, etc. in WRefCon */
 	tempWS = (WSHandle) GetWRefCon(window);
 	HLock((Handle)tempWS);
 	pstrcpy((*tempWS)->fname,(char *)&reply.fName);
 	(*tempWS)->vRef = reply.vRefNum;
 	(*tempWS)->vers = reply.version;
 	HUnlock((Handle)tempWS);
 	SetWRefCon(window,(long)tempWS);
 	
 	if ((window != diagnosticWin) && (window != interactiveWin)) {
 		SetWTitle(window,(unsigned char *)&reply.fName);
 		redrawWinMenu();	
 	}
 	
 	setDirtyBit(window); /* force a save */
 	doSave(window);
 	return(1);
}


	
int doSave(window)
	WindowPtr	window;
{
/* If we have no file, call doSaveAs.  If it's not dirty, just return.
 * Otherwise, open, write and close.  Reset dirty flag. */
/* return false if we didn't save for any reason */
	WSHandle		tempWS;
	ParamBlockRec	pb;
	OSErr			theErr;
	Handle			tempTH;
	TEHandle		tempTEH;
	long			textLen;
	Ptr				textPtr;
	CursHandle		watchH;
	int				ret;
	
	tempWS = (WSHandle)GetWRefCon(window);
	
	if (!(*tempWS)->dirty) return;
	
	if (((*tempWS)->fname)[0] == 0) {
		ret = doSaveAs(window);
		return(ret);
	}
	
  	/* open */
	pb.ioParam.ioCompletion = 0L;
	pb.ioParam.ioNamePtr = (StringPtr)(*tempWS)->fname;
	pb.ioParam.ioVRefNum = (*tempWS)->vRef;
	pb.ioParam.ioVersNum = (*tempWS)->vers;
	pb.ioParam.ioPermssn = fsWrPerm;
	pb.ioParam.ioMisc = NIL;
	
	theErr = PBOpen(&pb,FALSE);	
	if (theErr) {
 		showFileDiagnostic((*tempWS)->fname,theErr);
 		return(0);
	}
	
	/* write */	
	tempTEH = (*tempWS)->theTEH;
	tempTH = (*tempTEH)->hText;
	textLen = (*tempTEH)->teLength;
	
	pb.ioParam.ioMisc = (Ptr) textLen;
	theErr = PBSetEOF(&pb,FALSE);
	if (theErr) {
		showFileDiagnostic((*tempWS)->fname,theErr);
		PBClose(&pb,FALSE);
		return(0);	
	}
	pb.ioParam.ioMisc = 0L;
	pb.ioParam.ioPosMode = fsFromStart;
	pb.ioParam.ioPosOffset = 0L;
	theErr = PBSetFPos(&pb,FALSE);
	if (theErr) {
		showFileDiagnostic((*tempWS)->fname,theErr);
		PBClose(&pb,FALSE);
		return(0);	
	}
	
	watchH = GetCursor(watchCursor);
	SetCursor(*watchH);
	HLock((Handle)tempTH);
	textPtr = *tempTH;
	pb.ioParam.ioBuffer = textPtr;
	pb.ioParam.ioReqCount = textLen;
	pb.ioParam.ioPosMode = fsAtMark;
	pb.ioParam.ioPosOffset = 0L;
	
	theErr = PBWrite(&pb,FALSE);
	if (theErr) {
		showFileDiagnostic((*tempWS)->fname,theErr);
		PBClose(&pb,FALSE);
		return(0);	
	}
	
	/* close */
	theErr = PBClose(&pb,FALSE);
	if (theErr) {
 		showFileDiagnostic((*tempWS)->fname,theErr);
 		return(0);
	}
	PBFlushVol(&pb,FALSE);
	HUnlock((Handle)tempTH);
	
	/* clear dirty bit and set WRefCon */
	(*tempWS)->dirty = FALSE;
	SetWRefCon(window,(long)tempWS);
	
	SetCursor(&arrow);
	return(1);
	/* InitCursor(); */
}

closeScript(window) /* close file */
	WindowPtr	window;
{
/* if the dirty bit is set, save, or saveAs.  otherwise, just release memory
 * and close of the window. */
	WSHandle	tempWS;
	char		*fname;
	OSErr		theErr;
	int			i;
	int			rval = 1;
	
	
	tempWS = (WSHandle)GetWRefCon(window);
	if ((*tempWS)->dirty) {
		/* show "save changes?" dialog here then doSave if yes */
		/* If we've quit here, we go into a loop if the user hits cancel */
		/* should dim the cancel button in this case */
		SetCursor(&arrow);
		i = Alert(200,0L); 
		if (i == 1) {
			rval = doSave(window);
		}
		if ((i == 2) || (rval == FALSE)){ /* cancel */
			return;
		}
	}
	
	windowListDelete(window);
	redrawWinMenu(); 
 	/* release memory... */
 	TEDispose((*tempWS)->theTEH);
 	DisposeControl((*tempWS)->theCH);
 	DisposHandle((Handle)tempWS);
 	CloseWindow(window);
 	DisposPtr((Ptr)(WindowPeek)window); 
	countScriptWins--;
}

