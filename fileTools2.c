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
#include "shellutils.h"
#include "dirtools.h"
#include "stdio.h"
/* This file contains code for chtype,chcreator,rgrep(in progress),
 * bintohex, head, tail */

short doChtype(argc, argv, stdiop)
	int				argc;
	char			**argv;
	StdioPtr		stdiop;
{
/* 
 *  Require two arguments, minimum.  The first one is the new type
 *  tag; others are file names.
 */
	char	*usage = "Usage: chtype TYPE filename [filename2...]\r";
	short	i;
	long	tmplong;
	HParamBlockRec	hpb;
	OSErr	theErr;

	if (argc < 3) {
		showDiagnostic(usage);
		return(TRUE);
	}
	
	/* convert argv[1] to a long integer */
	BlockMove(argv[1],(char *)&tmplong,4L);
	
	for(i=2;i<argc;i++) {
		CtoPstr(argv[i]);
		hpb.fileParam.ioCompletion = 0L;
		hpb.fileParam.ioNamePtr = (StringPtr)argv[i];
		hpb.fileParam.ioVRefNum = 0; 
		hpb.fileParam.ioFDirIndex = 0;
		hpb.fileParam.ioDirID = 0L; 	
	 	theErr = PBHGetFInfo(&hpb,FALSE);
	 	if (theErr) {
 			showFileDiagnostic(argv[i],theErr);
	 	} else {
		 	hpb.fileParam.ioFlFndrInfo.fdType = tmplong;
		 	hpb.fileParam.ioDirID = 0L;
		 	theErr = PBHSetFInfo(&hpb,FALSE);	
		}
	}
	
}

short doChcreator(argc, argv, stdiop)
	int				argc;
	char			**argv;
	StdioPtr		stdiop;
{
/* 
 *  Require two arguments, minimum.  The first one is the new type
 *  tag; others are file names.
 */
	char	*usage = "Usage: chcreator CRTR filename [filename2...]\r";
	short	i;
	long	tmplong;
	HParamBlockRec	hpb;
	OSErr	theErr;

	if (argc < 3) {
		showDiagnostic(usage);
		return(TRUE);
	}
	
	/* convert argv[1] to a long integer */
	BlockMove(argv[1],(char *)&tmplong,4L);
	
	for(i=2;i<argc;i++) {
		CtoPstr(argv[i]);
		hpb.fileParam.ioCompletion = 0L;
		hpb.fileParam.ioNamePtr = (StringPtr)argv[i];
		hpb.fileParam.ioVRefNum = 0; 
		hpb.fileParam.ioFDirIndex = 0;
		hpb.fileParam.ioDirID = 0L; 	
	 	theErr = PBHGetFInfo(&hpb,FALSE);
	 	if (theErr) {
 			showFileDiagnostic(argv[i],theErr);
	 	} else {
		 	hpb.fileParam.ioFlFndrInfo.fdCreator = tmplong;
		 	hpb.fileParam.ioDirID = 0L;
		 	theErr = PBHSetFInfo(&hpb,FALSE);	
		}
	}
	
}


short doRgrep(argc, argv, stdiop)
	int argc;
	char **argv;
	StdioPtr stdiop;
{
/* (This is still work in progress)
 * How to do rgrep...some ideas:
 *  -Does not work on stdin, only on resource forks of file
 *  -Should provide some means to input keyword as hexidecimal
 *  -Provide case-insensitive option (which would be irrelevant
 *   in conjunction with the hex option)
 *  -Should provide output fields (some optional?):
 *		file name (if there are more than one)
 *		resource name
 *		resource type
 *		resource ID
 *		context/offset
 *		entire resource
 *  -Accept glob character in keyword...??
 *  -Allow for keyword to be a type,name,or id, or some combination.
 *  -Could also provide inverse grep function (Not really necessary?)
 */
 char	*usage = "rgrep [-i] [-h] pattern filename [filename...] \r";
 char 	*noci = "rgrep: warning: illegal combination of flags; -i ignored.\r";
 char	pattern[256],ch;
 short	err,optind,currarg;
 RGFlagsRec	flags;
 
 flags.casein = FALSE;
 flags.notgrep = FALSE;
 flags.hexinput = FALSE;
 flags.mfiles = FALSE;
 optind = 0;
 err = FALSE;
 while (((ch = getopt(argc, argv, "ih",&optind)) != 0) && (!err)) {
	switch (ch) {
		case 'i':
			flags.casein = TRUE;
			break;
		case 'h':
			flags.hexinput = TRUE;
			break;
		default:
			showDiagnostic(usage);
			err = TRUE;
	}
 }
 if (err)
	return(1);
 argc -= optind;
 argv += optind;

 if (flags.casein && flags.hexinput) {
 	showDiagnostic(noci);
 	flags.casein = FALSE;
 }
 
 BlockMove(argv[0],flags.pattern,strlen(argv[0])+1);
 /* check and translate?? hex input */
 
 if (argc < 2) {
	showDiagnostic(usage);
	return(1);
 } 
 
 for(currarg = 1;currarg<argc;currarg++) {
 	err = rGrepFile(argv[currarg],stdiop,&flags);
 }
 return(0);
}



