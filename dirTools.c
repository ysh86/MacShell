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
#include "dirtools.h"
#include "shellutils.h"
#include "filetools.h"

/* This file contains code for cd, ls, pwd, and mkdir*/

doLs(argc, argv, stdiop)
	char		**argv;
	int			argc;
	StdioPtr	stdiop;
{	
/* long list option "-l" should show:
 *    file size
 *    file creator tag
 *    file type tag
 *    if file is locked
 *    file or folder creation date
 *    file or folder mod date
 */
 
	int				i, len, volID, ok, err, ch, optind;
	int				longls;
	OSErr			theErr;
	WDPBRec			pb;
	CInfoPBRec		cpb;
	char 			fname[256], str[256], vname[256];
	char			*nsv = "Current volume is unavailable. \r";
	char			*errmsg = "Ls: Error.\r";
	char			*usage = "Usage: ls [-l] [fname...] [dirname...]\r";
	
	len = 0;
	ok = TRUE;
	
	optind = 0;
	err = FALSE;
	longls = FALSE;
	while (((ch = getopt(argc, argv, "l",(short *)&optind)) != 0) && (!err)) {
		switch (ch) {
			case 'l':
				longls = TRUE;
				break;
			default:
				showDiagnostic(usage);
				err = TRUE;
		}
	}
	if (err)
		return;
	argc -= optind;
	argv += optind;
	
	pb.ioNamePtr=(StringPtr)vname;			
	pb.ioCompletion=0L;
	PBHGetVol(&pb,FALSE); 
	
	theErr = checkVol(&pb);
	if (theErr) {
		showDiagnostic(nsv);
		/* can we still list some other volume? */
	}

	if (argc == 0) {  /* no arguments to ls, just show current directory.*/
		listADir(pb.ioWDDirID, pb.ioWDVRefNum, stdiop, longls); 
	} else { /* there are arguments */
		i = 0;
		while (i < argc) { 
			strcpy(fname,argv[i]);
			volID = getVol(fname); /* different volume? */
			if (volID==0) volID = pb.ioVRefNum; 
			CtoPstr(fname);
			cpb.hFileInfo.ioNamePtr=(unsigned char *) fname; 	
			cpb.hFileInfo.ioVRefNum= 0;
			cpb.dirInfo.ioCompletion= 0L;
			cpb.dirInfo.ioFDirIndex= 0;  /* Zero makes it obey the nameptr */
			cpb.dirInfo.ioDrDirID = 0L;
			theErr = PBGetCatInfo(&cpb,FALSE); 
			if (!theErr) { 
				if (cpb.hFileInfo.ioFlAttrib & 0x10) { /* it's a directory */
					if (argc > 1) {
						sprintf(str,":::::::%s:::::::\r",argv[i]);
						SOInsert(stdiop,str,strlen(str));	
					}
					listADir(cpb.dirInfo.ioDrDirID, volID, stdiop,longls);
					if (argc > 1) { 
						SOInsert(stdiop,RETURNSTR,1);
					}
				} else { /* it's a file, just print it. */
					myPtoCstr(fname); 
					if (!longls) {
						SOInsert(stdiop,fname,strlen(fname));
						SOInsert(stdiop,RETURNSTR,1);
					} else {
						/* long list */
						longLsLine(stdiop,fname,&cpb);
					}
					CtoPstr(fname);
				} 
			} else {
			  	ok = FALSE;
			} 
			i++;
		} /* next arg */
	}
	if (!ok) showDiagnostic(errmsg);
}


