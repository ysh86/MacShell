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
#include "variable.h"
#include "standalone.h"
#include "copy.h"
#include "fileTools.h"
#include "script.h"
#include "event.h"
#include "shellutils.h"

/* This file contains all the stuff for calling external code resources, 
 * and all the stuff to support the path and manpath variables. */
 
/* globals */
PathHandle	gPathList[MAXPATH];
short		gPathCount;
PathHandle	gManpathList[MAXPATH];
short		gManpathCount;



short callCodeRes(argc, argv, stdiop)
 int argc;
 char **argv;
 StdioPtr stdiop;
{
 /* Call this after we've determined that a command refers to a file
  * with MacShell's creator, and type 'CODE'.
  * Proceed as follows:
  *   - Do GetResource to load 'CODE' 128.
  *   - HomeResFile to verify that we got the resource from the file we opened.
  *   - If everything looks ok, execute the resource.
  *   - ReleaseResource when done.
  *   - Return true or false depending on the success or failure of finding
  *     and executing the resource.
  *
  */
	int			c, refnum, testval;
	char		fname[256];
	ExtArgRec	argRec;
	int 		(**pH)();
	int			result; 
	char		*wrongres = "  (Wrong resource type/ID) \r";
	char		*wrongtype = "  (Wrong file type or creator) \r";
	char		*nofile = "  (No such file, or no resource fork) \r";
	char		*wrongfile = "  (Wrong resource file) \r";
	char		*notme = "Can't execute the current application. \r";
	

	strcpy(fname,argv[0]);
	CtoPstr(fname);
	result = 1;	
	
	refnum = OpenResFile((unsigned char *)fname);
	/* ouch! Things got really wedged when we closed the app's res.file */
	if ((refnum != -1) && (refnum != gMyResFile)) {
		pH = (int (**)())GetResource('CODE',128);
		if (pH != NIL) {
			testval = HomeResFile((Handle)pH); 
			/* make sure the resource really came from the right file */
			if (refnum == testval) {
				if (checkFInfo(fname)) {
					initProcRec();
					argRec.Stdiop = stdiop;
					argRec.MSProc = &gProcRec;
					argRec.argc = argc;
					argRec.argv = argv;
				
					HLock((Handle)pH);
					result = (**pH)(&argRec);
					HUnlock((Handle)pH);
				} else {
					showCmdNotFound(argv[0]);
					showDiagnostic(wrongtype);
				}
			} else { 			
				showCmdNotFound(argv[0]);
				showDiagnostic(wrongfile);
			}
			ReleaseResource((Handle)pH);
		} else { 
			showCmdNotFound(argv[0]);
			showDiagnostic(wrongres);		
		} 
		CloseResFile(refnum); 	
	} else if (refnum == -1) { 
		showCmdNotFound(argv[0]); 
		showDiagnostic(nofile); 
	} else {
		showDiagnostic(notme);
	}
	
	return(result);
}



showCmdNotFound(cmd)
 char *cmd;
{
	char		*msg = "Command not found: ";
	showDiagnostic(msg);
	showDiagnostic(cmd);
	showDiagnostic(RETURNSTR);
}

initProcRec()
{
	gProcRec.SIGet = (Ptr)SIGet;
	gProcRec.SIRead = (Ptr)SIRead;
	gProcRec.SOInsert = (Ptr)SOInsert;
	gProcRec.SOFlush = (Ptr)flushStdout;
	gProcRec.GetC = (Ptr)extGetC;
	gProcRec.PrintDiagnostic = (Ptr)showDiagnostic;
}