short rGrepFile(fname,stdiop,flags)
	char	*fname;
	StdioPtr	stdiop;
	RGFlagsPtr		flags;
{
 char	*norf = "No resource fork for this file. \r";
 char	*runningapp = "Can't open the resource fork of a running application.\r"; 
 char	*memerr = "Not enough memory available. \r";
 char	*noftypes = "Number of resource types: ";
 char	*fne = "Can't rgrep a folder.  File names expected. \r";
 int	fileid,err,ntypes,typen;
 char	tempstr[256];
 ResType	type;
 
 CtoPstr(fname);
  
 fileid = OpenResFile((unsigned char *)fname);
 if (fileid == -1) {
 	err = ResError();
 	/* returns error -49 when fed running applications */
 	/* How can we open these files read-only? */
 	/* returns -39 for no resource fork */
 	if (err == -49)
 		showDiagnostic(runningapp);
 	else if (err == -39)
 		showDiagnostic(norf);
 	else
 		if(isDir(fname,0L,0)) {
 			showDiagnostic(fne);
 		} else {
 			showFileDiagnostic(fname,(OSErr)err);
 		}
 	return(1);
 }
 
 
 ntypes = CountTypes();
 SOInsert(stdiop,noftypes,strlen(noftypes));
 NumToString((long)ntypes,(unsigned char *)tempstr);
 SOInsert(stdiop,tempstr+1,*tempstr);
 SOInsert(stdiop,RETURNSTR,1);
 
 for (typen=1;typen<ntypes;typen++) {
 	GetIndType(&type,typen);
 	err = RgType(stdiop,flags,fname,type,fileid);
 }
 /* Careful, don't close any resource file we didn't open ourselves.
  * gMyResFile tells me the id of the application resource file.
  * Ref number 2 represents the system resource file.  
  * Is there other checking we should do?
  */
 if ((fileid != gMyResFile) && (fileid != 2)) {
 	CloseResFile(fileid);
 }
 return(0);
 
}

short RgType(stdiop,flags,fname,type,fileid)
	StdioPtr		stdiop;
	RGFlagsPtr	flags;
	char		*fname;
	ResType		type;
	int			fileid;
{
	short 	rescount,i,err;
	Handle	rHandle;
	
	rescount = CountResources(type);
	for(i=1;i <= rescount;i++) {
		SetResLoad(FALSE);
		rHandle = GetIndResource(type,i);
		SetResLoad(TRUE);
		if(fileid == HomeResFile(rHandle)) {
			LoadResource(rHandle);
			err = RgRes(stdiop,flags,fname,type,rHandle);
			ReleaseResource(rHandle);
		}
	}	
}

short RgRes(stdiop,flags,fname,type,rHandle)
	StdioPtr stdiop;
	RGFlagsPtr flags;
	char		*fname; /* fname is a Pascal string */
	ResType		type;
	Handle		rHandle;
{
  




}


