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
#include "regexp.h"
#include "shellutils.h"
#include "dirtools.h"
/* This file contains the code for the front end to egrep, as well as
 * related functions 'seg' and 'xn'.   
 * The regexp stuff is in regexp.c
 */

doEgrep(argc, argv, stdiop)
	int				argc;
	char			**argv;
	StdioPtr		stdiop;
{
/* 
 * Notes:
 *   To deal with long lines we'll truncate, (at 32k) 
 *   and warn the user when it happens.
 *
 *	 To augment this, a utility is provided to segment on word boundaries
 *   prior to egrep-ping.  
 *
 *   Also note that nulls in the file will cause havoc, so another utility
 *   is provided to filter them out.  Thus the entire mess might go something
 *   like this:  xn filename | seg | egrep keyword
 *
 *   When matches are found in long lines, we should only report a small part of the
 *   line containing the match...might not be easy to do this.
 *
 * ToDo:
 *
 * Standard egrep options to handle:
 *	 c	count matching lines only
 *   e  next argument is the expression to match which begins with a minus
 *   f  take r.e. from a file (only the first line?)
 *   l  list files with matching lines only once
 *   n  preceed each line with its line number
 *   v  display lines which do not match (done)
 *   s  silent mode (when we have shell variables and can return status)
 *
 * In the case of a long line, we should report only a little piece...or not?
 *
 * Bugs: none known.
 *  
 */
 	WDPBRec			wdpb;
 	HParamBlockRec	hpb;
 	regexp 			*r;
	int				done, err, ch, currarg, templen;
 	int				fileopen, ok, notgrep, optind, tmp;
 	int				needchunk, lastchunk, locnt, linebcnt; 
 	int				linestart, bcount, chunkpos,buflen;
 	int				linecount, tempcnt, where, match, nullShown;
 	OSErr			theErr;
	char			fname[256], *linebuf, tempname[256];
 	char			*usage = "Usage: egrep [-v] keyword [filename...]\r";
 	char			*direrr = "Egrep: Can't grep a directory. \r";
 	char			*fileerr = "Egrep: Error getting file info. \r";
	char			*trunc = "Egrep: Truncating long line. \r";
	char			*memerr = "Egrep: Low memory. \r";
	char			*reerr = "Egrep: Bad expression. \r";
	char			*nullWarn = "Egrep: Warning: The NULL character was found in input line(s). Line(s) will be truncated. \r";  	

	if (argc < 2) { 
		showDiagnostic(usage); 
		return;
	}
	
	buflen = BUFMAX;
	/* buflen = 256; */
	
	nullShown = FALSE;
	err = FALSE;
	notgrep = FALSE;
	optind = 0;
	while (((ch = getopt(argc, argv, "v",(short *)&optind)) != 0) && (!err)) {
		switch (ch) {
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
	} 
	if (!(linebuf = NewPtr(buflen))) {
		showDiagnostic(memerr);
		return;
	}
	
	/* compile the r.e. */
	if ((r = regcomp(argv[0])) == 0) {
		showDiagnostic(reerr);
		DisposPtr((Ptr)linebuf);
		return;
	}
	
	if (argc != 1) { /* prepare to read a file */
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
			if (SIReadLine(linebuf, buflen, stdiop, (short *)&templen)) {
				done = TRUE;
				ok = FALSE;
			} else {
				if (linebuf[templen-1] != RET) {
					if (templen == buflen-1) {
						showDiagnostic(trunc);
						dumpToEol(stdiop);
					} 
				} else {
					/* trash the return; replace it with null */
					linebuf[templen-1] = '\0';
				}
				/* see if there are any stray null chars. in this line. 
				 * if so, emit truncation warning */
				if (!nullShown && strayNulls(linebuf, templen-1)) {
				 	showDiagnostic(nullWarn);
				 	nullShown = TRUE;
				}
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
					DisposPtr((Ptr)r);
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
						DisposPtr((Ptr)r);
						return;
					}	
					hpb.ioParam.ioReqCount = (long)(buflen - 1); /* save room for a null. */
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
					if (hpb.ioParam.ioActCount == 0) {
						ok = FALSE; /* this happens if the last line is truncated */
					} else if (linebuf[hpb.ioParam.ioActCount - 1] == RET) {
						linebuf[hpb.ioParam.ioActCount - 1] = '\0'; 
					} else {
						linebuf[hpb.ioParam.ioActCount] = '\0';
					}
				} else if (theErr) {
					showFileDiagnostic(fname,theErr);
					theErr = PBClose((ParmBlkPtr)&hpb, FALSE);
					DisposPtr((Ptr)linebuf);
					DisposPtr((Ptr)r);
					return;
				} else {
					/* replace the return char. with a null */
					if (linebuf[hpb.ioParam.ioActCount - 1] == RET)
						linebuf[hpb.ioParam.ioActCount - 1] = '\0';
					else {
						if (hpb.ioParam.ioActCount == buflen - 1) {
							showDiagnostic(trunc);
							readToEol(&hpb);
						}
						linebuf[hpb.ioParam.ioActCount] = '\0';
					}
				}
				if (ok) {
					/* see if there are any stray null chars. in this line. 
					 * if so, emit truncation warning */
					if (!nullShown && strayNulls(linebuf, hpb.ioParam.ioActCount-1)) {
					 	showDiagnostic(nullWarn);
					 	nullShown = TRUE;
					}
				}
			}
			
			if ((!fileopen) && (currarg >= argc)) {
				done = TRUE;
			}
		} /* end else: (argc != 1) */
		
		if (ok) {
			match = regexec(r, linebuf);
			if (((match) && (!notgrep)) || ((!match) && (notgrep))) { 
				if (argc > 2) { /* put the file name in */
					strcpy(tempname,argv[currarg]);
					templen = strlen(tempname);
					tempname[templen++] = COLON;
					tempname[templen] = 0;
					SOInsert(stdiop,tempname,templen); 
				}
				/* if the line is really long, only report a little for context */
				SOInsert(stdiop, linebuf, strlen(linebuf));
				SOInsert(stdiop, RETURNSTR, 1);
			} 
		}
	} /* get another line */
		
	DisposPtr((Ptr)linebuf);
	DisposPtr((Ptr)r);
}

