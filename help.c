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
#include "man.h"
#include "variable.h"
#include "standalone.h"
#include "copy.h"

/* This file contains code for man */

doMan(argc,argv,stdiop)
	int			argc;
	char		**argv;
	StdioPtr	stdiop;
{
	char	*usage = "Usage:man commandName\r";
	char	*resnf = "Man: Resource file not found.\r";
	char	*nofile = " (File not found, or wrong file type/creator.) \r";
	char	*dfltentry = "intro";
	short	err,eerr,slen;
	char	cname[256], *path;
	Handle	resH;
	
	if (argc > 2) {
		showDiagnostic(usage);
		return;
	}
	
	UseResFile(gMyResFile);
	if (err = ResError()) {
		showDiagnostic(resnf);
		return;
	}
	
	if (argc == 1) {
		BlockMove(dfltentry,cname,(Size)(strlen(dfltentry)+1)); 
	} else {
		BlockMove(argv[1],cname,(Size)(strlen(argv[1])+1)); 
	}
	CtoPstr(cname);
	
	resH = GetNamedResource(manResType,(unsigned char *)cname);
	if (!ResError()) {
		HLock((Handle)resH);
		stringMore(*resH,stdiop);
		HUnlock((Handle)resH);
	} else {
		/* It's not in the main resource file, so try to open a file
		 * named "argv[1]" and find the resource there. */
		if ((countColons(argv[1])==0) && (gManpathCount >0)) { 
			if (!(lookupInPath(argv[1],gManpathList,gManpathCount,&path,0))) {
				/* no match */
				manErr(argv[1],nofile);
				return;
			}
		} else { /* just use the path given */
			slen=strlen(argv[1]);
			if(!(path = NewPtr(slen+1))) {
				showDiagnostic(gMEMERR);
				return;
			}
			BlockMove(argv[1],path,slen+1);
		}
	
		/* assert: path is an allocated string here */ 
		eerr = externalMan(path,stdiop);
		DisposPtr((Ptr)path);
		
	}	
}



short externalMan(name,stdiop)
	char	*name;
	StdioPtr	stdiop;
{ /* try to open a file named 'name' and read 'help' resorce named
   * 'name' from it.  If all goes well, we'll spit it out with
   * stringMore, close the file and return.  Issue the appropriate
   * error message.  Return 1 for error.
   */
    short		refnum, testval, err;
	char		fname[256],path[256];
	Handle		resH;

	char		*wrongres = " (No such resource type/ID.) \r";
	char		*wrongtype = " (Wrong file type or creator.) \r";
	char		*nofile = " (No such file, or no resource fork.)\r";
	char		*wrongfile = " (Wrong resource file!) \r";
	char		*notme = " (Got the current resource fork!)\r";
	

	strcpy(path,name);
	getFName(path,fname);
	CtoPstr(fname);
	CtoPstr(path);
	err = 1;
	refnum = OpenResFile((unsigned char *)path);
	/* ouch! Things got really wedged when we closed the app's res.file */
	if ((refnum != -1) && (refnum != gMyResFile)) {
		resH = GetNamedResource(manResType,(unsigned char *)fname);
		if (!ResError()) {
			testval = HomeResFile(resH); 
			/* make sure the resource really came from the right file */
			if (refnum == testval) {
				if (checkFInfo(path)) {
					HLock((Handle)resH);
					stringMore(*resH,stdiop);
					HUnlock((Handle)resH);
					err=0;
				} else {
					manErr(name,wrongtype);
				}
			} else { 			
				manErr(name,wrongfile);
			}
			ReleaseResource(resH);
		} else { 
			manErr(name,wrongres);		
		} 
		CloseResFile(refnum); 	
	} else if (refnum == -1) { 
		manErr(name,nofile); 
	} else {
		manErr(name,notme);
	}
	return(err);
}

manErr(name,msg)
	char	*name,*msg;
{
	char	*noentry = "No manual entry for: ";
	showDiagnostic(noentry);
	showDiagnostic(name);
	showDiagnostic(msg);
}
