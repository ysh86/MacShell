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
#include "copy.h"
#include "filetools.h"
#include "shellutils.h"
/* This file contains the code for 'cp', and related file system 
 * utilities */

doCp(argc,argv,stdiop)
	int			argc;
	char		**argv;
	StdioPtr	stdiop;
{
	OSErr		theErr;
	int			i, toVolID, fromVolID, cwv, todir, fromdir;
	int			dorecursive, cperr;
	long 		toDirID, fromDirID, cwd;
	char		*newname,*temp,*toname,*fromname;
	char		*usage = "Usage: cp f1 f2; or: cp f1 ... fn d2 \r";
	char		*myname = (char *)"\pdoCp";
	char		*accerr = "Cp: Error accessing file. \r";
	char		*norflag = "Cp: Directory specified, but no -r given. \r";
	char		*flusherr = "Cp: Error flushing volume. \r";
	
	dorecursive = FALSE;
	i = 1;
	if (strcmp(argv[i],"-r") == 0) {
		dorecursive = 1;			
		i = 2;
	}
	if (((argc == 3) && (dorecursive)) || (argc < 3)) {
		showDiagnostic(usage);
		return;
	}
	
	temp = NewPtr(255);
	toname = NewPtr(255);
	newname = NewPtr(255);
	fromname = NewPtr(255);
	
	cwv = defaultVol();
	defaultDir(&cwd);
	
	toVolID = getVol(argv[argc-1]); /* only use this to flush the volume */
	if (!toVolID) toVolID = cwv;
	
	strcpy(toname,argv[argc-1]);
	if (todir = validDir(toname,&toDirID)) { 
		/* We have an existing directory to copy to. 
		 * This may be a volume name followed by a colon. */
		if (countColons(toname) == 0) 
			addColonFront(toname);
		addColonEnd(toname);
	} /* otherwise it must be a nonexistent directory, or any file, existent or not. */
						
	while (i < (argc-1)) {
		fromVolID = getVol(argv[i]);
		if (!fromVolID) fromVolID = cwv;
		strcpy(fromname,argv[i]);
		if (fromdir = validDir(fromname,&fromDirID)) {
			if (countColons(fromname) == 0) 
				addColonFront(fromname);
			addColonEnd(fromname);
		}
		if (!fromdir)  { /* copy from a file */
			if (todir)  {	/* copy to a directory */
				strcpy(newname,toname);
				getFName(fromname,temp);
				strcat(newname,temp);
				cperr = copyAFile(fromname,newname);
			} else { 		/* copy to a file */
				cperr = copyAFile(fromname, toname);
			}
		} else if (dorecursive) { /* copy from a directory */
			strcpy(newname,toname);
			if (todir) { /* to an existing directory or volume */
				getLastDirName(fromname,temp,fromVolID);
				strcat(newname,temp);
			} else if (countColons(newname) == 0) {
				addColonFront(newname); 
			}
			cperr = copyRecursive(fromname,newname,fromVolID,0);				
		} else { /* no -r given */
			showDiagnostic(norflag);
		}
		i++;
	}
	theErr = FlushVol(0L,toVolID);
	if (theErr) {
		showFileDiagnostic(myname,theErr);
	}	
	DisposPtr((Ptr)newname);
	DisposPtr((Ptr)temp);
	DisposPtr((Ptr)toname);
	DisposPtr((Ptr)fromname);
}


int	absolutePath(path)	
	char	*path;
{	/* find out if "path" starts with a valid volume name by brute force */
	/* This is NOT used by cp.  Questionable if it sould be used */
	char			*temp,*vname;
	HParamBlockRec	hpb;
	int				i,found;
	OSErr			theErr;
	
	temp = NewPtr(255);
	vname = NewPtr(255);
	strcpy(temp,path);
	i = 0;
	while ((temp[i] != COLON) && (temp[i] != 0)) i++;
	temp[i] = 0;
	
	hpb.volumeParam.ioCompletion = 0L;
	hpb.volumeParam.ioNamePtr = (StringPtr)vname;
	hpb.volumeParam.ioVRefNum = 0;
	hpb.volumeParam.ioVolIndex = 1;

	theErr = PBGetVInfo((ParmBlkPtr)&hpb, FALSE);
	found = FALSE;
	while ((!theErr) && (!found)) {
		myPtoCstr(vname);
		if (strcmp(vname,temp) == 0) {
			found = 1;
		} else {
			CtoPstr(vname);
			hpb.volumeParam.ioVRefNum = 0;
			hpb.volumeParam.ioVolIndex++;
			theErr = PBGetVInfo((ParmBlkPtr)&hpb, FALSE);
		}
	}
	
	DisposPtr((Ptr)temp);
	DisposPtr((Ptr)vname);
	
	return(found);	
}



