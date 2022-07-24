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
#include "fileTools.h"
#include "dirtools.h"
#include "copy.h"
#include "shellutils.h"
/* This file contains cat,find,grep,mv,rm, and assorted file 
 * system utility functions*/

doCat(argc, argv, stdiop)
	int			argc;
	char		**argv;
	StdioPtr	stdiop;
{
/* To do:
 * 1. "cat file1 -r file2 file3" should
 * output the data fork of file1, the resource fork of file2 and the
 * data fork of file3 in succession.
 *
 * 2. "cat file1 -" should output file1 followed by standard input.
 */
	char				fname[256], *buffer;
 	HParamBlockRec		hpb;
	OSErr				theErr;
	int					currarg, err, bcount, readres;
	
	char		*memerr = "Cat: Low memory error.\r";
	char		*direrr = "Cat: Can't cat a directory. \r";
	char		*usage = "Usage: cat [file1] [-r file2] ... \r";
	
	if (!(buffer = NewPtr(BUFSIZE))) {
		showDiagnostic(memerr);
		return;
	} 	
	
	if (argc == 1) {
		/* read from stdin */
		err = catFromStdin(stdiop,buffer,BUFSIZE);
	} else { 
		currarg = 1;
		while (currarg < argc) {
			/* the argument could be: filename, "-r", or "-" */
			readres = FALSE;
			if (argv[currarg][0] == '-') {
				if (argv[currarg][1] == 0) {
					/* read standard input here */
					err = catFromStdin(stdiop,buffer,BUFSIZE);
					currarg++;
					continue;
				} else if ((argv[currarg][1] == 'r') && (argv[currarg][2] == 0)) {
					/* read resource fork of next arg */
					if (currarg + 1 < argc) {
						readres = TRUE;
						currarg++;
					} else {
						showDiagnostic(usage);
						DisposPtr((Ptr)buffer);
						return(1);
					}
				} 
			}
			
			strcpy(fname, argv[currarg]);
			CtoPstr(fname);
	
			hpb.ioParam.ioCompletion=0L;
			hpb.ioParam.ioNamePtr=(StringPtr)fname; 	/* Pascal string!! */
			hpb.ioParam.ioVRefNum= 0; 
			hpb.ioParam.ioPermssn=fsRdPerm; 			/* read only = 1 */
			hpb.ioParam.ioMisc = 0L; 					/* use volume buffer */
			hpb.fileParam.ioDirID = 0L; 			/* current directory */
			
			if (!readres) 	theErr = PBHOpen(&hpb, FALSE);
			else 			theErr = PBHOpenRF(&hpb, FALSE);
			
			if (theErr) { 
				if (isDir(argv[currarg],0L,0) == 1) {
					showDiagnostic(direrr);
				} else {
					showFileDiagnostic(fname,theErr);
				}
			} else {
				hpb.ioParam.ioReqCount = (long)BUFSIZE;
				hpb.ioParam.ioBuffer = buffer;
				hpb.ioParam.ioPosMode = 0;
				hpb.ioParam.ioPosOffset = 0L; 

				while (!theErr) {			
					theErr = PBRead((ParmBlkPtr)&hpb, FALSE); 
					if (theErr == -39) { /* eof */ 
					} else if (theErr) {
						showFileDiagnostic(fname,theErr);
						break;
					}
					SOInsert(stdiop,buffer,LoWord(hpb.ioParam.ioActCount));
				}
				
				theErr = PBClose((ParmBlkPtr)&hpb, FALSE);
				if (theErr) showFileDiagnostic(fname,theErr); 
			}
    		currarg++;
 	 	}
 	}	
	DisposPtr((Ptr)buffer);
}

short catFromStdin(stdiop,buffer,bsize)
	StdioPtr	stdiop;
	char		*buffer;
	int			bsize;
{
	int	err = FALSE;
	int done = FALSE;
	int	bcount;
	
	while ((!err) && (!done)) {
		err = SIGet(stdiop,buffer,bsize,(short *)&bcount);
		if (bcount != BUFSIZE) {
			done = TRUE;
		}
		if (!err) {
			SOInsert(stdiop,buffer,bcount);
		}
	}
	return(err);	
}