short hasMagicCookie(fname,volID,dirID)
char	*fname;
short	volID;
long	dirID;
{
	HParamBlockRec	hpb;
	OSErr			theErr;
	long			cnt = 2;
	char			buf[3];
	
	
	hpb.ioParam.ioCompletion = 0L;
	hpb.ioParam.ioNamePtr	= (StringPtr)fname;
	hpb.ioParam.ioVRefNum = volID; 
	hpb.ioParam.ioPermssn = fsRdPerm; 
	hpb.ioParam.ioMisc = NIL; 
	hpb.fileParam.ioDirID = dirID; 
	theErr = PBHOpen(&hpb,FALSE);
	if (!theErr) {
		theErr = FSRead(hpb.ioParam.ioRefNum,&cnt,buf);
		if (!theErr && (buf[0] == '#') && (buf[1] == '!')) {
			theErr = FSClose(hpb.ioParam.ioRefNum);
			return(1);
		}
		theErr = FSClose(hpb.ioParam.ioRefNum);
	}
	return(0);

}



int checkFInfo(fname)
	char	*fname;
{
/* verify that the file type and creator is correct for one of our
 * standalone code resources.  If it is, return true.  */
	OSErr	err;
	FInfo	finfo;
	
	err = GetFInfo((unsigned char *)fname, 0, &finfo);
	if (!err) {
		if ((finfo.fdCreator == docCreator) &&
			(finfo.fdType == codeFileType)) {
			return(TRUE);
		}
	}
	return(FALSE);
}

int extGetC()
{
/* This is a wrapper for the inProcEventLoop to allow it to be used safely by 
 * external code resources.  The cursor blink is hardwired to "on", and
 * the routine puts the selection range back at the end of the text edit rec
 * before returning.  */
 
	int			c;
	TEHandle	tempText;
	
	
	c = inProcEventLoop(TRUE);
	tempText = unpackTEH(interactiveWin);
	putCursorAtEnd(tempText);
	return(c);
}

short isExe(fname, volID, dirID, finfo)
char	*fname;
short	volID;
long	dirID;
FInfo	finfo;
{
  /*
   * How we determine whether a file is 'executable':
   *  -require that the file creator be MacShell
   *  -require file type be 'CODE' or 'TEXT'.  If file type is 'TEXT'
   *   require that the first two characters in the data fork be '#!'.
   */
	if (finfo.fdCreator == docCreator) {
		if (finfo.fdType == codeFileType) return(1);
		else if (finfo.fdType == textFileType) {
			if (hasMagicCookie(fname,volID,dirID)) return(1);
		} 
	}
	return(0);
		
}

short lookupKey(key,listp,listc,indp)
char	*key;
Handle	*listp;
short	listc;
short	*indp;
{
/* if key exists in the list of handles listp, return true, and return the
 * index of the key in indp
 */
 short i=0,j;
 while (i<listc) {
 	j = 0;
 	while((key[j] == (*(listp[i]))[j]) && (key[j]!=0) && ((*(listp[i]))[j]!=0))	j++;
 	if ((key[j]==0) && ((*(listp[i]))[j]==0)) {
 		*indp = i;
 		return(1);
 	} 	
 	i++;
 }
 return(0);
 
}