getLastDirName(path,dir,volID)
	char	*path,*dir;
	int		volID;
{ /* Get the last directory name in path using filesystem calls. */
	OSErr		theErr;
	long		dirID;
	char		*temp;
	char		*myname = (char *)"\pgetLastDirName";
	char		*direrr = "getLastDirName: Can't access directory. \r";
	CInfoPBRec	cpb;
	int		i;

	temp = NewPtr(255);
	strcpy(temp,path);
	CtoPstr(temp);
	cpb.hFileInfo.ioDirID=0L; /* current directory ID */
	cpb.hFileInfo.ioNamePtr=(unsigned char *) temp; 	
	cpb.hFileInfo.ioCompletion=0L; 	 
	cpb.hFileInfo.ioVRefNum= 0;
	cpb.hFileInfo.ioFDirIndex = 0; /* make it pay attention to the name */
	theErr=PBGetCatInfo(&cpb,FALSE); /* get the dirID */
	if (theErr) {
		showFileDiagnostic(myname,theErr);
	}
	dirID = cpb.hFileInfo.ioDirID;
	cpb.hFileInfo.ioFDirIndex = -1;
	cpb.hFileInfo.ioVRefNum = volID;
	theErr=PBGetCatInfo(&cpb,FALSE);

	myPtoCstr(temp);
	strcpy(dir,temp);
	
	DisposPtr((Ptr)temp);

}

getFName(path,file)
	char	*path,*file;
{	/* extract the file name (if any) from a path name */
	int		i;
	i = strlen(path);
	while ((path[i] != COLON) && (i != 0)) i--;
	if (path[i] == COLON) i++;
	strcpy(file,&path[i]);
}


int copyRecursive(fromdir,todir,fromvol,depth)
	char	*fromdir,*todir;
	int		fromvol,depth;
{ 	
	OSErr		theErr;
	int			i,volID,cperr,err,newdepth;
	long		dirID;
	char		*tempfrom, *fromname, *toname, *temp;
	char		*createerr = "copyRecursive: Error creating directory: ";
	char		*direrr = "copyRecursive: Can't access directory to copy from. \r";
	CInfoPBRec	cpb;
	
	newdepth = depth + 1;
	if (newdepth == 30) {
		temp = NewPtr(255);
		sprintf(temp,"Bailing out at copyRecursive depth: %ld  \r", (long)newdepth);
		showDiagnostic(temp);
		DisposPtr((Ptr)temp);
		return(1);
	}	
	
	/* create todir */
	theErr = createADir(todir);
	if (theErr) {
		CtoPstr(todir);
		showFileDiagnostic(todir,theErr);
		myPtoCstr(todir);
		return(theErr);
	}
	
	cperr = FALSE;
	fromname = NewPtr(strlen(fromdir) + 255);
	toname = NewPtr(strlen(todir) + 255);
	tempfrom = NewPtr(255);
	strcpy(tempfrom,fromdir);
	CtoPstr(tempfrom);

	
	/* setup to index through fromdir with cat info until we hit an error.*/
	cpb.hFileInfo.ioDirID=0L; /* current directory ID */
	cpb.hFileInfo.ioNamePtr=(unsigned char *) tempfrom; 	
	cpb.hFileInfo.ioCompletion=0L; 	 
	cpb.hFileInfo.ioVRefNum= 0;
	cpb.hFileInfo.ioFDirIndex = 0; /* make it pay attention to the name */
	theErr=PBGetCatInfo(&cpb,FALSE); /* get the dirID */
	if (theErr) {
		cperr = 1;
		showFileDiagnostic(tempfrom,theErr);
		goto cleanup;
	}
	dirID = cpb.hFileInfo.ioDirID;
	cpb.hFileInfo.ioVRefNum = fromvol; 
	cpb.hFileInfo.ioFDirIndex = 1;
	theErr=PBGetCatInfo(&cpb,FALSE);
	while (!theErr) {	
		strcpy(fromname,fromdir);
		strcpy(toname,todir);
		addColonEnd(fromname);
		addColonEnd(toname);
		myPtoCstr(tempfrom);
		strcat(toname,tempfrom);
		strcat(fromname,tempfrom);
		CtoPstr(tempfrom);
		if (cpb.hFileInfo.ioFlAttrib & 0x10) {
			err = copyRecursive(fromname,toname,fromvol,newdepth);
			if (err) cperr = 1;
		} else {
			err = copyAFile(fromname,toname);
			if (err) cperr = 1; 
		}
		cpb.hFileInfo.ioFDirIndex++;
		cpb.hFileInfo.ioDirID=dirID; 
		theErr=PBGetCatInfo(&cpb,FALSE);
	}
	cleanup:
	if (cperr) {
		deleteDir(todir);
		/* remove the directory we created if it's empty. */
	}
	DisposPtr((Ptr)tempfrom);
	DisposPtr((Ptr)fromname);
	DisposPtr((Ptr)toname);
	return(cperr);
}