listADir(dirID, vRef, stdiop,longflag)
	long		dirID;
	int			vRef;
	StdioPtr	stdiop;
	int			longflag; 
{
	char		fname[256],temp[256];
	OSErr		theErr;
	int			n;
	CInfoPBPtr	cpbp;
	
	cpbp = (CInfoPBPtr)NewPtr(sizeof(CInfoPBRec));
	cpbp->dirInfo.ioNamePtr = (StringPtr)fname;
	cpbp->dirInfo.ioCompletion=0L;
	cpbp->dirInfo.ioFDirIndex=1;  /* set index */
	cpbp->dirInfo.ioDrDirID = dirID;
	cpbp->dirInfo.ioVRefNum= vRef;
	theErr = PBGetCatInfo(cpbp,FALSE); 

	while (!theErr) {
		myPtoCstr(fname);
		n = strlen(fname);
		if (cpbp->dirInfo.ioFlAttrib & 0x10) { 
			/* fname[n++] = 58; */
			temp[0] = 58; 
			BlockMove(fname,temp+1,n+1);
			BlockMove(temp,fname,n+2);
			n++;
		}
		if (!longflag) {
			fname[n++] = RET;
			SOInsert(stdiop,fname,n);
		} else {
			/* long list */
			fname[n] = '\0';
			longLsLine(stdiop,fname,cpbp);
		}							
		cpbp->dirInfo.ioDrDirID = dirID;
		cpbp->dirInfo.ioFDirIndex++;
		theErr=PBGetCatInfo(cpbp,FALSE);
	} 
	
	DisposPtr((Ptr)cpbp);
}


longLsLine(stdiop,fname,cpbp)
	StdioPtr	stdiop;
	char		*fname;
	CInfoPBPtr	cpbp;
/* fname is a c string.  If fname is a directory, it already has
 * colons.  cpbp points to its info block. 
 * construct lines like the following:
 * name size SIGN TYPE crDate modDate
 * name "Folder"       crDate modDate
 */
{
	char	line[256],tempstr[256];
	int		templen;
	int		pos;
	long	tmplong;
	char	*folder = "Folder";
	char	*elipsis = "..";
	DateTimeRec	dtRec;
	int		maxname = 13;
	int		rfsize = maxname + 7;
	int		crcol = rfsize + 7;
	int		typecol= crcol + 6;
	int		cdcol= typecol + 5;
	int		mdcol= cdcol + 18;
	
	pos = strlen(fname);
	BlockMove(fname,line,(long)pos);
	if (pos > maxname) {
		pos = maxname-2;
		BlockMove(elipsis,line+pos,2);
		pos += 2;
	} else {
		while (pos<maxname) {
			line[pos++] = SP;
		}
	}
	
	if (cpbp->hFileInfo.ioFlAttrib & 0x10) { /* it's a directory */
		while (pos <= crcol)
			line[pos++] = SP;
		templen = strlen(folder);
		BlockMove(folder,line+pos,(long)templen);
		pos += templen;
		while (pos < cdcol)
			line[pos++] = SP;
				
		/* creation date */
		Secs2Date(cpbp->dirInfo.ioDrCrDat,&dtRec);
		fmtDateTime(&dtRec,tempstr);
		templen = strlen(tempstr);
		BlockMove(tempstr,line+pos,(long)templen);
		pos += templen;
		while (pos < mdcol) 
			line[pos++] = SP;
		
		/* modification date */
		Secs2Date(cpbp->dirInfo.ioDrMdDat,&dtRec);
		fmtDateTime(&dtRec,tempstr);
		templen = strlen(tempstr);
		BlockMove(tempstr,line+pos,(long)templen);
		pos += templen;
		/* line[pos++] = SP; */

	} else {
		/* File size */
		/* data fork first, then resource fork */
		tmplong = cpbp->hFileInfo.ioFlLgLen;
		NumToString(tmplong,(unsigned char *)tempstr);
		while (pos + tempstr[0] < rfsize) 
			line[pos++] = SP;
		BlockMove(tempstr+1,line+pos,tempstr[0]);
		pos += tempstr[0];
		line[pos++] = SP;

		tmplong = cpbp->hFileInfo.ioFlRLgLen;
		NumToString(tmplong,(unsigned char *)tempstr);
		while (pos + tempstr[0] < crcol) 
			line[pos++] = SP;
		BlockMove(tempstr+1,line+pos,tempstr[0]);
		pos += tempstr[0];
		line[pos++] = SP;

		/* Creator and type */
		tmplong = cpbp->hFileInfo.ioFlFndrInfo.fdCreator;
		if (tmplong != 0L) {
			BlockMove((char *)&tmplong,line+pos,4L);
			pos += 4;
		}
		while (pos < typecol) 
			line[pos++] = SP;
		tmplong = cpbp->hFileInfo.ioFlFndrInfo.fdType;
		if (tmplong != 0L) {
			BlockMove((char *)&tmplong,line+pos,4L);
			pos += 4;
		}
		while (pos < cdcol) 
			line[pos++] = SP;	
				
		/* creation date */
		Secs2Date(cpbp->hFileInfo.ioFlCrDat,&dtRec);
		fmtDateTime(&dtRec,tempstr);
		templen = strlen(tempstr);
		BlockMove(tempstr,line+pos,(long)templen);
		pos += templen;
		while (pos < mdcol) 
			line[pos++] = SP;
		
		/* modification date */
		Secs2Date(cpbp->hFileInfo.ioFlMdDat,&dtRec);
		fmtDateTime(&dtRec,tempstr);
		templen = strlen(tempstr);
		BlockMove(tempstr,line+pos,(long)templen);
		pos += templen;
		/* line[pos++] = SP; */
	}
	
	line[pos++] = RET;
	SOInsert(stdiop,line,pos);
}