readToEol(hpbp)
HParmBlkPtr	hpbp;
{	
	/* assume hpbp PosMode is already configured to do readline */
	OSErr	theErr;
	char	junk[256], *tempBuf;
	int		tempCount, tempAct;
	
	tempCount = hpbp->ioParam.ioReqCount;
	hpbp->ioParam.ioReqCount = 255;
	
	tempAct = hpbp->ioParam.ioActCount;
	
	tempBuf = hpbp->ioParam.ioBuffer;
	hpbp->ioParam.ioBuffer = junk;
	
	theErr = PBRead((ParmBlkPtr)hpbp,FALSE);
	while ((!theErr) && (junk[hpbp->ioParam.ioActCount - 1] != RET)) {
		theErr = PBRead((ParmBlkPtr)hpbp,FALSE);
	}
	
	hpbp->ioParam.ioReqCount = tempCount;
	hpbp->ioParam.ioBuffer = tempBuf;
	hpbp->ioParam.ioActCount = tempAct;
}

dumpToEol(stdiop)
StdioPtr stdiop;
{
	char  	junk[256];
	int		i;
	/* do readlines until we get one with a return in it, or we hit 
	 * the end. */
	while (!SIReadLine(junk, 256, stdiop, (short *)&i)) {
		if (junk[i-1]==RET) break;
	} 
}

int strayNulls(buf, size)
 char 	*buf;
 int	size;
{
	int	i=0;
	
	while (i < size) {
		if (buf[i++]=='\0') {
			return(TRUE);
		}
	}
	return(FALSE);
}

short getIntArg(argvp,argcp,optindp)
	char ***argvp;
	short *argcp,*optindp;
{
/* get a positive integer argument from a command line. It may be 
 * either at &argv[optind][2] or argv[optind+1].  Convert it from 
 * a string to an integer, check that it's less than 32k, and readjust
 * argc and argv such that the argument containing the integer is the
 * first argument.  Set optind to 0. Return -1 if no integer is found. */
	char *nstr;
	long	longint;
	short	shortint;
	char	*oor = "Argument out of range. \r";
	
	if ((*argvp)[*optindp][2]==0) { /* look at next argument */
		if (isNum((*argvp)[(*optindp)+1])){						
			nstr = (*argvp)[(*optindp)+1];
			*argvp += (*optindp)+1;
			*argcp -= (*optindp)+1;
		} else {
			return(-1);
		}
	} else if (isNum(&(*argvp)[(*optindp)][2])){
		nstr = &(*argvp)[(*optindp)][2];
		*argvp += (*optindp);
		*argcp -= (*optindp);
	} else {
		return(-1);
	}
	(*optindp) = 0;
	if (strlen(nstr) > 6) {
		showDiagnostic(oor);
		return(-1);
	}
	CtoPstr(nstr);
	StringToNum((unsigned char *)nstr,&longint);
	myPtoCstr(nstr);
	if (longint<=32767) {
		shortint = longint;
		return(shortint);
	} else {
		showDiagnostic(oor);
		return(-1);
	} 
}

