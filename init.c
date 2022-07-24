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
#include "variable.h"
#include "script.h"
#include "window.h"
#include "menu.h"
#include "filetools.h"
#include "shellutils.h"
#include "alias.h"
#include "standalone.h"
#include "history.h"
#include "stdio.h"

/* This file contains code to initialize the program, open startup 
 * files, execute startup scripts, execute
 * config file, put up initial prompt */
 
initialize() 
{
	short	gMachineType;
	
	/* get data from the gestalt manager, verify that we have the 128K rom, 
		make sure we really have enough memory */
	/* if ((gMachineType < gestaltMac512KE) || (!gHasWaitNextEvent)) (can't do this with ThinkC 3.0)
		DeathAlert(rBadNewsStrings, sWimpyMachine);  */
	
	/* if ((long) GetApplLimit() - (long) ApplicZone() < 250) */
		/* DeathAlert(rBadNewsStrings, sHeapTooSmall)*/ ;

	/* initialize managers */
	InitGraf(&thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(NIL);
	InitCursor();
	
	/* initialize globals */
	iBeam = GetCursor(iBeamCursor);
	gMyResFile = CurResFile();
	countScriptWins = 0;
	totalScripts = 0;
	gSeq = 0;
	gDone = FALSE;
	gWLRoot.next = 0L; /* root of linked list for the window menu */
	gWLRoot.prev = 0L;
	gWLRoot.wptr = 0L;
	gInProc = FALSE;
	gTextH = 0L;
	gRemapEsc = FALSE; /* used to fake escape key on older keyboards */
	gTabNC = FALSE; /* use tab key to trigger name completion instead of esc. */
	gVarCount = 0; /* current number of shell variables defined */
	gAliasCount = 0; /* current number of shell aliases defined */
	gPathCount = 0; /* number of directories in the path */
	gManpathCount = 0; /* number of directories in the manpath */

	gHistStart = 0;
	gHistLen = 0;
	gHistIndex = 1L;
	gHistCurrent = 1L;
	
	/* setup menus */
	standardMenuSetup(rMenuBar, mApple);

	FlushEvents(everyEvent, 0);

	/* open windows */	
	openDiagnosticWin();	
	openInteractiveWin(); 
	
	/* open startup files a bit later */
			
	MoreMasters();		/* how many? */
	MoreMasters();
	MoreMasters();
	MoreMasters();
	MoreMasters();
	
	
	doFree();	/* show free heap */	
	
}


startupFiles()
{
	short		whatToDo;
	short		numberOfFiles;
	int		i,len;
	OSErr		err;
	AppFile		theAppFile;
	WindowPtr	wp;
	
	CountAppFiles(&whatToDo, &numberOfFiles);
	if (numberOfFiles > 0) {
		err = 0;
		
		for (i = 1; (i <= numberOfFiles) && (!err); ++i) {
			GetAppFiles(i, &theAppFile);
			ClrAppFiles(i);
			/* open the file */
			openFile((char *)theAppFile.fName, theAppFile.vRefNum, (short)theAppFile.versNum,&wp);
			/* note that openFile does not do ShowWindow, and returns null in case of an error */
			/* If it has the magic cookie, execute it. */
			if ((wp!=0L) && (isExecutable(wp))) {
				runStartup((char *)theAppFile.fName,theAppFile.vRefNum);
			}
		}
	}
}


int isExecutable(wp)
	WindowPtr	wp;
{
/* Look for the magic cookie at the beginning of this te Rec. 
 * Return true if it's found.
 * Maybe this should check the file type too. */
 	TEHandle	teh;
 	Handle		texth;
	teh = unpackTEH(wp);
	texth = (*teh)->hText;
	if (((*texth)[0] == '#') && ((*texth)[1] == '!')) 
		return(1);
	return(0);
}


showAllWindows()
{
	WLPtr			tempnode;
	
	SelectWindow(interactiveWin);    	
	ShowWindow(interactiveWin);
	ShowWindow(diagnosticWin);
	
	/* show file windows */
	tempnode = &gWLRoot;
	while (tempnode->next != 0L) {
		tempnode = (WLPtr)tempnode->next;
		SelectWindow(tempnode->wptr);
		ShowWindow(tempnode->wptr);
	}	
}


runStartup(fname,vref)
	char 	*fname;
	int		vref;
{
	char		path[512];
	char		*exmsg = "Executing file: ";
 	int			argc = 2;
 	char		*argv[3];
	char		*run = "run";
	char		*nullc = "\0";
	char		*stdioerr = "Error with stdio while running startup file. \r";
	char		*configerr = "Error running startup file.\r";
	StdioPtr	stdiop;
	int			err,len;

	/* get full path */
	getPath(vref,0L,path);
	len = strlen(path);
	BlockMove((fname+1),(path+len), fname[0]);
	path[fname[0]+len] = '\0';
	
	showDiagnostic(exmsg);
	showDiagnostic(path);
	showDiagnostic(RETURNSTR);
	
	/* construct command */
	argv[0] = run;
	argv[1] = path;
	argv[2] = nullc;
	
	stdiop	= (StdioPtr)NewPtr(sizeof(StdioRec)); /* storage for the stdio record */
	/* setup default values */
	err = setStdioDefaults(stdiop);
	if (err) {
		showDiagnostic(stdioerr);
		return;
	}

	err = doSource(argc,argv,stdiop);
	if (err) {
		return;
	}
	
	flushStdout(stdiop,stdiop->outLen); 
	/* dispose of two io buffers and the stdio rec itself. */
	disposStdioMem(stdiop);
}


runConfig()
{
/* look for the file called "macshell.rc"  If it's present in the cwd, execute it as
 * a script. */
 	int		argc = 2;
 	char	*argv[3];
	char	*run = "source";
	char	*fname = "macshell.rc";
	char	*pname = (char *)"\pmacshell.rc";
	char	*nullc = "\0";
	char	*stdioerr = "Error with stdio while running config file. \r";
	char	*configerr = "Error running config file.\r";
	char	*noconfig = "Configuration file (macshell.rc) not found.\r";
	StdioPtr	stdiop;
	int			err;
 	FInfo		fi;
 	OSErr		ose;
	/* is it there? */	
	ose = GetFInfo((unsigned char *)pname,0,&fi);
	if (ose != 0){
		showDiagnostic(noconfig);
		return;
	}
	
	/* construct command */
	argv[0] = run;
	argv[1] = fname;
	argv[2] = nullc;
	
	stdiop	= (StdioPtr)NewPtr(sizeof(StdioRec)); /* storage for the stdio record */
	/* setup default values */
	err = setStdioDefaults(stdiop);
	if (err) {
		showDiagnostic(stdioerr);
		return;
	}

	err = doSource(argc,argv,stdiop);
	if (err) {
		return;
	}
	
	flushStdout(stdiop,stdiop->outLen); 
	/* dispose of two io buffers and the stdio rec itself. */
	disposStdioMem(stdiop);
	
}


initPrompt()
{
	TEHandle	teh;
	
	teh = unpackTEH(interactiveWin);
	putUpPrompt(teh);

	/* need this just in case the config file dumped some text to the screen */
	adjustTEPos(interactiveWin);		
	updateThumbValues(interactiveWin);

}