short buildPathTable(dirp,ph,ispath)
char	*dirp;
PathHandle	ph;
short	ispath;
{ /* ispath == FALSE means that it's manpath, not path.  At this point they are
   * handled exactly the same, so we don't care about ispath.
   *
   * Proceed as follows:
   * - verify that dirp is a valid directory
   * - if it is, collect a list of executables, allocate storage, 
   *   and link into ph.
   * - if dirp is not valid, set (*ph)->listc to 0.
   * - if we run out of memory, return null.
   *
   */
 long	dirID;
 short	volID, theErr, n=0;
 char	fname[256];
 char   *toomany="Warning:Too many executables in path directory.\r";
 CInfoPBRec	cpb;
 Handle	*temphlist;
 
 if (!(temphlist = (Handle *)NewPtr(sizeof(Handle)*MAXEXE))) return(0);

 if (validDir(dirp, &dirID)) {
 	volID = getVol(dirp);
	cpb.dirInfo.ioNamePtr = (StringPtr)fname;
	cpb.dirInfo.ioCompletion=0L;
	cpb.dirInfo.ioFDirIndex=1;  /* set index */
	cpb.dirInfo.ioDrDirID = dirID;
	cpb.dirInfo.ioVRefNum= volID;
	theErr = PBGetCatInfo(&cpb,FALSE); 
	while (!theErr) {
		if (cpb.dirInfo.ioFlAttrib & 0x10) { /* it's a directory */
		} else { 
			/* if executable, add to the list */
			if (isExe(fname, volID, dirID,cpb.hFileInfo.ioFlFndrInfo)) {
				if ( n >= MAXEXE ) {
					showDiagnostic(toomany); break;
				} 
				myPtoCstr(fname);
				if(!(temphlist[n] = NewHandle(strlen(fname)+1))) {
					DisposPtr((Ptr)temphlist); return(0);
				}
				HLock((Handle)temphlist[n]);
				BlockMove(fname,*(temphlist[n]),strlen(fname)+1);
				HUnlock((Handle)temphlist[n]);
				n++;
			}
		}
		cpb.dirInfo.ioDrDirID = dirID;
		cpb.dirInfo.ioFDirIndex++;
		theErr=PBGetCatInfo(&cpb,FALSE);
	}
	(*ph)->listc = n;
	if(!(((*ph)->listh) = (Handle **)NewHandle(sizeof(Handle)*n))) {
		DisposPtr((Ptr)temphlist); return(0);
	}
	HLock((Handle)((*ph)->listh));
	BlockMove(temphlist,*((*ph)->listh),sizeof(Handle)*n);
	HUnlock((Handle)((*ph)->listh));
 } else {
 	(*ph)->listc = 0;
 }
 DisposPtr((Ptr)temphlist);
 return(1);
}


setPath(apath,cntp,vh,ispath)
PathHandle	*apath;
short	*cntp;
VarHandle	vh;
short	ispath;
{ /* For each item in the list in the struct pointed to by *vh, we do the 
   * following:
   * -check to make sure we don't have too many items in the list
   * -allocate a path record, and increment the count
   * -put the name of the directory in the path record.
   * -determine whether it's a relative or absolute reference, and set flag.
   * -if absolute, verify that it's a valid directory and construct a list of
   *  executable contents.
   *
   * Always verify that path is not currently set before calling this.
   * Calling this twice without clearing in between will result in
   * a bunch of orphaned handles.
   */
  char *toolong = "Warning: Path contains too many elements. \r";
  char *memerr = "Out of memory. \r";
  Handle temph;
  
  *cntp = 0; /* it should always be 0 here anyway */
  
  if ((*vh)->listc > MAXPATH) {
  	showDiagnostic(toolong);
  }
  
  while ((*cntp < MAXPATH) && (*cntp < (*vh)->listc)) {
  	if(!(apath[(*cntp)] = newPathElement())) {
  		showDiagnostic(memerr);
  		return;
  	}
  	temph = ((*vh)->listh)[(*cntp)];
  	HLock((Handle)temph);
  	if(!((*(apath[(*cntp)]))->dname = NewHandle(strlen(*temph)+1))) {
  		showDiagnostic(memerr);
  		return;
  	} 
  	HLock((Handle)(*(apath[(*cntp)]))->dname);
   	BlockMove(*temph,*((*(apath[(*cntp)]))->dname),(long)(strlen(*temph)+1));  	
   	HUnlock((Handle)(*(apath[(*cntp)]))->dname);

  	if (**temph == ':') { /* relative path */
  		(*(apath[(*cntp)]))->relf = TRUE;
  	} else { /* absolute path */
  		(*(apath[(*cntp)]))->relf = FALSE;
  		if (!buildPathTable(*temph,apath[(*cntp)],ispath)) {
   			showDiagnostic(memerr);
  			return;
  		}
   	}
  	HUnlock((Handle)temph);
  	(*cntp)++;
  }
}