deleteDir(name)
	char	*name;
{
	HParamBlockRec	hpb;
	char			*temp;
	OSErr			theErr;
	
	temp = NewPtr(255);
	strcpy(temp,name);
	CtoPstr(temp);
	hpb.fileParam.ioCompletion = 0L;
	hpb.fileParam.ioNamePtr = (StringPtr)temp;
	hpb.fileParam.ioVRefNum = 0;
	hpb.fileParam.ioDirID = 0L;
	theErr = PBHDelete(&hpb,FALSE);
	DisposPtr((Ptr)temp);
	
}


OSErr createADir(name)
	char	*name;
{ /* create a directory */
	HParamBlockRec	hpb;
	OSErr			theErr;
	char			*temp;
	
	temp = NewPtr(255);
	strcpy(temp,name);
	CtoPstr(temp);
	
	hpb.ioParam.ioCompletion = 0L;
	hpb.ioParam.ioNamePtr = (StringPtr)temp;
	hpb.ioParam.ioVRefNum = 0;
	hpb.fileParam.ioDirID = 0L;
	
	theErr = PBDirCreate(&hpb,FALSE);
	DisposPtr((Ptr)temp);
	return(theErr);
}




int copyAFile(from,to) 
	char	*from,*to;
{	/* "from" must exist and be readable, etc.  "To" must not exist */
	char	*fname,*tname,*temp;
	OSErr	theErr,closeErr1,closeErr2,deleteErr;
	int		fref,tref,open1,open2;
	FInfo	fiRec,*fiPtr;
	long	cdate,mdate;
	char	*closeerr = "copyAFile: Error closing data fork. \r";
	char	*closerferr = "copyAFile: Error closing resource fork. \r";
	char	*deleteerr = "copyAFile: Error removing file. \r";
	char	*fncopied = "File not copied: ";
	
	temp = NewPtr(255);
	fiPtr = &fiRec;
	fname = NewPtr(255);
	strcpy(fname,from);
	tname = NewPtr(255);
	strcpy(tname,to);
	CtoPstr(tname);
	CtoPstr(fname);
	
	/* get info of file to copy from */
	theErr = getInfo(fname,fiPtr,&cdate,&mdate);
	if (theErr) {
		showFileDiagnostic(fname,theErr);
		goto cleanup;
	}		
	
	/* create file to copy to */
	theErr = createFile(tname,fiPtr,cdate,mdate); 
	if (theErr) {
		showFileDiagnostic(tname,theErr);
		goto cleanup;
	} 
	
	/* open data forks */
	open1 = open2 = FALSE;
	theErr = openDF(fname,&fref,TRUE);
	if (theErr) {
		showFileDiagnostic(fname,theErr);
	} else { open1 = TRUE; }
	if (!theErr) { 
		theErr = openDF(tname,&tref,FALSE);
		if (theErr) {
			showFileDiagnostic(tname,theErr);
		} else { open2 = TRUE; }
	}

	/* copy data forks */
	if (!theErr) {
		theErr = copyFork(fref,tref);
		if (theErr) {
			showFileDiagnostic(fname,theErr);
		} 
	}
	
	/* close data forks */
	closeErr1 = closeErr2 = FALSE;
	if (open1) closeErr1 = FSClose(fref);
	if (open2) closeErr2 = FSClose(tref);
	if (closeErr1 || closeErr2) {
		showDiagnostic(closeerr);
	} 
	if (closeErr1 || closeErr2 || theErr)
		goto delete;
	
	
	/* open resource forks */
	open1 = open2 = FALSE;
	theErr = openRF(fname,&fref,TRUE);
	if (theErr) {
		showFileDiagnostic(fname,theErr);
	} else {
		open1 = TRUE;
		theErr = openRF(tname,&tref,FALSE);
		if (theErr) {
			showFileDiagnostic(tname,theErr);
		}  else { open2 = TRUE; }
	}
	
	/* copy resource forks */
	if (!theErr) {
		theErr = copyFork(fref,tref);
		if (theErr) {
			showFileDiagnostic(fname,theErr);
		}
	}	
	
	/* close resource forks */
	closeErr1 = closeErr2 = FALSE;
	if (open1) closeErr1 = FSClose(fref);
	if (open2) closeErr2 = FSClose(tref);
	if (closeErr1 || closeErr2) {
		showDiagnostic(closerferr);
	}
	
	
	delete:
	if (closeErr1 || closeErr2 || theErr) {
		myPtoCstr(tname);
		strcpy(temp,fncopied);
		strcat(temp,tname);
		strcat(temp,RETURNSTR);
		CtoPstr(tname);
		showDiagnostic(temp);
		
		deleteErr = deleteFile(tname);
		if (deleteErr) {
			showDiagnostic(deleteerr);
		}
	}

	cleanup:
	DisposPtr((Ptr)temp);
	DisposPtr((Ptr)fname);
	DisposPtr((Ptr)tname);
	if (closeErr1 || closeErr2 || theErr) {
		return(1);
	} else {
		return(0);
	}
}