short doBintohex(argc, argv, stdiop)
	int			argc;
	char		**argv;
	StdioPtr	stdiop;
{
   /* 
   	*  Each input byte is converted to its hex representation.
   	*  bytes are separated by spaces.
    *
    */
	char				fname[256], *buffer;
 	HParamBlockRec		hpb;
	OSErr				theErr;
	int					currarg, err, done, bcount;
	int					fileopen, ok, i,j;
	char				*tempbuf;
	char		*memerr = "bintohex: Low memory error.\r";
	char		*usage = "Usage: bintohex [file...] \r";
	char		*direrr = "bintohex: Can't work on a directory. \r";
	
	
	if (!(buffer = NewPtr(BUFSIZE)) || !(tempbuf = NewPtr(BUFSIZE*3))) {
		showDiagnostic(memerr);
		return;
	} 
	
	fileopen = FALSE;
	currarg = 1;
	done = FALSE;
	while (!done) { /* loop once per bufferfull */	
		ok = TRUE;
		if (argc == 1) {
			/* read from stdin */
			err = FALSE;
			done = FALSE;
			err = SIGet(stdiop,buffer,BUFSIZE,(short *)&bcount);
			if (bcount != BUFSIZE) {
				done = TRUE;
			}
			if (err) {
				done = TRUE;
				ok = FALSE;
			}
		} else { /* read from a file */
			if (!fileopen) {
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
					if (isDir(argv[currarg],0L,0)) {
						showDiagnostic(direrr);
					} else {
						showFileDiagnostic(fname,theErr);
					}
					ok = FALSE;
					currarg++;
				} else {	
					hpb.ioParam.ioReqCount = BUFSIZE; /* maximum */
					hpb.ioParam.ioBuffer = buffer;
					hpb.ioParam.ioPosOffset = 0L; 
					hpb.ioParam.ioPosMode = 0;
					
					fileopen = TRUE;
				}
			}
			if (ok) {	
				theErr = PBRead((ParmBlkPtr)&hpb, FALSE); 
				if (theErr == -39) {
					theErr = PBClose((ParmBlkPtr)&hpb, FALSE);
					fileopen = FALSE;
					currarg++;
				} else if (theErr) {
					showFileDiagnostic(fname,theErr);
					theErr = PBClose((ParmBlkPtr)&hpb, FALSE);
					fileopen = FALSE;
					currarg++;
					ok = FALSE;
				}
				bcount = (int)hpb.ioParam.ioActCount;
			}
			
			if ((!fileopen) && (currarg >= argc)) {
				done = TRUE;
			}
 		}
 		
		if (ok) {
			binToHex((Byte *)buffer,(short *)&bcount,tempbuf);
			SOInsert(stdiop,tempbuf,bcount*3);
		}
   }
    	
	DisposPtr((Ptr)buffer);
	DisposPtr((Ptr)tempbuf);
}

short binToHex(bytestr,len,hexstr)
	Byte	*bytestr;
	short	*len;
	char	*hexstr;
{
	short i,j;
	j = 0;
	for(i=0;i<(*len);i++) {
		byteToHex(&(bytestr[i]),&(hexstr[j]));
		j += 2;
		hexstr[j] = ' ';
		j++;
	}
	hexstr[j] = 0;
	return(j);
}

byteToHex(abyte,hstr)
	Byte	*abyte;
	char	*hstr;
{
	short highnib,lownib;
	char  achar;
	highnib = (*abyte)/16;
	lownib = (*abyte) - (highnib*16);
	if (highnib < 10) {
		achar = highnib + '0';
	} else {
		achar = highnib + 'A' - 10;
	}
	*hstr = achar;
	if (lownib < 10) {
		achar = lownib + '0';
	} else {
		achar = lownib + 'A' - 10;	
	}
	*(hstr+1) = achar;
}