short doFind(argc, argv, stdiop)
int			argc;
char		**argv;
StdioPtr	stdiop;
{
/* 
 * Additional features to support soon:
 *  -print (don't show path by default as we do now)
 *
 * Someday:
 *	-size
 *  -date & time (creation, modification)
 *  -forks (has resource, has data)
 *  -entity type  (folder or file...)
 *	...
 */
 	WDPBRec		wdpb;
 	OSErr		theErr;
 	char		vname[256];
 	int			vref, i, noterr, len; 
 	long		wdref;
 	FindFlagsRec	fflags;
 	
  	char		*volerr = "Find: Error getting volume info. \r";
 	char		*direrr = "Find: Error accessing root directory for search. \r";
	char		*usage = "Usage: find dirname -name regexp [-exec ...] \r";
	char		*badftype = "Find: File type tag not found. \r";
	char		*badtype = "Find: Bad type flag. \r";
	char		*badcrtr = "Find: Creator tag not found. \r";
	char		*badexec = "Find: Exec string unterminated. \r";
	
 	fflags.printF = FALSE; 	/* true if -print is found */
 	fflags.nameF = FALSE; 	/* true if -name is found */
 	fflags.execF = FALSE; 	/* true if valid -exec is found */
 	fflags.ftypeF = FALSE; 	/* true if -ftype is found */
 	fflags.fcreatorF = FALSE;	/* true if -fcreator is found */
 	fflags.typeF = FALSE;
	fflags.eargc = 0;
	
	if (argc < 2) {
 		showDiagnostic(usage);
 		return;
 	}
 	strcpy(fflags.rootname,argv[1]); 
 	len = strlen(fflags.rootname);
 	if ((fflags.rootname)[len-1] != ':') {
 		(fflags.rootname)[len] = ':';
 		(fflags.rootname)[len+1] = 0;
 	}
	i = 2;
	while (i < argc) {
		if (strcmp("-name",argv[i])==0){
			strcpy(fflags.name,argv[i+1]);
			fflags.nameF = TRUE; i++;
		} else if (strcmp("-exec",argv[i])==0)	{
			/* get everything from here to ; or \; in eargv. */
			i++;  fflags.eargv=argv + i;
			while ((i < argc) && (strcmp("\;",argv[i])) &&
					(strcmp("\\;",argv[i]))) {i++; (fflags.eargc)++;}
			if (i != argc) {
				fflags.execF = TRUE;
			} else {
				showDiagnostic(badexec);
				return(TRUE);
			}
		} else if (strcmp("-print",argv[i])==0) {
			fflags.printF = TRUE;
		} else if (strcmp("-type",argv[i])==0) {
			if ((*(argv[i+1])=='f') || (*(argv[i+1])=='d')) {
				fflags.type = *(argv[i+1]);
				fflags.typeF = TRUE; i++;
			} else { showDiagnostic(badtype); return(TRUE); }
		} else if (strcmp("-ftype",argv[i])==0) {
			if (i+1 < argc) { BlockMove(argv[i+1],(char *)&(fflags.ftype),4L); }
			else { showDiagnostic(badftype); return(TRUE);}
			fflags.ftypeF = TRUE; i++;
		} else if (strcmp("-fcreator",argv[i])==0) {	
			if (i+1 < argc) { BlockMove(argv[i+1],(char *)&(fflags.fcreator),4L); }
			else { showDiagnostic(badcrtr); return(TRUE);}
			fflags.fcreatorF = TRUE; i++;
		} else {
			showDiagnostic(usage);
			return(TRUE);
		}
		i++;
	}
	
 	
	/* current volume...when would volerr ever happen? */ 	
	wdpb.ioNamePtr=(unsigned char *) vname;	
	wdpb.ioCompletion=0L;
	if (PBHGetVol(&wdpb,FALSE)) { showDiagnostic(volerr); return; }
 	
 	vref = getVol(fflags.rootname); /* on a different volume? */
	if (vref==0) vref = wdpb.ioWDVRefNum; 

	noterr = validDir(fflags.rootname,&wdref); 
	if (!noterr) { showDiagnostic(direrr);  return; }
	
 	/* do recursive find and exec */
 	findRecursive(stdiop,&fflags, wdref, vref);
 	 	
}


findRecursive(stdiop, flagsp, dirID, volID)
	StdioPtr		stdiop;
	FindFlagsPtr	flagsp;
	long			dirID;
	int				volID;
{
/* -Maintain a string showing the path...or at any rate, don't calculate it
 * repeatedly.
 *
 */
	char		fname[256], path[256];
	CInfoPBRec	cpb;
	OSErr		theErr;
	int			len;
	short		isdir;

	cpb.dirInfo.ioNamePtr = (StringPtr)fname;
	cpb.dirInfo.ioCompletion=0L;
	cpb.dirInfo.ioFDirIndex=1;  /* set index */
	cpb.dirInfo.ioDrDirID = dirID;
	cpb.dirInfo.ioVRefNum= volID;
	theErr = PBGetCatInfo(&cpb,FALSE); 

	while (!theErr) {
		if (cpb.dirInfo.ioFlAttrib & 0x10) { /* it's a directory */
			isdir = TRUE;
			len = strlen(flagsp->rootname);
			BlockMove(fname+1,flagsp->rootname + len,*fname);
			(flagsp->rootname)[len + *fname] = ':';
			(flagsp->rootname)[len + *fname + 1] = 0;
 			findRecursive(stdiop,flagsp, cpb.dirInfo.ioDrDirID,volID);
 			(flagsp->rootname)[len] = 0;
		} else { isdir = FALSE; }
		if (flagsp->typeF) {
			if ((flagsp->type == 'f' && isdir) ||
				(flagsp->type == 'd' && !isdir)) { goto next;}		
		}
		if (flagsp->nameF) {
			myPtoCstr(fname);
			if (!checkMatch(fname,flagsp->name,1)) { goto next;}
			CtoPstr(fname);		
		}
		if (flagsp->ftypeF) {
			if ((isdir) || 
			(flagsp->ftype != cpb.hFileInfo.ioFlFndrInfo.fdType)) { goto next;}		
		}
		if (flagsp->fcreatorF) {
			if ((isdir) || 
			(flagsp->fcreator != cpb.hFileInfo.ioFlFndrInfo.fdCreator)) { goto next;}		
		}
		if ((flagsp->printF) || (flagsp->execF)) {
			len = strlen(flagsp->rootname);
			BlockMove(flagsp->rootname,path,len);
			BlockMove(fname+1,path+len,*fname);
			len +=*fname; 
			if (isdir) { path[len++] = ':';}
			path[len] = 0;
		}
		if (flagsp->printF) {
			path[len++] = 13; path[len] = 0;
			SOInsert(stdiop,path,len);
			path[len-1] = 0; /* remove the return character */
		}
		if (flagsp->execF) {
			doExec(stdiop, (int *)&(flagsp->eargc), (char **)flagsp->eargv, (char *)path); 
		}
		
		next: 		/* setup for the next iteration */
		cpb.dirInfo.ioDrDirID = dirID;
		cpb.dirInfo.ioFDirIndex++;
		theErr=PBGetCatInfo(&cpb,FALSE);
	}
}