int		copyFork(from,to)
	int		from,to;
{
	char			*buffer;
	int				err;
	ParamBlockRec	rpb,wpb;
	OSErr			readErr,writeErr;
	char			*temp1, *temp2;
	char			*myname = (char *)"\pcopyFork";
	char			*readerr = "copyFork: Error reading file. \r";
	char			*writerr = "copyFork: Error writing to file. \r";
	char			*diskfullerr = "copyFork: Disk Full Error. \r";
	
	temp1 = NewPtr(255);
	temp2 = NewPtr(255);
	buffer = NewPtr(4096);
	
	rpb.ioParam.ioCompletion = 0L;
	rpb.ioParam.ioNamePtr = (StringPtr)temp1;
	rpb.ioParam.ioRefNum = from;
	rpb.ioParam.ioBuffer = buffer;
	rpb.ioParam.ioReqCount = 4096L;
	rpb.ioParam.ioPosMode = 0;
	rpb.ioParam.ioPosOffset = 0L;
	
	wpb.ioParam.ioCompletion = 0L;
	wpb.ioParam.ioNamePtr = (StringPtr)temp2;
	wpb.ioParam.ioRefNum = to;
	wpb.ioParam.ioBuffer = buffer;
	wpb.ioParam.ioPosMode = 0;
	wpb.ioParam.ioPosOffset = 0L;
	
	
	readErr = writeErr = FALSE;
	while ((!readErr) && (!writeErr)) {
		readErr = PBRead(&rpb,FALSE);
		if ((!readErr) || (readErr == -39)) { /* -39 is EOF */
			wpb.ioParam.ioReqCount = rpb.ioParam.ioActCount;
			writeErr = PBWrite(&wpb,FALSE);
			if (writeErr) {
				/* showFileDiagnostic(myname,writeErr); */
			}
		} else {
			showFileDiagnostic(myname,readErr);
		}
	}
	DisposPtr((Ptr)buffer);
	DisposPtr((Ptr)temp1);
	DisposPtr((Ptr)temp2);
	if (writeErr)
		return(writeErr);
	else if (readErr != -39) 
		return(readErr);
	else
		return(0);
}