doSeg(argc, argv, stdiop)
	int			argc;
	char		**argv;
	StdioPtr	stdiop;
{
   /* Attempt to break the input into lines by adding returns after
    * spaces or tabs. By default we start trying to break a line after 
    * 65 characters.  If an appropriate place isn't found by the 4096th
    * character, we break anyway.  
    * 
    * To do:
    * Allow the user to specify  options "-b n" and "-m i"
    * where n is the number of the character after which we will begin
    * trying to find a place to add a break, and i is the point at 
    * which we add a break regardless.
    *
    */
	char				fname[256], *buffer;
 	HParamBlockRec		hpb;
	OSErr				theErr;
	int					currarg, err, done, bcount,derr;
	int					cnt, breakpast, max, i, j, fileopen, ok;
	short		optind, intarg;
	char		*tmpbuf,ch;
	char		*memerr = "Seg: Low memory error.\r";
	char		*usage = "Usage: seg [-s n1] [-m n2] [file...] (n1 & n2 are integers)\r";
	char		*direrr = "Seg: Can't segment a directory. \r";
	/* defalults */
	breakpast = 65;
	max = 4096;	
	
	/* get options */	
	err = FALSE;
	optind = 0;
	while (((ch = getopt(argc, argv, "sm",&optind)) != 0) && (!err)) {
		switch (ch) {
			case 's':
				intarg = getIntArg(&argv,(short *)&argc,&optind);
				if (intarg == -1) { 
					err = TRUE;
				} else {
					breakpast = intarg;
				}
				break;
			case 'm':
				intarg = getIntArg(&argv,(short *)&argc,&optind);
				if (intarg == -1) { 
					err = TRUE; 
				} else {
					max = intarg;
				}
				break;
			default:
				err = TRUE;
		}
	}
	if (err) {
		showDiagnostic(usage);
		return;
	}
	argc -= optind;
	argv += optind;	
	
	if (!(buffer = NewPtr(BUFSIZE))) {
		showDiagnostic(memerr);
		return;
	} 
	
	cnt = 0;
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
 		
 		tmpbuf = buffer;
 		/* use i to compare to bcount, j to index tmpbuf, and cnt to determine
 		 * where to begin looking for a place to insert the return.  The
 		 * difference between cnt and j is that cnt can carry over from
 		 * chunk to chunk.  */
		if (ok) {
			i = j = 0;
			while (i < bcount) {
				while  ((cnt < breakpast-1) && (cnt < max-1) &&
												(i < bcount)) {
					if (tmpbuf[j] == RET) {
						SOInsert(stdiop,tmpbuf,j+1);
						tmpbuf += j+1;
						cnt = 0; j = 0; i++;
					} else {
						i++; j++; cnt++;
					}
				}
				while ((i < bcount) && (cnt != 0)) {
					if (tmpbuf[j] == RET) {
						SOInsert(stdiop,tmpbuf,j+1);
						tmpbuf += j+1;
						cnt = 0; j = 0; i++;
					} else if ((tmpbuf[j] == SP) || (tmpbuf[j] == TAB)
								|| (cnt >= max-1)) {
						SOInsert(stdiop,tmpbuf,j+1);
						if (!((cnt >= max-1) && (tmpbuf[j+1] == RET)))
							SOInsert(stdiop,RETURNSTR,1);
						tmpbuf += j+1;
						cnt = 0; j = 0; i++;
					} else {
						i++; j++; cnt++;
					}
				}
			}
		    if (cnt != 0){
		    	SOInsert(stdiop,tmpbuf,j); /* leftover from this chunk */
		    }
		} /* end if (ok) */ 
    }
	DisposPtr((Ptr)buffer);
}


doXnull(argc, argv, stdiop)
	int			argc;
	char		**argv;
	StdioPtr	stdiop;
{
   /* 
   	*  Examine each character of the input.  If NULLs are found, filter 
   	*  them out.
    *
    */
	char				fname[256], *buffer;
 	HParamBlockRec		hpb;
	OSErr				theErr;
	int					currarg, err, done, bcount;
	int					fileopen, ok, i,j;
	char				*tempbuf;
	char		*memerr = "Xn: Low memory error.\r";
	char		*usage = "Usage: xn [file...] \r";
	char		*direrr = "Xn: Can't segment a directory. \r";
	
	
	if (!(buffer = NewPtr(BUFSIZE)) || !(tempbuf = NewPtr(BUFSIZE))) {
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
			i = j = 0;
			while (i < bcount) {
				if (buffer[i] != '\0') {
					tempbuf[j++] = buffer[i];
					
				}
				i++;
			}
			SOInsert(stdiop,tempbuf,j);
		}
   }
    	
	DisposPtr((Ptr)buffer);
}