doExec(stdiop, eargcp, eargv, path)
	StdioPtr	stdiop;
	int			*eargcp;
	char		**eargv, *path;
{ /* if {} exists as an arg, replace it with path name, then callUtility 
   * This replaces multiple occurences of {}, and the placeholder
   * does not need to exist as a separate argument */
	int		i;
	char	**tempargv;
	char	*memerr = "Out of memory.\r";
	
	if (!(tempargv=(char **)NewPtr(((*eargcp)+1)*sizeof(Ptr)))){
			showDiagnostic(memerr);
			return;
	}

	i = 0;
	while (i < *eargcp) {
		if (prepExecArg(&(tempargv[i]),path, eargv[i])) {
			return;
		}
		i++;
	}
	tempargv[i]=NewPtr(1);
	*(tempargv[i])=0;
	
	callUtility(stdiop, &i, tempargv);
	
	disposArgs(&i,tempargv);
	DisposPtr((Ptr)tempargv);
	
}	

short prepExecArg(execargv,path,carg)
	char **execargv, *path,*carg;
/* examine string carg.  Copy to execarg, but insert 
 * the string "path" wherever the placeholder "{}" appears.  The placeholder
 * may appear multiple times. Return 1 if out of memory 
 */
{
	int		i=0,len=0,j;
	char	*memerr = "Out of memory.\r";
	j=strlen(path);
	/* find out how long it will be */
	while (carg[i]!=0) {
		if ((carg[i]=='{') && (carg[i+1]=='}')) {
			len += j;
			i=i+2;
		} else {
			len++;
			i++;
		}
	}
	if (!((*execargv)=NewPtr(len+1))){
		showDiagnostic(memerr);
		return(1);
	}
	i=0; len=0;
	/* transfer stuff into the new argument */
	while (carg[i]!=0) {
		if ((carg[i]=='{') && (carg[i+1]=='}')) {
			BlockMove(path,&((*execargv)[len]),j);
			len += j;
			i=i+2;
		} else {
			(*execargv)[len] = carg[i];
			len++;
			i++;
		}
	}
	(*execargv)[len] = 0;
	return(0);

}


getPath(vref,wdref,path)
	int		vref;
	long	wdref; /* this is really a dirID */
	char	*path;
{
	CInfoPBRec	cpb;
	OSErr		theErr;
	char		dname[256],temp[256];
	char		*errmsg = "getPath: Error getting path. \r";
	int			plen,i,st,j;
	
	cpb.hFileInfo.ioCompletion=0L;
	cpb.hFileInfo.ioVRefNum=vref; 
	cpb.hFileInfo.ioDirID=wdref;
	cpb.hFileInfo.ioFDirIndex=-1;						
	cpb.hFileInfo.ioNamePtr=(StringPtr)dname;		

	plen = 0;	
	theErr = PBGetCatInfo(&cpb,FALSE);
	while(!theErr) {
		if (*dname==0) {break;}
		myPtoCstr(dname);
		*(temp+plen)=58; /*colon*/
		plen++;
		strcpy(temp+plen,dname);	
		plen += strlen(dname);	
		CtoPstr(dname);	
		cpb.hFileInfo.ioDirID=cpb.hFileInfo.ioFlParID;
		theErr = PBGetCatInfo(&cpb,FALSE);
	}
	*(temp+plen)=58; /*colon to mark the end*/
	/* reverse the order of things */
	i = plen-1;
	while ((temp[i] != 58) && (i > 0)) i--;
	st = i+1; j = 0;
	while (i >= 0) {
		while (temp[st] != 58)  path[j++] = temp[st++];
		path[j++] = 58;
		i--;
		while ((temp[i] != 58) && (i > 0)) i--;
		st = i+1;
	}	
	path[j++] = 0;
}

int	getVol(vname)
	char	*vname;
{ /* Cut off everyting after the first colon, and try to get
   * a volume ID for vname.
   * Return 0 if it's not a valid volume name. */
	HParamBlockRec	pb;
	char			temp[256];
	int				i, vref;
	OSErr			theErr;

	if (vname[0] == 58) return(0); /* relative path */
		
	strcpy(temp,vname);
	i = 0;
	while((temp[i] != 0) && (temp[i] != 58)) i++; /* 58=':' */ 
	if (temp[i] == 0) {
		return(0);
	}
	i++;
	temp[i] = 0;
	
	CtoPstr(temp);
	pb.volumeParam.ioCompletion = 0L;
	pb.volumeParam.ioNamePtr = (StringPtr)temp;
	pb.volumeParam.ioVRefNum = 0;
	pb.volumeParam.ioVolIndex = -1;
	theErr = PBHGetVInfo(&pb,FALSE); 
	
	vref = pb.volumeParam.ioVRefNum; 
	if (theErr) {
		return(0);
	} else {
		 return(vref);
	} 
}

