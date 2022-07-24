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
 *  NOTES ABOUT USING THIS SOURCE WITH DIFFERENT COMPILERS
 *
 * This was originally done with ThinkC 3.0.  In 1995 it was ported to 
 * ThinkC 7 (Substantial work by Greg Dunn).  Starting with version 0.54b, 
 * issues pertaining to backward compatibility with TC3 have been 
 * neglected.  The following notes address a few items of interest to
 * those who may be using this source with different versions
 * of ThinkC, or other compilers altogether:
 *
 * -The definition of 'TC7' in macshell.h can be used to indicate to some extent 
 * which version of Think C is in use.  Beginning with version 0.54, clearing it 
 * will not necessarily result in compatibility with TC3.  
 *
 * -I used ThinkC 7's 'infer prototypes', and version 3's 'require 
 * prototypes'.  Some prototypes are TC3 only, because TC7 requires ANSI
 * style function declarations if one uses prototypes with certain types 
 * of arguments.  If old style function declarations are used, TC7 complains
 * about invalid redeclarations.  Since ANSI style declarations aren't 
 * recognized under TC3, the workaround I settled upon was to let these
 * prototypes be inferred under TC7.  Beginning with MacShell 0.54b, some ANSI 
 * style declarations have been used.  These function declarations and thier 
 * Prototypes will need to be adjusted to accomodate earlier versions.
 *
 * Other contributions or suggestions which further enhance the portability 
 * of this source would be appreciated.
 */
 
#include "MacShell.h"
#include "regexp.h"
#include "man.h"
#include "variable.h"
#include "test.h"
#include "script.h"
#include "fileTools.h"
#include "window.h"
#include "menu.h"
#include "standalone.h"
#include "shellutils.h"
#include "alias.h"
#include "copy.h"
#include "dirtools.h"
#include "event.h"
#include "history.h"
#include "backquote.h"
#include "stdio.h"

/* This file contains code for main and testing */

/* Globals  */
int 			countScriptWins; 				/* Current number */
int				totalScripts;					/* Number created since launching. */
int				gDone;							/* How we know when we're done. */
int				gSeq;							/* Sequence number used in temp file names */
WindowPtr		diagnosticWin,interactiveWin; 	/* These two windows always exist. */
WLRec			gWLRoot; 						/* Root of linked list for the window menu */
CursHandle 		iBeam;
int				gMyResFile;						/* My resource file ID */
MSProcRec		gProcRec;						/* Pointers to MacShell functions callable
												 * from external code resources */
int				gInProc;						/* true when using inProcEventLoop */
TEHandle		gTextH;

StdioPtr		gStdiop;

/* globals for shell preferences */
short		gRemapEsc;  /* use backquote for name completion */
short		gTabNC;		/* use tab for name completion */

doTest(argc, argv, stdiop)
	int argc;
	char **argv;
	StdioPtr stdiop;
{ /* A place to plug in things for debugging purposes. 
   * You get here when the user types 'test' in the 
   * interactive window. */
    short err;
 	char *temp = (char *)"testing\r";
 	
	/* Debugger(); */
	
}


main()
{	
	MaxApplZone();	/* Expand the heap so code segments load at the top. */
	initialize();	/* Init lots of stuff */
	runConfig();	/* Execute MacShell.rc */
	if (!gDone) {
		startupFiles(); /* Open and sometimes execute startup files */
		showAllWindows();  	/* Show all the windows, and we're ready to go. */	
		initPrompt(); 	/* Put up the initial prompt */
		/* UnloadSeg((Ptr)initialize());  We should dump the init segment here. */
		doEventLoop();
	}		
}