PathHandle newPathElement()
{
/* Create storage for a path element, and initialize it. If there isn't enough memory 
 * to create the record, return NULL */

	PathHandle	ph;
	
	ph = (PathHandle)NewHandle(sizeof(PathRec));
	if (ph) {
		(*ph)->listc = 0;
	}
	return(ph);
}

short lookupInPath(key,pathl,pathc,resulth,ispath)
char	*key;
PathHandle	*pathl;
short	pathc;
char	**resulth;
short	ispath;
{ /* 
   * Look for key in the pathlist 'pathl'.  Return true or false depending on
   * success or failure, and if a match is found, return a pointer to the 
   * directory in *resulth.
   * Ispath is true if it's a path, not a manpath.  I think we'll deal with 
   * the two cases in exactly the same way, but...who knows!
   */
 short	match=0,ind,n=0,dlen;
 
 while (n < pathc) {
	if((*(pathl[n]))->relf) { /* relative reference */
		HLock((Handle)(*(pathl[n]))->dname);
		if (lookupRelative(key,*((*(pathl[n]))->dname))) 
			match = 1;
		HUnlock((Handle)(*(pathl[n]))->dname);
	} else if ((*(pathl[n]))->listc > 0) { /* some handles to check */
		HLock((Handle)(*(pathl[n]))->listh);
		if (lookupKey(key, *((*(pathl[n]))->listh),(*(pathl[n]))->listc,&ind)) 
			match = 1;
		HUnlock((Handle)(*(pathl[n]))->listh);
	}
	if (match) {
		/* setup resulth */
		HLock((Handle)(*(pathl[n]))->dname);
		if (!joinPath(key,*((*(pathl[n]))->dname),resulth)) {
			showDiagnostic(gMEMERR);
			return(0);
		}
		HUnlock((Handle)(*(pathl[n]))->dname);
		return(1);
	}
	n++;
 }
 return(0);
 
}


clearPath(apath,cntp,ispath)
PathHandle	*apath;
short		*cntp;
short		ispath;
{
/* dispose of the memory associated with the path and manpath 
 * variables, and reset globals.
 */
 PathHandle tempph;
 short	n;
 
 while (*cntp > 0) {
 	(*cntp)--;
 	tempph = apath[(*cntp)];
 	HLock((Handle)tempph);
 	n = (*tempph)->listc;
 	while ((*tempph)->listc > 0) {
 		((*tempph)->listc)--;
 		DisposHandle((Handle)(*((*tempph)->listh))[((*tempph)->listc)]);
 	}
 	DisposHandle((Handle)(*tempph)->dname);
 	if ( !((*tempph)->relf) && (n>0)) {
 		DisposHandle((Handle)(*tempph)->listh);
 	}
 	HUnlock((Handle)tempph);
 	DisposHandle((Handle)tempph);
 }
 	
}