OSErr	openDF(name,frefp,read) /* name must be a pascal string */
	char	*name;
	int		*frefp;
	int		read; /* true == read-only */
{
	HParamBlockRec	hpb;
	OSErr			theErr;
	
	hpb.ioParam.ioCompletion = 0L;
	hpb.ioParam.ioNamePtr	= (StringPtr)name;
	hpb.ioParam.ioVRefNum = 0; 
	if (read) 	hpb.ioParam.ioPermssn = fsRdPerm; 
	else 		hpb.ioParam.ioPermssn = fsWrPerm;
	hpb.ioParam.ioMisc = NIL; 
	hpb.fileParam.ioDirID = 0L; 
	theErr = PBHOpen(&hpb,FALSE);
	if (!theErr) {
		*frefp = hpb.ioParam.ioRefNum;
	}
	return(theErr);
}


OSErr	openRF(name,frefp,read)
	char	*name;
	int		*frefp;
	int		read; /* true == read-only */
{
	HParamBlockRec	hpb;
	OSErr			theErr;
	
	hpb.ioParam.ioCompletion = 0L;
	hpb.ioParam.ioNamePtr	= (StringPtr)name;
	hpb.ioParam.ioVRefNum = 0; 
	if (read) 	hpb.ioParam.ioPermssn = fsRdPerm; 
	else 		hpb.ioParam.ioPermssn = fsWrPerm;
	hpb.ioParam.ioMisc = NIL; 
	hpb.fileParam.ioDirID = 0L; 
	theErr = PBHOpenRF(&hpb,FALSE);
	if (!theErr) {
		*frefp = hpb.ioParam.ioRefNum;
	}
	return(theErr);
}


OSErr	createFile(name,fiPtr,cdate,mdate)
	char	*name;
	FInfo	*fiPtr;
	long	cdate,mdate;
{
	HParamBlockRec	hpb;
	OSErr			theErr;
	
	hpb.fileParam.ioCompletion = 0L;
	hpb.fileParam.ioNamePtr	= (StringPtr)name;
	hpb.fileParam.ioVRefNum = 0; /* volID; */
	hpb.fileParam.ioDirID = 0L; /* dirID; */
	theErr = PBHCreate(&hpb,FALSE);
	if (theErr) {
		return(theErr);
	}
	hpb.fileParam.ioFlFndrInfo = *fiPtr;
	hpb.fileParam.ioFlCrDat = cdate;
	hpb.fileParam.ioFlMdDat = mdate;
	theErr = PBHSetFInfo(&hpb,FALSE);
	return(theErr);
}

OSErr	deleteFile(name)
	char	*name;
{
	HParamBlockRec	hpb;
	OSErr			theErr;
	
	hpb.fileParam.ioCompletion = 0L;
	hpb.fileParam.ioNamePtr	= (StringPtr)name;
	hpb.fileParam.ioVRefNum = 0; /* volID; */
	hpb.fileParam.ioDirID = 0L; /* dirID; */
	theErr = PBHDelete(&hpb,FALSE);
	return(theErr);
}



OSErr	getInfo(fname,finfop,cdatep,mdatep)
	char	*fname;
	long	*cdatep,*mdatep;
	FInfo	*finfop;
{
	HParamBlockRec	hpb;
	char			*temp;
	OSErr			theErr;
	
	hpb.fileParam.ioCompletion = 0L;
	hpb.fileParam.ioNamePtr = (StringPtr)fname;
	hpb.fileParam.ioVRefNum = 0; /* volRef */
	hpb.fileParam.ioFDirIndex = 0;
	hpb.fileParam.ioDirID = 0L; /* dirID; */
	theErr = PBHGetFInfo(&hpb,FALSE);
	if (!theErr) {
		BlockMove(&hpb.fileParam.ioFlFndrInfo,finfop,(Size)sizeof(FInfo));
		*cdatep = hpb.fileParam.ioFlCrDat;
		*mdatep = hpb.fileParam.ioFlMdDat;
	}
	return(theErr);
}