fmtDateTime(dtPtr,str)
	DateTimeRec	*dtPtr;
	char		*str;
/* Format date and time and return null terminated string */
{
	int		pos = 0,templen;
	char	temp[256];
	int		timecol = 9;
	int		endcol = 17;
	
	NumToString((long)(dtPtr->month),(unsigned char *)temp);
	if (temp[0] == 1) {
		temp[2] = temp[1]; temp[1] = '0';
		temp[0] = 2;
	}
	BlockMove(temp+1,str,(long)temp[0]);
	pos += temp[0];
	str[pos++] = '/';

	NumToString((long)(dtPtr->day),(unsigned char *)temp);
	if (temp[0] == 1) {
		temp[2] = temp[1]; temp[1] = '0';
		temp[0] = 2;
	}
	BlockMove(temp+1,str+pos,(long)temp[0]);
	pos += temp[0];
	str[pos++] = '/';
	
	NumToString((long)(dtPtr->year),(unsigned char *)temp);
	/*  assume we always get four character years */
	
	BlockMove(temp+3,str+pos,(long)2);
	pos += 2;
	while (pos < timecol)
		str[pos++] = ' ';

	NumToString((long)(dtPtr->hour),(unsigned char *)temp);
	if (temp[0] == 1) {
		temp[2] = temp[1]; temp[1] = '0';
		temp[0] = 2;
	}
	BlockMove(temp+1,str+pos,(long)temp[0]);
	pos += temp[0];
	str[pos++] = ':';

	NumToString((long)(dtPtr->minute),(unsigned char *)temp);
	if (temp[0] == 1) {
		temp[2] = temp[1]; temp[1] = '0';
		temp[0] = 2;
	}
	BlockMove(temp+1,str+pos,(long)temp[0]);
	pos += temp[0];
	str[pos++] = ':';
	
	NumToString((long)(dtPtr->second),(unsigned char *)temp);
	if (temp[0] == 1) {
		temp[2] = temp[1]; temp[1] = '0';
		temp[0] = 2;
	}
	BlockMove(temp+1,str+pos,(long)temp[0]);
	pos += temp[0];
	while (pos < endcol)
		str[pos++] = ' ';

	str[pos] = '\0';
}