short doHead(argc, argv, stdiop)
	int			argc;
	char		**argv;
	StdioPtr	stdiop;
{
/* copy bytes from the top of the standard input, or from the named files
 * to the standard output.
 */
 	long			countbytes=0L,savecb=0L;
 	short			gotopt, err, done, fileopen, ok, derr;
 	short			optind, intarg, currarg, bcount;
	char			ch, *buffer;
	char			fname[256];
 	HParamBlockRec	hpb;
 	OSErr			theErr;
	
	char		*memerr = "Head: Low memory error.\r";
	char		*usage = "Usage: head [-b n1] [-k n2] [file...] (b and k are mutually exclusive).\r";
	char		*direrr = "Head: Can't work on a directory. \r";

	/* defalults */
	countbytes = 200L;
	
	/* get options */	
	err = FALSE;
	optind = 0;
	gotopt = FALSE;
	while (((ch = getopt(argc, argv, "b",&optind)) != 0) && (!err)) {
		switch (ch) {
			case 'b':
				intarg = getIntArg(&argv,(short *)&argc,&optind);
				if ((intarg == -1) || (gotopt)) { 
					err = TRUE;
				} else {
					countbytes = (long)intarg;
					gotopt = TRUE;
				}
				break;
			case 'k':
				intarg = getIntArg(&argv,(short *)&argc,&optind);
				if ((intarg == -1) || (gotopt)) { 
					err = TRUE;
				} else {
					
					countbytes = (long)(intarg) * (1024L);
					gotopt = TRUE;
				}
				break;
			default:
				err = TRUE;
		}
	}
	if (err) {
		showDiagnostic(usage);
		return(1);
	}
	argc -= optind;
	argv += optind;	
	
	if (!(buffer = NewPtr(BUFSIZE))) {
		showDiagnostic(memerr);
		return(1);
	} 
	
	savecb = countbytes;
	fileopen = FALSE;
	currarg = 0;
	done = FALSE; 
	while (!done) { /* loop once per bufferfull */	
		ok = TRUE;
		if (argc == 0) {
			/* read from stdin */
			err = FALSE;
			done = FALSE;
			err = SIGet(stdiop,buffer,BUFSIZE,(short *)&bcount);
			if (bcount != BUFSIZE) {
				done = TRUE;
			}
			if (err) {
				done = TRUE;
				ok = FALSE;
			}
		} else { /* read from a file */
			if (!fileopen) {
				strcpy(fname, argv[currarg]);
				CtoPstr(fname); 
	
				hpb.ioParam.ioCompletion=0L;
				hpb.ioParam.ioNamePtr=(StringPtr)fname; 				
				hpb.ioParam.ioVRefNum=0; /* wdpbp->ioWDVRefNum; */
				hpb.ioParam.ioPermssn=fsRdPerm; 			
				hpb.ioParam.ioMisc = 0L; 					
				hpb.fileParam.ioDirID=0L; /* wdpbp->ioWDDirID; */
	
				theErr = PBHOpen(&hpb, FALSE);
				countbytes = savecb;

				if (theErr) {
					derr = isDir(argv[currarg],0L,0);
					if (derr == 1) {
						showDiagnostic(direrr);
					} else {
						showFileDiagnostic(fname,theErr);
					}
					ok = FALSE;
					currarg++;
				} else {	
					hpb.ioParam.ioReqCount = BUFSIZE; /* maximum */
					hpb.ioParam.ioBuffer = buffer;
					hpb.ioParam.ioPosOffset = 0L; 
					hpb.ioParam.ioPosMode = 0;
					
					fileopen = TRUE;
				}
			}
			if (ok) {	
				theErr = PBRead((ParmBlkPtr)&hpb, FALSE); 
				if (theErr == -39) {
					theErr = PBClose((ParmBlkPtr)&hpb, FALSE);
					fileopen = FALSE;
					currarg++;
				} else if (theErr) {
					showFileDiagnostic(fname,theErr);
					theErr = PBClose((ParmBlkPtr)&hpb, FALSE);
					fileopen = FALSE;
					currarg++;
					ok = FALSE;
				}
				bcount = (int)hpb.ioParam.ioActCount;
			}
			
			if ((!fileopen) && (currarg >= argc)) {
				done = TRUE;
			}
 		} /* end else read from file */
 		
 		/* do the work here */
		if (ok) {
			if (bcount >= countbytes) {
				SOInsert(stdiop,buffer,countbytes);
				if (fileopen) {
					theErr = PBClose((ParmBlkPtr)&hpb, FALSE);
					currarg++;
					fileopen = FALSE;
					if (currarg >= argc) 
						done = TRUE;
				} else if (argc == 0) {
					done = TRUE;
				} else if (currarg >= argc) {
					done = TRUE;
				}
			} else {
				SOInsert(stdiop,buffer,bcount);
				countbytes = countbytes - bcount;
			}
		} /* end if (ok) */ 
		
    } /* end while not done */
    
	DisposPtr((Ptr)buffer);
	return(0);
}