short doWhich(argc,argv, stdiop)
short	argc;
char	**argv;
StdioPtr	stdiop;
{ /* Provide a way for the user to determine how a command word would
   * be resolved given the current path.  Return the path to the command
   * which would be executed.  Or error msg. if command not found.
   */
   	FInfo	finfo;
   	short 	slen,result;
   	char	*path;
   	char	*notvalid = "Not a valid command: ";
   	char	*notfound = "External command not found: ";
   	char	*usage = "Usage: which command \r";
	OSErr	err;
	
   	if (argc < 2) {
   		showDiagnostic(usage);
   		return(1);
   	}
   
  	/* path variable is set and the command doesn't seem to contain a path. */
	if ((countColons(argv[1])==0) && (gPathCount >0)) { 
		if (!(lookupInPath(argv[1],gPathList,gPathCount,&path,1))) {
			/* no match message*/
			SOInsert(stdiop,notfound,strlen(notfound));
			SOInsert(stdiop,argv[1],strlen(argv[1]));
			SOInsert(stdiop,RETURNSTR,1);
			return(1);
		} 
	} else { /* just use the path given */
		slen=strlen(argv[1]);
		if(!(path = NewPtr(slen+1))) {
			showDiagnostic(gMEMERR);
			return(1);
		}
		BlockMove(argv[1],path,slen+1);
	}
	
	CtoPstr(path);
	err = GetFInfo((unsigned char *)path, 0, &finfo);
	myPtoCstr(path);
	
	if (!err) {
		if (finfo.fdCreator == docCreator) {
			if (finfo.fdType == textFileType) {
				if (hasMagicCookie2(path)) {
					result=0;
				} else {
					result = 1;
				}
			} else if (finfo.fdType == codeFileType) {
				result = 0;
			} else {
				result = 1;
			}
		} else {
			result = 1;
		}
	} else {
		result = 1;
	}
	if(result == 0) {
		SOInsert(stdiop,path,strlen(path));
		SOInsert(stdiop,RETURNSTR,1);
	} else {
		SOInsert(stdiop,notvalid,strlen(notvalid));
		SOInsert(stdiop,path,strlen(path));
		SOInsert(stdiop,RETURNSTR,1);
		DisposPtr((Ptr)path);
		return(0);
	}	
	DisposPtr((Ptr)path);
	return(result);
}

short doRehash(argc, argv, stdiop)
short	argc;
char	**argv;
StdioPtr	stdiop;
{ /* Provides a way for the user to rebuild internal path and 
   * manpath tables.  We just clear the tables, and rebuild, based 
   * on the current state of the path and manpath variables.
   * Of course it's possible that path and manpath aren't set.
   */
    short	vindex;
    VarHandle	vh;
    
    if (findVar("path",&vindex)) {
    	vh = gVarList[vindex];
		clearPath(gPathList,&gPathCount,1);
		setPath(gPathList,&gPathCount,vh,1);
	} 
	
	if (findVar("manpath",&vindex)) {
   		vh = gVarList[vindex];
		clearPath(gManpathList,&gManpathCount,0);
		setPath(gManpathList,&gManpathCount,vh,0);
	}  
	return(0);
}	
	
short callExtCmd(argc, argv, stdiop)
 int argc;
 char **argv;
 StdioPtr stdiop;
{
 /* Call this after we've determined that argv[0] is not in the
  * set of built-in commands.
  * - If path is set, search for the named file in it.
  * - If path is not set, look for the file in the current directory.
  * - If we find a likely file, check that the creator is MacShell.  
  *   The file type may be 'CODE', or 'TEXT'.  In the latter case the 
  *   first two characters must be '#!'.  We don't strictly need to do this if
  *   the file was in the path because it should already have been checked, but
  *   maybe we should check anyway, just in case it was changed since the 
  *   path was set.
  *
  * - If it's type 'CODE':
  *   - run callCodeRes
  * - If it's type 'TEXT':
  *    - send it to doRun.
  *
  *
  */	
  	short result = 0, isCode = 0, isScript = 0, slen;
  	long	fType;
  	char	*path, *temp;
 	OSErr	err;
	FInfo	finfo;
	char	*nomc = "Script must begin with '#!': ";
	char	*wrongtype = "Wrong file type: ";
	char	*wrongcreator = "Wrong file creator: ";
 	
  	/* path variable is set and the command doesn't seem to contain a path. */
	if ((countColons(argv[0])==0) && (gPathCount >0)) { 
		if (!(lookupInPath(argv[0],gPathList,gPathCount,&path,1))) {
			/* no match */
			showCmdNotFound(argv[0]);
			return(1);
		}
	} else { /* just use the path given */
		slen=strlen(argv[0]);
		if(!(path = NewPtr(slen+1))) {
			showDiagnostic(gMEMERR);
			return(1);
		}
		BlockMove(argv[0],path,slen+1);
	}

	/* assert: we have a valid path pointer allocated here */
	
	CtoPstr(path);
	err = GetFInfo((unsigned char *)path, 0, &finfo);
	myPtoCstr(path);
	
	if (!err) {
		if (finfo.fdCreator == docCreator) {
			if (finfo.fdType == textFileType) {
				if (hasMagicCookie2(path)) {
					temp = argv[0];
					argv[0] = path;
					path = temp;
					result = doRun(argc,argv,stdiop); 
				} else {
					showDiagnostic(nomc);
					showDiagnostic(path);
					showDiagnostic(RETURNSTR);
					result = 1;
				}
			} else if (finfo.fdType == codeFileType) {
				temp = argv[0];
				argv[0] = path;
				path = temp;
				result = callCodeRes(argc,argv,stdiop);
			} else {
				showDiagnostic(wrongtype);
				showDiagnostic(path);
				showDiagnostic(RETURNSTR);
				result = 1;
			}
		} else {
			showDiagnostic(wrongcreator);
			showDiagnostic(path);
			showDiagnostic(RETURNSTR);
			result = 1;
		}
	} else {
		showCmdNotFound(path);
		result = 1;
	}
	DisposPtr((Ptr)path);
	return(result);
}