OSErr	checkVol(wdpbp)
	WDPBPtr	wdpbp;
{
/* check to see if the current volume is still mounted.  Call right after
 * PBHGetVol. */
 	char			name[256];
	OSErr			theErr;
	CInfoPBRec 		cpb;
		
	cpb.dirInfo.ioCompletion=0L;
	cpb.dirInfo.ioVRefNum=wdpbp->ioWDVRefNum; /* -1 for hard disk */
	cpb.dirInfo.ioDrDirID=wdpbp->ioWDDirID;   /* ioWDDirID is a good directory identifier 
										   *	the dir id of the root is always 2?
										   */
	
	cpb.dirInfo.ioFDirIndex=-1;  /* don't use the name */
	cpb.dirInfo.ioNamePtr = (StringPtr)name;	
	
	theErr = PBGetCatInfo(&cpb,FALSE); 
	return(theErr);
}


doCd(argc, argv, stdiop)
	char		**argv;
	int			argc;
	StdioPtr	stdiop; 
{
	char			*name;
	OSErr			theErr;
	WDPBRec 		pb;
	CInfoPBRec 		cpb;
	char 			*wdout;
	char			*errmsg = "Change Directory Error.\r";
	char			*usage = "Usage: cd dirname \r";
	char			*nsv = "Current volume no longer available. \r";

	if (argc > 2) {
		showDiagnostic(usage);
		return;
	}
	
	name = NewPtr(255);
	strcpy(name,argv[1]);	
		
	wdout=NewPtr(256);
	pb.ioNamePtr=(unsigned char *) wdout;	
	pb.ioCompletion=0L;
	theErr = PBHGetVol(&pb,FALSE); /* get the current volume and directory data */

	theErr = checkVol(&pb);

	if ((!theErr) || (theErr == -35)) {	
		/* -35 is no such volume.  This happens if the current volume
		 * gets unmounted during operation. We need to let the user 
		 * cd to a mounted volume. It would be nice to have argc==1
		 * cause us to go back to some valid volume in this case. */
		if (argc == 1) { /* go to the root of the current volume */
			if (theErr == -35)
				showDiagnostic(nsv);
			pb.ioVRefNum = pb.ioWDVRefNum;
			pb.ioNamePtr=0L;
		} else { /* try to treat it as a relative or absolute path */
			CtoPstr(name);		/* it must be a pascal string */
			pb.ioNamePtr=(StringPtr)name; 	
			pb.ioVRefNum = 0;
		}
	
		pb.ioCompletion=0L;
		pb.ioWDDirID=0L;
		theErr = PBHSetVol(&pb,FALSE); /*set new default dir*/
	} 
	if (theErr) {
		showDiagnostic(errmsg);
	}
	DisposPtr((Ptr)wdout);
	DisposPtr((Ptr)name);
}