short doTail(argc, argv, stdiop)
	int			argc;
	char		**argv;
	StdioPtr	stdiop;
{
/* copy bytes from the bottom of the standard input, or from the named files
 * to the standard output.
 */
 	long			countbytes=0L,savecb=0L,flen = 0L,bytesread,bsize;
 	long			silen;
 	short			derr, err, done, fileopen, ok, fromend, gotopt;
 	short			usebytes, stemp;
 	short			optind, intarg, currarg, bcount;
	char			ch, *buffer;
	char			fname[256];
 	HParamBlockRec	hpb;
 	OSErr			theErr;
	
	char	*memerr = "Tail: Not enough memory.\r";
	char	*usage = "Usage: tail [+b || -b || +k || -k n1] [file...] \r";
	char	*direrr = "Tail: Can't work on a directory. \r";
	char	*lamemsg = "Tail: Sorry, can't work with standard input > 32k.\r";
	
	/* defalults */
	countbytes = 200L;
	fromend = TRUE;
	
	/* get options */	
	usebytes = FALSE;
	err = FALSE;
	optind = 1;
	gotopt = FALSE;
	if (((argv[1][0] == '+') || (argv[1][0] == '-')) && 
		((argv[1][1] == 'b') || (argv[1][1] == 'k'))){
		if (argv[1][0] == '+') { fromend = FALSE; }
		if (argv[1][1] == 'b') { usebytes = TRUE; }
		optind = 1;
		intarg = getIntArg(&argv,(short *)&argc,&optind); /* these are all var args!! */
		if (intarg == -1) { err = TRUE; }
		else {
			gotopt = TRUE; optind++;
			if (usebytes) { 
				countbytes = (long)intarg;
			} else {
				countbytes = (long)(intarg) * (1024L);
			}
		}
	}
	if (err) {
		showDiagnostic(usage);
		return(1);
	}
	argc -= optind;
	argv += optind;	
	
	if (!(buffer = NewPtr(BUFSIZE))) {
		showDiagnostic(memerr);
		return(1);
	} 
	
	savecb = countbytes;
	fileopen = FALSE;
	currarg = 0;
	done = FALSE; 
	
	/* do the work here */
	if (argc > 0) { /* read from files */
	
		while (!done) { /* loop once per bufferfull */	
			ok = TRUE;
			if (!fileopen) {
				strcpy(fname, argv[currarg]);
				CtoPstr(fname); 
	
				hpb.ioParam.ioCompletion=0L;
				hpb.ioParam.ioNamePtr=(StringPtr)fname; 				
				hpb.ioParam.ioVRefNum=0; /* wdpbp->ioWDVRefNum; */
				hpb.ioParam.ioPermssn=fsRdPerm; 			
				hpb.ioParam.ioMisc = 0L; 					
				hpb.fileParam.ioDirID=0L; /* wdpbp->ioWDDirID; */
				
				hpb.fileParam.ioFDirIndex = 0;
				theErr = PBHGetFInfo(&hpb,FALSE);
				if (!theErr) {
					flen = hpb.fileParam.ioFlLgLen;
				}
				
				hpb.fileParam.ioDirID=0L;
				theErr = PBHOpen(&hpb, FALSE);
				countbytes = savecb;

				if (theErr) {
					derr = isDir(argv[currarg],0L,0);
					if (derr == 1) {
						showDiagnostic(direrr);
					} else {
						showFileDiagnostic(fname,theErr);
					}
					ok = FALSE;
					currarg++;
				} else {	
					hpb.ioParam.ioReqCount = BUFSIZE; 
					hpb.ioParam.ioBuffer = buffer;
					hpb.ioParam.ioPosMode = fsFromStart; 
					if (fromend) { 
						if ((flen-countbytes) < 0) {
							hpb.ioParam.ioPosOffset =  0L;
						} else { 
							hpb.ioParam.ioPosOffset = (flen - countbytes);
						}
					} else { 
						hpb.ioParam.ioPosOffset = countbytes;
					}
					
					fileopen = TRUE;
				}
			} /* end if not fileopen */
			
			if (ok) {	
				theErr = PBRead((ParmBlkPtr)&hpb, FALSE); 
				if (theErr == -39) {
					theErr = PBClose((ParmBlkPtr)&hpb, FALSE);
					fileopen = FALSE;
					currarg++;
				} else if (theErr) {
					showFileDiagnostic(fname,theErr);
					theErr = PBClose((ParmBlkPtr)&hpb, FALSE);
					fileopen = FALSE;
					currarg++;
					ok = FALSE;
				}
				/* if (hpb.ioParam.ioPosMode != fsAtMark) {
					hpb.ioParam.ioPosMode = fsAtMark;
					hpb.ioParam.ioPosOffset = 0L;
				} */
				
				bcount = (int)hpb.ioParam.ioActCount;
			}
			
			if ((!fileopen) && (currarg >= argc)) {
				done = TRUE;
			}
 		
 			/* do the work here */
			if (ok) {
				SOInsert(stdiop,buffer,bcount);
			} 
		} /* end while not done */
	
	} else { /* read from standard input */
		silen = SIGetSize(stdiop);
		if (fromend) {  /* convert to count from top */
			if (countbytes >= silen) {
				countbytes = 0L;
			} else {
				countbytes = silen - countbytes;
			}
		}
		err = FALSE;
		done = FALSE;
		bytesread = 0L;
		while (!done) {
			err = SIGet(stdiop,buffer,BUFSIZE,(short *)&bcount);
			if (bcount != BUFSIZE) {
				done = TRUE;
			}
			if (err) {
				done = TRUE;
			} else {
				bytesread += bcount;
				if (bytesread > countbytes) {
					if ((bytesread - countbytes) >= (long)bcount) {
						SOInsert(stdiop,buffer,bcount);
					} else {
						stemp = (short)(bytesread - countbytes);
						SOInsert(stdiop,buffer+bcount-stemp,stemp);
					}
				}
			}
		}
	}
	
	DisposPtr((Ptr)buffer);
	return(0);
}