short lookupRelative(key,dname)
	char	*dname;
	char	*key;
{ /* Join dname and key.  If this forms a path to a valid 'MacShell-executable' 
   * file, return true.
   */
 char *path;
 FInfo	finfo;
 short result = 0;
 OSErr err;
 
 if (!(joinPath(key,dname,&path))) {
	showDiagnostic(gMEMERR);
	return(0);
 }
 CtoPstr(path);
 err = GetFInfo((unsigned char *)path, 0, &finfo);
 myPtoCstr(path);
 if (err) {
 	DisposPtr((Ptr)path);
 	return(0);
 }	
 if (finfo.fdCreator == docCreator) {
	if (finfo.fdType == textFileType) {
		if (hasMagicCookie2(path))  result = 1;
	} else if (finfo.fdType == codeFileType) result = 1;
 }
 DisposPtr((Ptr)path);
 return(result);
}

short hasMagicCookie2(fname)
	char	*fname;
{ /* this version only uses a file/path name only (it's a 'C' string). */
	HParamBlockRec	hpb;
	OSErr			theErr;
	long			cnt = 2;
	char			buf[3];
	
	CtoPstr(fname);
	hpb.ioParam.ioCompletion = 0L;
	hpb.ioParam.ioNamePtr	= (StringPtr)fname;
	hpb.ioParam.ioVRefNum = 0; 
	hpb.ioParam.ioPermssn = fsRdPerm; 
	hpb.ioParam.ioMisc = NIL; 
	hpb.fileParam.ioDirID = 0L; 
	theErr = PBHOpen(&hpb,FALSE);
	myPtoCstr(fname);
	if (!theErr) {
		theErr = FSRead(hpb.ioParam.ioRefNum,&cnt,buf);
		if (!theErr && (buf[0] == '#') && (buf[1] == '!')) {
			theErr = FSClose(hpb.ioParam.ioRefNum);
			return(1);
		}
		theErr = FSClose(hpb.ioParam.ioRefNum);
	}
	return(0);
}

short joinPath(key,dname,path)
	char *key,*dname,**path;
{ /* Allocate storage and join dname and key adding after dname
   * if necessary.
   * Return false if out of memory.
   */

	short addColon=0,dlen,klen;
	
	dlen = strlen(dname);
	if (dname[dlen-1] != ':') {
		addColon = TRUE;
	}
	klen = strlen(key);
	if (!(*path = NewPtr(dlen + klen + 2))) {
		return(0);
	}
	BlockMove(dname,*path,dlen);
	if (addColon) {
		(*path)[dlen] = ':'; dlen++;
	}
	BlockMove(key,*path + dlen,klen+1);
	return(1);

}