getWD(stdiop) /* do pwd */
	StdioPtr	stdiop; 
{
	char		*name;
	int			i, j;
	FileParam	FParamBlk;
	
	/* vars for testing */
	SFTypeList		typeList; 
	short			numTypes;
	SFReply 		reply;
	OSErr			theErr;
	ioParam			openIOParamBlk;
	VolumeParam		volPBlk;
	
	Point			loc;	
	WDPBRec 		pb;
	CInfoPBRec 		cpb;
	char 			tempst[100],*wdtemp,*wdout,*start,*store,*trav;

	/* if (!isHFS()) return(getVname()); /* if not HFS then only volume exists */

	wdout=NewPtr(256);
	wdtemp=NewPtr(256);
	
	pb.ioNamePtr=(unsigned char *) wdout;		/* BYU LSC */
	PBHGetVol(&pb,FALSE); /* get the volume name */
	
	cpb.dirInfo.ioCompletion=0L;
	cpb.dirInfo.ioVRefNum=pb.ioWDVRefNum;
	cpb.dirInfo.ioDrDirID=pb.ioWDDirID;
	trav=wdtemp; /* points to the beginning of the path string */
	
	while(cpb.dirInfo.ioDrDirID!=0) {
		cpb.hFileInfo.ioNamePtr=(unsigned char *) trav;		
		cpb.hFileInfo.ioFDirIndex=-1;						
		if (PBGetCatInfo(&cpb,FALSE)!=0)  {cpb.dirInfo.ioDrDirID=0; break;}
		if (*trav==0) {cpb.dirInfo.ioDrDirID=0;break;}
		i=*trav; 				/* i = length of dirname(since it's a pstring ) */
		*trav=0; start=trav+1; 	/* start points to the beginning of the cstring */
		trav+=i+1; *trav=0; 	/* trav points beyond the end of the string-gets null terminator */
		cpb.dirInfo.ioDrDirID=cpb.dirInfo.ioDrParID;
	}

	*wdtemp=0; 				/* null string--points to the beginning of trav */
	store=wdout+*wdout+1; 					/* store points just beyond the volume name */
	if (trav-wdtemp) *wdout=(trav-wdtemp); 	/* the length of the path string */
	if (trav!=wdtemp) start=trav-1;  	/* start points to the end of the path string */
	else start=wdtemp; 					/* 		or to the beginning */

	if (start!=wdtemp) {
		while(*start) start--;		/* Go to beginning of the last element of the path string */
		if (start!=wdtemp) start--;
	}
	

	 while(start!=wdtemp) {
		while(*start) start--;		/* start points to beginning of a section of the string */
		trav=start+1; *store=':';	/* Ready to move directory name */
		store++;
		while (*trav) {*store=*trav; store++;trav++; } /* store it */
		if (start!=wdtemp) start --; /* Move start back one */
		}
		
	*store=13; /* make sure it has a return at the end */
	myPtoCstr(wdout);			  
	
	SOInsert(stdiop, wdout, strlen(wdout)); 
	DisposPtr((Ptr)wdtemp);
	DisposPtr((Ptr)wdout);
}

int		isDir(fname,wddirid,vrefnum)
	char	*fname; /* c string */
	long	wddirid;
	int		vrefnum;
{
/* if the name "fname" in directory pointed to by "wddirid" in volume "vrefnum":
 * 	-is a directory		return 1
 *	-is a file			return 0
 *	-error				return -1
 * 
 * Note that the caller may pass 0L, and 0 in the last two arguments.  
 */
	CInfoPBRec		cpb;
 	OSErr			theErr;
 	
 	if (fname[0] == 0)
 		return(-1);
 		
 	CtoPstr(fname);
 	cpb.hFileInfo.ioNamePtr=(unsigned char *) fname; 	
	cpb.hFileInfo.ioVRefNum= vrefnum;
	cpb.hFileInfo.ioCompletion=0L;
	cpb.hFileInfo.ioFDirIndex=0;  /* don't use index */
	cpb.hFileInfo.ioDirID = wddirid;
	theErr = PBGetCatInfo(&cpb,FALSE);
	myPtoCstr(fname);
	if (theErr)
		return(-1);
	else if (cpb.hFileInfo.ioFlAttrib & 0x10) 
		return(1);
	else
		return(0);
 
}

doMkdir(argc,argv,stdiop)
	int			argc;
	char		**argv;
	StdioPtr	stdiop;
{
	char			*usage = "Usage: mkdir dirname \r";
	char			*dname;
	OSErr			theErr;
	HParamBlockRec	hpb;
	
	if (argc != 2) {
		showDiagnostic(usage);
		return;
	}
	
	dname = NewPtr(255);
	strcpy(dname,argv[1]);
	
	CtoPstr(dname);
	hpb.fileParam.ioCompletion = 0L;
	hpb.fileParam.ioNamePtr	= (StringPtr)dname;
	hpb.fileParam.ioVRefNum = 0;
	hpb.fileParam.ioDirID = 0L;
	theErr = PBDirCreate(&hpb,FALSE);
	if (theErr) {
		showFileDiagnostic(dname,theErr);
	}
	DisposPtr((Ptr)dname);
	/* flush volume ? */
}