int getDir(dname,vref,drefp)  
	char	*dname;
	int		vref;
	long	*drefp;
{ /* Get directory ID for path name "dname."
   * It could be either absolute or relative path.
   * If it's absolute, a valid vref should be passed.
   * Return 0 if it's not a valid name. 
   *
   * Note: validDir shows a cleaner way to do this. The complexity here 
   * with vref is not really necessary.
   */
	char		temp[256];
	CInfoPBRec	cpb;
	int			len,i,colons;
	OSErr		theErr;
	
	strcpy(temp,dname);
	len = strlen(temp);
	while ((temp[len] != 58) && (len != 0)) len--;
	temp[++len] = 0; 
	i = len;
	colons = 0;
	while (i >= 0) { if (temp[i] == 58) colons++; i--;}
	if ((temp[0] == 58) || (colons > 1)){ /* relative path */
		cpb.dirInfo.ioVRefNum = 0;
	} else { /*absolute path*/
		cpb.dirInfo.ioVRefNum = vref;
	}
	CtoPstr(temp);
	cpb.dirInfo.ioCompletion = 0L;
	cpb.dirInfo.ioNamePtr = (StringPtr)temp;
	cpb.dirInfo.ioFDirIndex = 0;
	cpb.dirInfo.ioDrDirID = 0;
	theErr = PBGetCatInfo(&cpb,FALSE);
	if (!theErr) {
		*drefp = cpb.dirInfo.ioDrDirID;
		return(0);
	} else {
		return(1);
	}
}

doGrep(argc, argv, stdiop)
	int				argc;
	char			**argv;
	StdioPtr		stdiop;
{
/* 
 * To do:
 *  more pattern matching: match beginning of line, eol etc.
 */
 	WDPBRec			wdpb;
 	HParamBlockRec	hpb;
 	int				matchlen,currarg,tmp,templen,err,optind,casein,notgrep,ch,done;
 	int				fileopen, ok;
 	int				needchunk, lastchunk, locnt, linebcnt, linestart, bcount, chunkpos;
 	int				linecount, tempcnt;
 	OSErr			theErr;
	char			fname[256], *linebuf, *chunkbuf, *leftover, *mstr, tempname[256];
 	char			*usage = "Usage: grep [-i] [-v] keyword [filename...]\r";
 	char			*direrr = "Grep: Can't grep a directory. \r";
 	char			*fileerr = "Grep: Error getting file info. \r";
	char			*longline = "Grep: Chopping off long line. \r";
	char			*memerr = "Low memory error. \r";
	 	
	if (argc < 2) { 
		showDiagnostic(usage); 
		return;
	}
	
	err = FALSE;
	casein = FALSE;
	notgrep = FALSE;
	optind = 0;
	while (((ch = getopt(argc, argv, "iv",(short *)&optind)) != 0) && (!err)) {
		switch (ch) {
			case 'i':
				casein = TRUE;
				break;
			case 'v':
				notgrep = TRUE;
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
	
	if (argc == 0) {
		showDiagnostic(usage);
		return;
	} else if (argc == 1) { /* prepare to use stdin */
		if (!(chunkbuf = NewPtr(BUFSIZE)) || !(leftover = NewPtr(BUFSIZE))
			|| !(linebuf = NewPtr(BUFSIZE))) {
			showDiagnostic(memerr);
			return;
		}
		needchunk = TRUE;
		lastchunk = FALSE;
		err = FALSE;
		locnt = 0;
	} else { /* prepare to read a file */
		if (!(linebuf = NewPtr(BUFSIZE))) {
			showDiagnostic(memerr);
			return;
		}
		wdpb.ioNamePtr = (StringPtr)tempname; 
		wdpb.ioCompletion=0L;
		PBHGetVol(&wdpb,FALSE); 
		currarg = 1;
		fileopen = FALSE;
	}
	
	done = FALSE;
	while (!done) {  /* loop once per line */
		ok = TRUE;
		if (argc == 1) { /* get stdin line */
			linebcnt = 0;
			if ((needchunk) && (!lastchunk)) {
				err = SIGet(stdiop,chunkbuf,BUFSIZE,(short *)&bcount);
				chunkpos = 0;
				linecount = 0;
				needchunk = FALSE;
			}
			if (bcount != BUFSIZE) {
				lastchunk = TRUE;
			}
			linestart = chunkpos;
			while ((chunkbuf[chunkpos] != RET) && (chunkpos < bcount))
				chunkpos++; /* find a return */
			if (chunkbuf[chunkpos] == RET) { linecount++; chunkpos++; 
			} else { ok = FALSE; needchunk = TRUE; }
			if (linecount == 1) { /* is this the first line in this chunk? */
				BlockMove(leftover,linebuf, (Size)locnt); /* get the leftovers from the last one */
				linebcnt += locnt;
				if (chunkpos+locnt > BUFSIZE) { /* line too big? */
					showDiagnostic(longline);
					tempcnt = BUFSIZE - locnt;
				} else { /* size is ok, so record the number of bytes */
					tempcnt = chunkpos-linestart;
				}
			} else { /* not the first line in this chunk */
				tempcnt = chunkpos - linestart;
			}
			BlockMove(chunkbuf+linestart,linebuf+linebcnt,(Size)tempcnt);
			linebcnt += tempcnt;
			if (!ok) { /* record the leftover from this chunk */
				BlockMove(chunkbuf+linestart,leftover,(Size)linebcnt);
				locnt = linebcnt;
			}
			if ((!ok) && (lastchunk)) {
				if (linebcnt != 0) {
					ok = TRUE; /* do this so it'll grep anyway */
					linebuf[linebcnt] = RET; /* terminate it with a return */
				}
				done = TRUE;
			}
		} else { /* get a line from a file */
			if (!fileopen) {
				if (tmp = isDir(argv[currarg],wdpb.ioWDDirID,wdpb.ioWDVRefNum)  == 1) {
					showDiagnostic(direrr);
					currarg++;
					ok = FALSE;
				} else if (tmp == -1) {
					showDiagnostic(fileerr);
					DisposPtr((Ptr)linebuf);
					return;
				} else { /* it's a file */
					strcpy(fname, argv[currarg]);
					CtoPstr(fname); 
	
					hpb.ioParam.ioCompletion=0L;
					hpb.ioParam.ioNamePtr=(StringPtr)fname; 				
					hpb.ioParam.ioVRefNum=0; /* wdpbp->ioWDVRefNum; */
					hpb.ioParam.ioPermssn=fsRdPerm; 			
					hpb.ioParam.ioMisc = 0L; 					
					hpb.fileParam.ioDirID=0L; /* wdpbp->ioWDDirID; */
	
					theErr = PBHOpen(&hpb, FALSE);
					if (theErr) {
						showFileDiagnostic(fname,theErr); 
						DisposPtr((Ptr)linebuf);
						return;
					}	
					hpb.ioParam.ioReqCount = BUFSIZE; /* maximum */
					hpb.ioParam.ioBuffer = linebuf;
					hpb.ioParam.ioPosOffset = 0L; 
					hpb.ioParam.ioPosMode = 3456;/* IM-IV, p121 (read one line only)*/
					
					fileopen = TRUE;
				}
			}
			if (ok) {	
				theErr = PBRead((ParmBlkPtr)&hpb, FALSE); 
				if (theErr == -39) {
					theErr = PBClose((ParmBlkPtr)&hpb, FALSE);
					fileopen = FALSE;
					currarg++;
					linebuf[hpb.ioParam.ioActCount] = RET; /* grep the last line too */
				} else if (theErr) {
					showFileDiagnostic(fname,theErr);
					DisposPtr((Ptr)linebuf);
					return;
				}
			}
			
			if ((!fileopen) && (currarg >= argc)) {
				done = TRUE;
			}
		} /* end else argc != 1 */
		
		/* check for match */
		if (ok) {
			matchlen = grepMatch(argv[0],linebuf,casein);
			if (((matchlen) && (!notgrep)) || ((!matchlen) && (notgrep))) { 
				if (argc > 2) { /* put the file name in */
					strcpy(tempname,argv[currarg]);
					templen = strlen(tempname);
					tempname[templen++] = COLON;
					tempname[templen] = 0;
					SOInsert(stdiop,tempname,templen); 
				}
				SOInsert(stdiop, linebuf, strlen(linebuf));
				SOInsert(stdiop, RETURNSTR,1);
			} 
		}
	} /* get another line */
		
	DisposPtr((Ptr)linebuf);
	if (argc == 1) {
		DisposPtr((Ptr)chunkbuf);
	  	DisposPtr((Ptr)leftover);
	}
}

			

int grepMatch(mstr, line, ci)
	char	*mstr, *line;
	int		ci; /* case insensitive flag */
{
/* (we don't mess with mstr, but line gets a null terminator.)
 * put NULL terminator on "line"
 * check its length, chop it if necessary
 * filter out stray nulls
 * add the glob char (*) on both ends of "mstr"
 * pass on to checkMatch() 
 * return == 0 -> no match
 * return == line length -> match */
 	int	ret = 13;
 	int	limit = BUFSIZE; /* maximum line length */
 	int	len, i;
 	char	*gmstr;
 	char	*choperr = "grepMatch: chopping off the end of a long line! \r";
 	char	*memerr = "grepMatch: can't allocate memory! \r";
 
 	len = 0;
 	while ((line[len] != ret) && (len < (limit -1))) len++;
 	line[len] = 0;
 	if (len == (limit -1)) showDiagnostic(choperr);
 
	 i = 0; 
	 while (i < len) { /* filter out other nils */
	 	if (line[i] == 0) line[i] = 1;
	 	i++;
	 }
 
	 gmstr = NewPtr(strlen(mstr)+2);
	 if (gmstr == NIL) {
	 	showDiagnostic(memerr);
	 	return(0);
	 }
	 *gmstr = 42; /* "*" */
	 i = 0;
	 while (mstr[i] != 0) {
	 	gmstr[i+1] = mstr[i];
	 	i++;
	 }
	 gmstr[i+1] = 42;
	 gmstr[i+2] = 0;
 
	 if (checkMatch(line, gmstr, ci)) {
	 	DisposPtr((Ptr)gmstr);
	 	return(len);
	 } else {
	 	DisposPtr((Ptr)gmstr);
 		return(0);
	} 
}


doRm(argc,argv,stdiop)
	int			argc;
	char		**argv;
	StdioPtr	stdiop;
{
	OSErr		theErr;
	int			i, fromVolID, cwv,fromdir;
	int			dorecursive;
	long 		cwd, fromDirID;
	char		*fromname;
	char		*usage = "Usage: rm [-r] f1...dn \r";
	char		*myname = (char *)"\pdoRm";
	char		*norflag = "Rm: Directory specified, but no -r given. \r";
	char		*err2 = "Rm: \r";
	char		*err3 = "Rm: \r";
	
	dorecursive = FALSE;
	i = 1;
	if (strcmp(argv[i],"-r") == 0) {
		dorecursive = TRUE;			
		i = 2;
	}
	if (((argc == 2) && (dorecursive)) || (argc < 2)) {
		showDiagnostic(usage);
		return;
	}
	
	fromname = NewPtr(255);
		
	cwv = defaultVol();
	defaultDir(&cwd);
		
	while (i < argc) {
		fromVolID = getVol(argv[i]);
		if (!fromVolID) fromVolID = cwv;
		strcpy(fromname,argv[i]);
		if (fromdir = validDir(fromname,&fromDirID)) {
			if (countColons(fromname) == 0) 
				addColonFront(fromname);
		}
		if (!fromdir)  { /* remove a file */
			theErr = removeAFile(fromname);
		} else if (dorecursive) { /* remove a directory */
			theErr = removeRecursive(fromname,fromVolID,fromDirID);
		} else { /* no -r given */
			showDiagnostic(norflag);
		}
		theErr = FlushVol(0L,fromVolID);
		if (theErr) {
			showFileDiagnostic(myname,theErr);
		}
		i++;
	}

	DisposPtr((Ptr)fromname);	
}

OSErr removeAFile(name)
	char	*name;
{
	HParamBlockRec	pbh;
	OSErr			theErr;
	char			*temp;
	
	temp = NewPtr(255);
	strcpy(temp,name);
	
	CtoPstr(temp);
	pbh.fileParam.ioCompletion = 0L;
	pbh.fileParam.ioNamePtr = (StringPtr)temp;
	pbh.fileParam.ioVRefNum = 0;
	pbh.fileParam.ioDirID = 0L;
	
	theErr = PBHDelete(&pbh,FALSE);
	/* showDiagnostic("I'd be removing the file named: ");
	showDiagnostic(name);
	showDiagnostic(RETURNSTR);
	theErr = 0; */ 
	if (theErr)
		showFileDiagnostic(temp,theErr);
	DisposPtr((Ptr)temp);
	return(theErr);
}


OSErr removeRecursive(dname,volID,dirID)
	char	*dname;
	int		volID;
	long	dirID;
{
	OSErr		theErr;
	int			i,rmerr,err;
	char		*tempdir, *tempname, *newname;
	char		*err1 = "removeRecursive:  \r";
	char		*err2 = "removeRecursive: \r";
	CInfoPBRec	cpb;
	
	rmerr = FALSE;
	tempdir = NewPtr(255);
	tempname = NewPtr(255);
	newname = NewPtr(255);
	strcpy(tempdir,dname);
	
	CtoPstr(tempdir);
	
	cpb.hFileInfo.ioCompletion=0L;
	cpb.hFileInfo.ioDirID = dirID;
	cpb.hFileInfo.ioVRefNum = volID;
	cpb.hFileInfo.ioNamePtr=(StringPtr)tempname;
	cpb.hFileInfo.ioFDirIndex = 1;
	theErr=PBGetCatInfo(&cpb,FALSE);
	while (!theErr) {	
		strcpy(newname,dname);
		addColonEnd(newname);
		myPtoCstr(tempname);
		strcat(newname,tempname);
		CtoPstr(tempname);
		if (cpb.hFileInfo.ioFlAttrib & 0x10) {
			err = removeRecursive(newname,volID,cpb.hFileInfo.ioDirID);
			if (err) rmerr = TRUE;
		} else {
			err = removeAFile(newname);
			if (err) rmerr = TRUE; 
		}
		if (rmerr) /* if all goes well, the index stays at 1 */
			cpb.hFileInfo.ioFDirIndex++;
		cpb.hFileInfo.ioDirID = dirID;
		theErr=PBGetCatInfo(&cpb,FALSE);
	}
	if (!rmerr) {
		err = removeAFile(dname);
	}
	cleanup:
	DisposPtr((Ptr)tempdir);
	DisposPtr((Ptr)tempname);
	DisposPtr((Ptr)newname);
	return(rmerr);
}


doMv(argc,argv,stdiop)
	int			argc;
	char		**argv;
	StdioPtr	stdiop;
{
	OSErr		theErr,err;
	int			i, toVolID, fromVolID, cwv, todir;
	long 		toDirID,toParID,fromParID;
	char		*toname,*fromname,*toparent,*tolast,*fromparent,*fromlast;
	char		*usage = "Usage: mv f1 f2 or mv f1 ... fn d1 (`fn' is a file or directory). \r";
	char		*myname = (char *)"\pdoMv";
	char		*err1 = "Mv: Invalid volume name.\r";
	char		*err2 = "Mv: Can't move across volumes.  Use cp and rm.\r";
		
	toname = NewPtr(255);		/* path to copy to */
	fromname = NewPtr(255);		/* path to copy from */
	toparent = NewPtr(255);		/* name of parent of toname */
	tolast = NewPtr(255);		/* last segment of toname */
	fromparent = NewPtr(255);	/* name of parent of fromname */
	fromlast = NewPtr(255);		/* last segment of fromname */

	cwv = defaultVol();
	
	toVolID = getVol(argv[argc-1]); 
	if (!toVolID) toVolID = cwv;	
		
	strcpy(toname,argv[argc-1]);
	if (todir = validDir(toname,&toDirID)) { 
		/* We have an existing directory to copy to. 
		 * This may be a volume name followed by a colon. */
		if (countColons(toname) == 0) 
			addColonFront(toname);
	} else { /* otherwise it must be a nonexistent directory, or any file, existent or not. */
		theErr = getParentInfo(toname,toparent,tolast,&toParID);
		if (theErr) { /* can't work without a valid parent */
			CtoPstr(toname);
			showFileDiagnostic(toname,theErr);
			myPtoCstr(toname);
			goto cleanup; 
		}
	}
	
	if ((argc < 3) || ((argc > 3) && (!todir))) {
		showDiagnostic(usage);
		goto cleanup;
	}
	
	i = 1;

	while (i < (argc-1)) {
		fromVolID = getVol(argv[i]);
		if (!fromVolID) fromVolID = cwv;
		strcpy(fromname,argv[i]);
		
		/* get "from" parent directory name and ID */
		theErr = getParentInfo(fromname,fromparent,fromlast,&fromParID);
		if (theErr) { /* can't work without a valid parent */
			CtoPstr(fromname);
			showFileDiagnostic(fromname,theErr);
			myPtoCstr(fromname); 
		} else if (fromParID == 0L) { /* change volume name */
			err = renameIt(fromname,toname);
		} else if (fromVolID != toVolID) { /* can't move across volumes */
			showDiagnostic(err2);
		} else if ((toParID == 0L) && (!todir)) { /* invalid volume name */
			showDiagnostic(err1);
		} else 	if (todir) { 	/* move to an existing directory */
			err = moveIt(fromname,toname);
		} else if (strcmp(fromlast,tolast) == 0)  {  /* no name change */
			err = moveIt(fromname,toparent);
		} else if (toParID == fromParID) { /* files or dirs with the same parent */			
			err = renameIt(fromname,toname); 
		} else { /* all other cases: move and rename */
			err = moveAndRename(fromname,toname, fromparent,toparent);
		}
		if (!theErr) {
			theErr = FlushVol(0L,fromVolID);
			if (theErr) {
				showFileDiagnostic(myname,theErr);
			}
		}
		i++;
	}

	/* flush to volume */
	theErr = FlushVol(0L,toVolID);
	if (theErr) {
		showFileDiagnostic(myname,theErr);
	}	

	cleanup:
	DisposPtr((Ptr)toname);
	DisposPtr((Ptr)fromname);
	DisposPtr((Ptr)toparent);
	DisposPtr((Ptr)tolast);
	DisposPtr((Ptr)fromparent);
	DisposPtr((Ptr)fromlast);
}

OSErr moveAndRename(fromname, toname, fromparent,toparent)
	char		*fromname,*toname,*fromparent,*toparent;
{	
	char	*temppath,*tempname;
	int		err;
	OSErr	theErr;
	
	temppath = NewPtr(255);
	tempname = NewPtr(255);
	
	getTempName(tempname);
	
	strcpy(temppath,fromparent);
	strcat(temppath,tempname);
	
	theErr = renameIt(fromname,temppath);
	
	if (!theErr)
		theErr = moveIt(temppath,toparent);
	if (!theErr) {
		strcpy(temppath,toparent);
		strcat(temppath,tempname);
	}	
	
	if (!theErr) { /* rename to final name */
		theErr = renameIt(temppath,toname);
	} else {		/* or punt */
		theErr = renameIt(temppath,fromname);
	}
	
	cleanup:	
	DisposPtr((Ptr)temppath);
	DisposPtr((Ptr)tempname);
	return(theErr);
}




OSErr moveIt(fromname,toname)
	char	*fromname,*toname;
{	/* fromname is an existing file or directory.
	 * toname is an existing directory. */
	CMovePBRec	cpb;
	char		*temp;
	OSErr		theErr;
	
	CtoPstr(fromname);
	CtoPstr(toname);
	cpb.ioCompletion = 0L;
	cpb.ioNamePtr = (StringPtr)fromname;
	cpb.ioVRefNum = 0;
	cpb.ioNewName = (StringPtr)toname;
	cpb.ioNewDirID = 0L;
	cpb.ioDirID = 0L;
	
	theErr = PBCatMove(&cpb,FALSE);
	if (theErr) {
		showFileDiagnostic((char *)"\p",theErr);
	}
	myPtoCstr(fromname);
	myPtoCstr(toname);
	return(theErr);
}

OSErr renameIt(from,to)
	char	*from,*to;
{ /* from is an existing file or directory.
   * to is the new name it should get. */
	HParamBlockRec	hpb;
	OSErr			theErr;
	
   	CtoPstr(from);
	CtoPstr(to);
	hpb.ioParam.ioCompletion = 0L;
	hpb.ioParam.ioNamePtr = (StringPtr)from;
	hpb.ioParam.ioVRefNum = 0;
	hpb.ioParam.ioMisc = to;
	hpb.fileParam.ioDirID = 0L;
	theErr = PBHRename(&hpb,FALSE); 
   	if (theErr) {
		showFileDiagnostic((char *)"\p",theErr);
	}
	myPtoCstr(from);
	myPtoCstr(to);
	return(theErr);
}



OSErr getParentInfo(path,parent,last,dirIDp)  
	char	*path,*parent,*last;
	long	*dirIDp;
{ /* If it looks like a raw volume name, last = path, parent is NULL.
   * Names such as NULL,:, and :: get a colon added.   
   * Otherwise strip off everything back to the last colon, and return 
   * the directory ID and name of the resultant path.  */
	char		*temp;
	CInfoPBRec	cpb;
	int			len,i,colons;
	OSErr		theErr;
	
	temp = NewPtr(255);
	strcpy(temp,path);
	len = strlen(temp);
	if ((temp[len-1] == 58) && (countColons(temp) == 1) && (len > 1)) {
		/* it looks like a volume name alone */
		strcpy(last,path);
		*parent = 0;
		*dirIDp = 0L;
		theErr = 0;
	} else { 
		if (countColons(temp) == len)  { /* only colons */
			temp[len++] = COLON; 
			temp[len] = 0; 
			*last = 0;
		} else {
			while ((temp[len] != COLON) && (len >= 0)) len--;
			if (len == -1) { /* no colons */
				temp[++len] = COLON;
			}
			temp[len+1] = 0; 
			strcpy(last,path+len);
		}
		strcpy(parent,temp);
	
		CtoPstr(parent);
		cpb.dirInfo.ioVRefNum = 0;
		cpb.dirInfo.ioCompletion = 0L;
		cpb.dirInfo.ioNamePtr = (StringPtr)parent;
		cpb.dirInfo.ioFDirIndex = 0;
		cpb.dirInfo.ioDrDirID = 0;
		theErr = PBGetCatInfo(&cpb,FALSE);
		*dirIDp = cpb.dirInfo.ioDrDirID;
		myPtoCstr(parent);
	}
	DisposPtr((Ptr)temp);
	return(theErr);
}

int validDir(dname, dirIDp)
	char	*dname;
	long	*dirIDp;
{ /* This is like getDir, except we don't cut the end off dname before 
   * checking it.  It must be a valid dirname as is. */
	char		*temp;
	CInfoPBRec	cpb;
	OSErr		theErr;
	
	temp = NewPtr(255);
	strcpy(temp,dname);	
	cpb.dirInfo.ioVRefNum = 0;
	CtoPstr(temp);
	cpb.dirInfo.ioCompletion = 0L;
	cpb.dirInfo.ioNamePtr = (StringPtr)temp;
	cpb.dirInfo.ioFDirIndex = 0;
	cpb.dirInfo.ioDrDirID = 0;
	theErr = PBGetCatInfo(&cpb,FALSE);
	DisposPtr((Ptr)temp);
	if ((!theErr) && (cpb.hFileInfo.ioFlAttrib & 0x10)) {
		*dirIDp = cpb.dirInfo.ioDrDirID;
		return(1);
	} else {
		return(0);
	}
}

int	defaultVol()
{ /* return the default volume id */
	WDPBRec	wdpb;
	
	wdpb.ioNamePtr = (StringPtr)NewPtr(255); 
	wdpb.ioCompletion=0L;
	PBHGetVol(&wdpb,FALSE);
	DisposPtr((Ptr)wdpb.ioNamePtr);
	return(wdpb.ioWDVRefNum);
}

defaultDir(dirIDp)
	long	*dirIDp;
{ /* return the default dir id */
	WDPBRec	wdpb;
	
	wdpb.ioNamePtr = (StringPtr)NewPtr(255); 
	wdpb.ioCompletion=0L;
	PBHGetVol(&wdpb,FALSE);
	DisposPtr((Ptr)wdpb.ioNamePtr);
	*dirIDp = wdpb.ioWDDirID;
}

addColonFront(name)
	char	*name;
{	/* add a colon in front of a name if there isn't one there already */
	char	*temp;
	if (*name != COLON) {
		temp = NewPtr(255);
		*temp = COLON;
		strcpy(temp+1,name);
		strcpy(name,temp);
		DisposPtr((Ptr)temp);
	}	
}

addColonEnd(name)
	char	*name;
{ /* add a colon to the end of "name" if there isn't one already */
	int		len;
	len = strlen(name);
	if (name[len - 1] != COLON) {
		name[len++] = COLON;
		name[len] = 0;
	}
}

int countColons(str)
	char	*str;
{
	int		i, num;
	i = num = 0;
	while (str[i] != 0) {
		if (str[i] == COLON) num++;
		i++;
	}
	return(num);
}





