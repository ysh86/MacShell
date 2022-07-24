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
#include "filetools.h"
#include "window.h"
#include "shellutils.h"
#include "copy.h"
#include "dirtools.h"
#include "standalone.h"
#include "alias.h"
#include "history.h"
#include "backquote.h"
#include "stdio.h"

/* This file contains code for globbing, commandline parsing, 
 * setup prompt, calling utilities,
 * close and quit, option handling, pascal string utils.  Some things
 * relating to pipes probably belong in stdio.c */



short checkArgvSize(argvp,argc,newargc)
char 	***argvp;
short	argc;
short	newargc;
/* 
 * We use the following gimmick to figure out how much space has been 
 * allocated for argv:
 * If argc<=255, the maximum argc is assumed to be 255.  If argc>=256 and
 * <= 511, the max is assumed to be 511, and so on.
 *
 * In this routine we attempt to allocate more space if necessary.  Use
 * the following technique:
 * Find the current maximum argc. Assume we increment it in ARGVINC units.
 * Make sure newargc+argc is still less than this maximum.
 * If it isn't, allocate enough new space in argv, and copy the existing pointers.
 * Return '1' for error (out of memory).
 * Allocate more space on 256,512,... if ARGVINC=256.
 */

{
	short i=0,j=0,maxargc;
	char **newargv;
	char *memerr = "Out of memory. \r";
	
	while ((i*ARGVINC)<=argc) i++;
	maxargc = (i*ARGVINC)-1;
	if (newargc > maxargc) {
		while ((j*ARGVINC)<=newargc) j++;
		if (!(newargv=(char **)NewPtr((j)*ARGVINC*sizeof(Ptr)))){
			showDiagnostic(memerr);
			return(1);
		} else {
			BlockMove(*argvp,newargv,(long)((argc+1)*sizeof(Ptr)));
			DisposPtr((Ptr)*argvp);
			*argvp=newargv;
		}
	}
 	return(0);
}



int checkMatch(string,globstr,ci)
	char	*string, *globstr;
	int		ci;
/* (recursive routine)
 * Compare the two Cstrings. Globstr may contain one or more 
 * multi-character wild-cards '*'.  Return = 1 if there is a match.
 * "ci" is the case-insensitive flag.
 * 12/95: The order of the first two if clauses in the while loop were reversed
 *        to fix a bug in grep which caused lines beginning with * to not be 
 *        found.
 */
{
	int		i, j, k, match, done;
	
	done = FALSE;
	match = FALSE;
	i=j=0;
	
	while (!done)	{	
		if (globstr[j] == 42) { /* 42 == '*' */
			k = j + 1;
			while (globstr[k] == 42) k++; /* go to the next non-asterisk */
			while ((!compareChars(globstr[k], string[i], ci)) && (string[i] != 0)) i++;
			if (string[i] == 0) {
				done = TRUE;
				if (globstr[k] == 0)
					match = TRUE;
				else
					match = FALSE;
			} else {
				match = checkMatch(&string[i],&globstr[k], ci);
				if (match) done = TRUE;
				else i++;
			} 
		} else if (compareChars(string[i], globstr[j], ci)) {
			i++;
			j++;					
		} else {
			match = FALSE;
			done = TRUE;
		}
		if ((string[i] == 0) && (globstr[j] == 0)) {
			match = TRUE;
			done = TRUE;
		}
	}
	if (match) {
		/* showDiagnostic("match: ");	
		showDiagnostic(string);
		showDiagnostic("\r"); */
		return(1);
	} else {
		return(0);
	}
}

int	compareChars(c1, c2, ci)
	int		c1,c2,ci;
{ /* Compare the characters c1 and c2.  If ci is true, compare 
   * without regard to case.  */
   
   	if (ci) { /* convert to lower case */
    	if ((c1 >= 65) && (c1 <= 90)) {
    		c1 = c1 + 32;
    	}
    	if ((c2 >= 65) && (c2 <= 90)) {
    		c2 = c2 + 32;
    	}
    	if (c1 == c2) return(1);
    	else return(0);
    } else {
    	if (c1 == c2) return(1);
    	else return(0);
    }	
}



int expandGlob(oldstr,newargvp,newargcp)
	char	*oldstr, ***newargvp;
	int		*newargcp;
/* return a list of matches in newargvp, and the count in newargcp.
 * return=0 -> no match.
 * The path segment with the glob in it must be the last one.
 * If we pass this something like :dirname*  we get back :dirnameblah (changed 4/20)
 */ 
{
	int				i, j, getdir, newargc;
	char			*tempstr,*globstr,*fname,*wdout, *path;
	OSErr			fileErr,volID,err;
	WDPBRec			pb;
	CInfoPBRec		cpb;
	long			dirID;
	
	tempstr = NewPtr(255);
	globstr = NewPtr(255);
	path = NewPtr(255);
	
	/* get the default directory & volume data */	
	wdout=NewPtr(256);
	pb.ioNamePtr=(unsigned char *) wdout;			
	pb.ioCompletion=0L;
	PBHGetVol(&pb,FALSE); 	

	/* If the glob isn't in the cwd, we need to get some
	 * reference numbers.  First we'll chop up the string a little. */
	i=0;
	while ((oldstr[i] != 0) && (oldstr[i] != 42)) i++; 	/* 42='*' */
	while ((i>=0) && (oldstr[i] != 58)) i--; 			/* 58=':' */
	strcpy(globstr,(oldstr+i+1));
	strcpy(tempstr,oldstr);
	tempstr[i+1] = 0; 
	if (i < 0) {
		getdir = FALSE; /* try to match something in the cwd*/
		dirID = pb.ioWDDirID;
		volID = pb.ioVRefNum;
	} else { 
		getdir = TRUE;	/* try to match something in some other directory */
		/* or in the current directory if oldstr == :dirname* */
		volID = getVol(tempstr); /* another volume? */
		if (volID==0) volID = pb.ioVRefNum; 
		err = getDir(tempstr,volID,&dirID);
		if (err) {
			DisposPtr((Ptr)wdout);
			DisposPtr((Ptr)tempstr);
			DisposPtr((Ptr)globstr);
			DisposPtr((Ptr)path);			
			return(0); 
		}
	}

	
	fname=NewPtr(256);
	cpb.hFileInfo.ioDirID=dirID; /* current directory ID */
	cpb.hFileInfo.ioNamePtr=(unsigned char *) fname; 	
	cpb.hFileInfo.ioCompletion=0L; 	 
	cpb.hFileInfo.ioVRefNum= volID;
	cpb.hFileInfo.ioFDirIndex = 1;
	
	fileErr=PBGetCatInfo(&cpb,FALSE);
	
	newargc = 0;
	
	while (!fileErr){ 
	  	myPtoCstr(fname);
	  	if (checkMatch(fname,globstr,1)) {  /* case insensitive */
	  		/* -update newargvp & newargc */
	  		if (getdir) {
	  			strcpy(path,tempstr);
	  			strcat(path,fname);
	 		} else {
	 			strcpy(path,fname);
	 		}
	 		
	 		/* sure would be nice to not add quite so much overhead here */
	 		if (checkArgvSize(newargvp,newargc,newargc+1)) {
					if (newargc > 0) { 
						(newargc)--;
						disposArgs(&newargc, *newargvp);
					}
					return(-1);
			}

	  		(*newargvp)[newargc] = NewPtr(strlen(path) + 1);
			strcpy((*newargvp)[newargc],path);
			newargc++; 	
		}
		CtoPstr(fname);
		/* setup for the next iteration */
		cpb.hFileInfo.ioDirID=dirID; /* set it back to the correct dir...*/
		cpb.hFileInfo.ioFDirIndex++;
		fileErr=PBGetCatInfo(&cpb,FALSE);
		
	} 
	
	DisposPtr((Ptr)fname);
	DisposPtr((Ptr)wdout);
	DisposPtr((Ptr)tempstr);
	DisposPtr((Ptr)globstr);
	DisposPtr((Ptr)path);

	if (newargc > 0) {
		(*newargvp)[newargc] = NewPtr(1);
		*((*newargvp)[newargc]) = 0;
		*newargcp = newargc; 
		return(1);
	} else {
 		return(0); /* no match */
 	}
	
}

getTempName(name)
	char	*name;
{ /* make a name for a temp file */
	long	time;
	char	*timestr,*seq;
	char	*prefix = "MS";
	
	timestr = NewPtr(256);
	seq = NewPtr(256);
	
	GetDateTime((unsigned long *)&time);
	NumToString(time,(unsigned char *)timestr);
	myPtoCstr(timestr);
	
	NumToString((long)gSeq,(unsigned char *)seq);
	myPtoCstr(seq);
	gSeq++;
	
	strcpy(name,seq);
	strcat(name,prefix);	
	strcat(name,timestr);
		
	DisposPtr((Ptr)timestr);
	DisposPtr((Ptr)seq);
}



extractSeg(argc, argv, i, segargcp, segargv)
	int		argc, i, *segargcp;
	char	**argv, **segargv;
{/* We use this to split a command line consisting of multiple segments
  * separated by the pipe character into pipe-free segments. We return the
  * segment indexed by i. */  
	int		j,k;
	char	*tempargv;
	
	j = 1;
	k = 0;
	while (j < i) { /* jump ahead to segment i */
		if (strcmp(argv[k], "|") == 0) {
			j++;
		}
		k++;
	}
	
	j = 0;
	/* copy the args */
	while ((strcmp(argv[k], "|") != 0) && (k < argc)) {
		tempargv = NewPtr(strlen(argv[k]) + 1);
		strcpy(tempargv, argv[k]);
		segargv[j] = tempargv;
		j++;
		k++;
	}
	tempargv = NewPtr(1);
	tempargv[0] = 0;
	segargv[j] = tempargv;
	*segargcp = j;	
}


flushPipeBuf(stdiop)
	StdioRec	*stdiop;
{ /* This could get called either when the buffer is full, or when
   * the utility is done and stdout is about to get closed.
   * If outBuf is full, we'll try first to increase it's size 
   * (up to 32k).  When the buffer reaches its maximum size, we'll
   * create a temp file, and write our data into that, then change 
   * outType accordingly. */
   
   	char	*tempfile = "Pipe full, using temp file. \r";
   	char	*fileerr = "Error creating temp file for pipe. \r";
   	char	*tempptr;
   	int		ok,temp,err;
   	Size	tempsize;
   	

	if (stdiop->outLen == stdiop->outSize) {
		ok = FALSE;
   		if (stdiop->outSize < BUFMAX) {
   			/* try to increase the buffer size */
   			tempptr = stdiop->outBuf;
   			/* use SetPtrSize instead */
   			if ((stdiop->outBuf = NewPtr(BUFMAX))) {
   				ok = TRUE;
   				BlockMove(tempptr,stdiop->outBuf,(Size)stdiop->outSize);
   				stdiop->outSize = BUFMAX;
   				DisposPtr((Ptr)tempptr);
   			} else {
   				stdiop->outBuf = tempptr;
   			}
   			tempsize = GetPtrSize(stdiop->outBuf);
   		}
   		if (!ok) {
			err = createPipeFile(stdiop);
			stdiop->outLen = 0;
			stdiop->outPos = 0;
			if (err) {
				showDiagnostic(fileerr);
				return;
			}
			/* showDiagnostic(tempfile); */
   		}
   	} else { 
   		/* not full, do nothing */
   	}

}

closePipeFile(stdiop)
	StdioRec	*stdiop;
{	/* Close and remove temp files used by pipes as appropriate. */
	/* Temp file used for stdout: flush it.
	 * Temp file used for stdin: close it and remove it.
	 */
	 
	ParmBlkPtr	pbp;
	OSErr		theErr;
	
	if (stdiop->outType == 3) {
		pbp = stdiop->outPBP;
		PBFlushFile(pbp,FALSE);
	}
	
	if (stdiop->inType == 3) {
		pbp = stdiop->inPBP;
 		if (theErr = PBClose(pbp,FALSE)) {
 			showFileDiagnostic((char *)pbp->ioParam.ioNamePtr,theErr);
 			return(1);
 		}
 		if (theErr = PBDelete(pbp,FALSE)) {
 			showFileDiagnostic((char *)pbp->ioParam.ioNamePtr,theErr);
 			return(1);
 		}
		DisposPtr((Ptr)pbp);
	}
}

int	openStdinPipe(stdiop)
	StdioRec	*stdiop;
{ /* Set mark to the beginning of temp file for stdin. */
	ParmBlkPtr	pbp;
	OSErr		theErr;
	
	pbp = stdiop->inPBP;
	pbp->ioParam.ioPosMode = fsFromStart;
	pbp->ioParam.ioPosOffset = 0L;
 	if (theErr = PBSetFPos(pbp,FALSE)) {
 		showFileDiagnostic((char *)pbp->ioParam.ioNamePtr,theErr);
 		return(1);
 	}
	pbp->ioParam.ioPosMode = fsAtMark;
 	
 	return(0);
 	
}

int createPipeFile(stdiop)
	StdioRec	*stdiop;
{ /* Create, open, and write to temp file. Change outType. */
  /* Construct file name.
   * create file.
   * open for writing.
   * write outBuf.
   * record refNum, pbp, file name
   * change outType.
   */
   
	ParmBlkPtr	pbp;
	OSErr			theErr;
	char	*fname;
	
	fname = NewPtr(256);
	getTempName(fname);
	CtoPstr(fname);

	pbp = (ParmBlkPtr)NewPtr(sizeof(ParamBlockRec));
	pbp->ioParam.ioCompletion = 0L;
	pbp->ioParam.ioNamePtr = (StringPtr)fname;
	pbp->ioParam.ioVRefNum = -1; /* use the root directory */
	pbp->ioParam.ioVersNum = 0;
	
 	if (theErr = PBCreate(pbp,FALSE)) {
 		showFileDiagnostic(fname,theErr);
 		goto hell;
 	}

	pbp->ioParam.ioPermssn = fsRdWrPerm; /* exclusive read write */
	pbp->ioParam.ioMisc = NIL;
 	if (theErr = PBOpen(pbp,FALSE)) {
 		showFileDiagnostic(fname,theErr);
 		goto hell;
 	}
	
	pbp->ioParam.ioBuffer = stdiop->outBuf;
	pbp->ioParam.ioReqCount = (long)stdiop->outLen;
	pbp->ioParam.ioPosMode = fsAtMark;
	pbp->ioParam.ioPosOffset = 0L;
 	if (theErr = PBWrite(pbp,FALSE)) {
 		showFileDiagnostic(fname,theErr);
 		goto hell;
 	}
	
	pstrcpy(stdiop->outName,fname);
	pbp->ioParam.ioNamePtr = (StringPtr)stdiop->outName;
	stdiop->outRefnum = pbp->ioParam.ioRefNum;
	stdiop->outPBP = pbp;
	stdiop->outType = 3;
	DisposPtr((Ptr)fname);
	return(0);
	
	hell:
	PBClose(pbp,FALSE);
	PBDelete(pbp,FALSE);
	DisposPtr((Ptr)fname);
	DisposPtr((Ptr)pbp);
	return(1);
	
}

int flushPipeFile(stdiop)
	StdioRec	*stdiop;
{ /* Write outBuf to the temp file. */
	ParmBlkPtr	pbp;
	OSErr		theErr;
	
	pbp = stdiop->outPBP;
	pbp->ioParam.ioPosMode = fsAtMark;
	pbp->ioParam.ioPosOffset = 0L;
	pbp->ioParam.ioReqCount = (long)stdiop->outLen;
 	if (theErr = PBWrite(pbp,FALSE)) {
 		showFileDiagnostic((char *)pbp->ioParam.ioNamePtr,theErr);
 		return(1);
 	}
	stdiop->outPos = 0;
	stdiop->outLen = 0;
	return(0);
}

readPipeFile(stdiop, req)
	StdioRec	*stdiop;
	int			req;
{ /* read from temp file to stdin. */
	ParmBlkPtr	pbp;
	OSErr		theErr;
	
	pbp = stdiop->inPBP;
	pbp->ioParam.ioBuffer = stdiop->inBuf;
	pbp->ioParam.ioReqCount = req;
	pbp->ioParam.ioPosMode = fsAtMark;
 	if (theErr = PBRead(pbp,FALSE)) {
 		if (theErr != -39) {
 			showFileDiagnostic((char *)pbp->ioParam.ioNamePtr,theErr);
 			return(1);
 		}
 	}
	stdiop->inLen = pbp->ioParam.ioActCount;
	stdiop->inPos = 0;
}


int	countSegments(argc,argv)
	int		argc;
	char	**argv;
/* Return one more than the number of occurences of "|" in argv. */
{
	int		i, count;
	i = count = 0;
	while (i < argc) {
		if (strcmp(argv[i],"|") == 0) {
			count++;
		}
		i++;
	}
	return(count+1);
}

int oldparseLine(Command,argcp,argv) /* return=1 -> mis-matched quotes */
	char	*Command, **argv;
	int		*argcp;
/* We respect single and double quoted entities as arguments.  
 * The quotes themselves are left alone here.  
 * If not quoted, ">",">>","<", and "|" form seperate arguments,
 * even if they aren't surrounded by whitespace. 
 *
 * In general we want to allow quoted spaces to be embedded
 * in arguments. Something like -d" " should be interpreted as one
 * argument, as is: "-d "
 *
 * To do:
 * We also should pass anything following a backslash unchanged.
 * All of the quoting characters remain part of the argument until
 * we do the globbing.
 *  
 * Globbing and variable and alias substitution routines need to be updated
 * if changes are made here.
 */
{	
 char		*temp;							
 int		i,j,argc,quote; 
 temp = NewPtr(256);
 
 argc = 0;
 i = 0;
 while (Command[i] != 0) {
 	while ((Command[i] == SP) || (Command[i] == TAB))
 		i++; 						/* go past leading spaces and tabs */
 	if (Command[i] != 0) {			/* got one on the line. */
 		argc++;						
 		j = 0;
		if (Command[i] == GT) { /* take one or two consecutive >'s */
 			temp[j++] = Command[i++];
			if (Command[i] == GT)  	temp[j++] = Command[i++];
		} else if  ((Command[i] == LT) || (Command[i] == PIPE)) {
			/* if it's LT, or PIPE, take one character only */
 			temp[j++] = Command[i++];
 		} else {	
			/* it's not one of the special characters, *
			 * so just go to the next space, tab, null, pipe, "<", or ">" 	*/
 			while 	((Command[i] != 0) && (Command[i] != SP) && (Command[i] != TAB) &&
 					(Command[i] != GT) && (Command[i] != LT) && (Command[i] != PIPE)) {
 
 				if ((Command[i] == DQ) || (Command[i] == SQ)) { /* if it's a quote */
 					quote = Command[i];
 					temp[j++] = Command[i++]; 		/* record the quote */
 					while ((Command[i] != 0) && (Command[i] != quote)) {
 						temp[j++] = Command[i++];
 					}
 					if (Command[i] == 0) {		/* mismatched quotes */
 						*argcp = 0;
 						DisposPtr((Ptr)temp);
 						return(1);
 					} else {
 						temp[j++] = Command[i++];  /* record the closing quote */	
 					}
				} else {
					temp[j++] = Command[i++];
				}
 			}
 		}		
 		temp[j] = 0;	/* null terminate it */
 		argv[argc-1] = NewPtr(j+1);
 		strcpy(argv[argc-1], temp);
 	}
  }
  argv[argc] = NewPtr(1);	
  *(argv[argc]) = 0;	/* per convention */
  *argcp = argc; 
  DisposPtr((Ptr)temp);
  return(0);
}



doCallCmd(theText,cmdh)
	TEHandle	theText;
	char		**cmdh;
{	/* Adjust the size of the TE Record and the position of the cursor in both
	 * Interactive and Diagnostic windows, then call the command, then put up a 
	 * new prompt and readjust things.  This is called by HandleKeyDown and 
	 * pasteToInteractive. */
	 
	TEHandle	tempText;
	
	tempText = unpackTEH(diagnosticWin);
	if ((*tempText)->teLength >= scrollback + 4096) { /* extra 4K so we don't chop it every time */
	 	adjustTESize(tempText);		/* cut back to "scrollback" bytes */	
	 	adjustTEPos(diagnosticWin);	/* put the cursor back on the screen */
	}
	if ((*theText)->teLength >= scrollback + 4096) { 
	 	adjustTESize(theText);					
	 	adjustTEPos(interactiveWin);	
	}
	HandleInteractiveCmd(cmdh, theText); 
	putUpPrompt(theText);
	adjustTEPos(interactiveWin);
	updateThumbValues(interactiveWin); /* update scrollbar "thumb" */

}

int handleAllLines(argcp,argvp,stdiop)
	int			*argcp;
	char		***argvp;
	StdioPtr	stdiop;
{
/* This function receives pre-parsed args, and handles globbing, 
 * variable substitutions etc., then does stdio 
 * assignments, manages any pipes, then calls utilities.  It should have a true/false 
 * return value.  This function gets called by the script handler as well as
 * by the interactive mode handler.  
 * Revision plans:
 *   -count the segments
 *   -preserve the stdio record
 *   -for each segment:
 *      -do alias, variable, and filename substitution
 *		-setup stdio
 *      -call utils
 *   -dispose of memory
 * 
 */
	int			globerr, stdioerr, err, pipeSegs, i, vserr,aerr;
	int			segargc;
	StdioPtr	tempstdiop;
	char		**segargv;
	char		*stdiomsg = "Error with Standard I/O. \r"; 
	char		*globerrmsg = "No Match. \r";
	char		*memerr = "Out of memory. \r";
	
	err = FALSE;
	
   aerr = aliasSubstitute((short *)argcp,argvp);
    if (aerr) { 
    	return(aerr);
    }
	
   vserr = varSubstitute((short *)argcp,argvp);
    if (vserr) { 
    	return(vserr);
    }
				
	globerr = doGlob(argcp,argvp); 
	if (globerr) {
		/* showDiagnostic(globerrmsg); */
		return(globerr);
	}
	
	pipeSegs = countSegments(*argcp,*argvp);
	if (!(segargv = (char **)NewPtr((*argcp+1)*sizeof(Ptr)))){
		showDiagnostic(memerr);
		return(1);
	}
		
	i = 1;
	
	/* if the inherit flag is set and we have more than one segment,
	 * we need to preserve stdout here, and restore it as the default for
	 * the final segment. */
	tempstdiop = (StdioPtr)NewPtr(sizeof(StdioRec));
	BlockMove(stdiop,tempstdiop,(Size)sizeof(StdioRec));
	 
	while ((i <= pipeSegs) && (!err)) {
		extractSeg(*argcp, *argvp, i, &segargc, segargv);

		/* this is a really clunky way to do this...ok, so this
		 * whole routine needs some rethinking. In particular, variable
		 * substitution and globbing should also be done within this 
		 * while loop. */		
		if (i != 1) { 
			aerr = aliasSubstitute((short *)&segargc,&segargv);
			if (aerr) { 
				disposArgs(&segargc, segargv);
				DisposPtr((Ptr)segargv); 
				DisposPtr((Ptr)tempstdiop);	
    			return(aerr);
    		}
		}
		
		stdioerr = setupStdio((short *)&segargc,segargv,i,pipeSegs,stdiop,tempstdiop);
		
		if (!stdioerr) { 
			err = callUtility(stdiop,&segargc,segargv);
			flushStdout(stdiop,stdiop->outLen); /* flush here so utilities don't have to */
			closeStdio(stdiop); 
		} else {
			showDiagnostic(stdiomsg);
			err = stdioerr;
		} 
		
		disposArgs(&segargc, segargv);	
		i++;
	}
	DisposPtr((Ptr)segargv); 
	DisposPtr((Ptr)tempstdiop);
	return(err); 
}	



int shortLineError(tempText) 	/* detect the error condition of a line shorter 
							 	 * than the prompt */
	TEHandle		tempText;
{
	int		sStart, sEnd, textEnd, cCount;
	Handle	TextH;
	
	sStart = (*tempText)->selStart;
	sEnd =   (*tempText)->selEnd;
	textEnd = (*tempText)->teLength;
	
	TextH = (*tempText)->hText;
	/* number of characters in the current line=>cCount */ 
	HLock((Handle)TextH);
	cCount = 1;
	while ((*TextH)[textEnd-cCount] != RET) cCount++;
	HUnlock((Handle)TextH);
	if (cCount <= strlen(PROMPT))
		return(1); /* error condition true */
	else
		return(0);
} /* end shortLineError */

	

putUpPrompt(tempText)
	TEHandle	tempText;
{
	/* print the new prompt */ 
	/* We insert a return if the TErecord doesn't have one already. */
	if ((*(*tempText)->hText)[((*tempText)->teLength) -1] != RET) {
		TEInsert(RETURNSTR, strlen(RETURNSTR),tempText);
	}
	TEInsert(PROMPT, strlen(PROMPT), tempText);
	/* TEInsert(SPACE, strlen(SPACE), tempText); */
}



int lookForGlob(anArg)
/* If the string passed contains an asterisk which is not quoted, 
 * return=1, else return=0.
 */
	char	*anArg;
{
	int		i,quote;
	
	i = 0;
	while ((anArg[i] != '\0')) {
		if ((anArg[i] == SQ) || (anArg[i] == DQ)) {
			quote = anArg[i]; i++;
			while (anArg[i] != quote) i++;
			i++;
		} else if (anArg[i] == '*') {
			return(1);
		} else {
			i++;
		}
	}
	return(0);
}



disposArgs(argcp, argv)
	int			*argcp;
	char		**argv;
{ /* dispose of an array of pointers.
   * We actually remove *argcp+1 pointers. */
	int			i;
	i = 0;
	while (i <= *argcp) {
		DisposPtr((Ptr)argv[i]);
		i++;
	}
}

int doGlob(argcp, argvp)
/* Look throught the strings in argv. if one contains a supported  
 * glob character, do the following operations:
 * -if it's quoted, just remove the quotes.  Otherwise:
 * -get a list of matches
 * -allocate more space in newargv if necessary, then copy
 * -add the length of the list to newargc
 * -insert the new list in place of the argv[] which contained the glob.
 * return=1 -> glob error (no match), or out of memory.
 */
	char	***argvp;
	int		*argcp;
{
	int		i, j, argc, gotOne, howMany, newargc, tempargc;
	short	len,egerr;
	char	**newargv, **tempargv;
	char	*nomatch = "No match.\r";
	char	*memerr = "Out of memory. \r";
	
	newargv=(char **)NewPtr(ARGVINC*sizeof(Ptr));
	tempargv=(char **)NewPtr(ARGVINC*sizeof(Ptr));
	
	argc = *argcp;
	i = 0;
	howMany = 0;
	newargc = 0;
	

	while (i < argc) {	
		if (lookForGlob((*argvp)[i])) { 
		 	if(stripQuotes(&(*argvp)[i])) {
	 			if (newargc > 0) { 
					newargc--;
					disposArgs(&newargc, newargv);
				}
				DisposPtr((Ptr)newargv); 
				DisposPtr((Ptr)tempargv);
		 		return(1);
		 	}
			if ((egerr = expandGlob((*argvp)[i],&tempargv,&tempargc))==1) { 
				j=0;
				if (checkArgvSize(&newargv,newargc,newargc+tempargc)) {
					if (newargc > 0) { 
						newargc--;
						disposArgs(&newargc, newargv);
					}
					DisposPtr((Ptr)newargv); 
					DisposPtr((Ptr)tempargv);
					return(1);
				}
				while (j < tempargc){
				 	newargv[newargc+j] = NewPtr(strlen(tempargv[j]) + 1);
					strcpy(newargv[newargc+j],tempargv[j]);
					j++;
				}
				newargc = newargc+tempargc; 
				disposArgs(&tempargc, tempargv);
				tempargc=0;
			} else { 
				if (newargc > 0) { 
					newargc--;
					disposArgs(&newargc, newargv);
				}
				DisposPtr((Ptr)newargv); 
				DisposPtr((Ptr)tempargv);
				if (egerr == 0) showDiagnostic(nomatch);
				/* egerr == -1; out of memory */
				return(1);	
			} 
		 } else { 
		 	if (stripQuotes(&(*argvp)[i])) {
	 			if (newargc > 0) { 
					newargc--;
					disposArgs(&newargc, newargv);
				}
				DisposPtr((Ptr)newargv); 
				DisposPtr((Ptr)tempargv);
		 		return(1);
		 	}
		 	if (checkArgvSize(&newargv,newargc,newargc+1)) {
				if (newargc > 0) { 
					newargc--;
					disposArgs(&newargc, newargv);
				}
				DisposPtr((Ptr)newargv); 
				DisposPtr((Ptr)tempargv);
				return(1);
			}
		 	len = strlen((*argvp)[i]);
			newargv[newargc] = NewPtr(len + 1);
			strcpy(newargv[newargc],(*argvp)[i]);
			newargc++; 
		}
		i++;	
	}
	
	newargv[newargc] = NewPtr(1);
	*(newargv[newargc]) = 0;
	*argcp = newargc;
	i = 0;

	disposArgs(&argc, *argvp);
	DisposPtr((Ptr)*argvp);
	
	*argvp = newargv;
	*argcp = newargc;
	
	DisposPtr((Ptr)tempargv);
	return(0);
}



short stripQuotes(anArg)
	char		**anArg;
/*   
 * Take any surrounding or embedded quotes off of an argument. 
 * We should check for matched quotes here because even though "parseLine" has already
 * done it, the various substitutuion mechanisms could re-introduce mismatches.
 */
{
	short	len,i=0,j=0,quote;
	char	*newarg;
	char	*mismatch="Mis-matched quotes.\r";
	
	len = strlen(*anArg);
	newarg = (char *)NewPtr(len+1);
	
	while ((*anArg)[i] != '\0') {
		if (((*anArg)[i] == SQ) || ((*anArg)[i] == DQ)) {
			quote = (*anArg)[i]; i++;
			while (((*anArg)[i] != quote) && ((*anArg)[i] != 0)) {
				newarg[j] = (*anArg)[i];
				i++;j++;
			}
			if ((*anArg)[i] == 0 ) {
				showDiagnostic(mismatch);
				return(1);
			}
			i++;
		} else {
			newarg[j] = (*anArg)[i];
			i++;j++;
		}
	}
	newarg[j]=0;
	DisposPtr((Ptr)*anArg);
	*anArg = newarg;
	return(0);

}

short pickOutCmdLine(tempText,cmd)
	TEHandle	tempText;
	char		*cmd;
{  /* cmd is assumed to be a pointer to at least 255 characters */
	Handle	tempTH;
	short	i,j;
	char	*err = "Command line too long. \r";

	tempTH = (Handle)(*tempText)->hText; 
	HLock((Handle)tempTH);
	i = ((*tempText)->teLength -1); 	/* get the TE Record length */
	while ((*tempTH)[i] != 13) 			/* go back to beginning of line */
		i--;
	i = i + strlen(PROMPT); 			/* add the length of the prompt */
	i++; 								/* add one for the return char itself */
	j = ((*tempText)->teLength) - i; 	/* get the length of the string*/
	if ((j > 0) && (j < 254)) {
 		BlockMove(&((*tempTH)[i]),cmd, (long)j); 	/* extract the command line */	
 	} else {
 		if (j >= 254)
 			showDiagnostic(err);
 		j = 0;
 	}
 	HUnlock((Handle)tempTH); 					
 	cmd[j] = 0;					
}	 		

pasteToInteractive(theText)
	TEHandle	theText;
{ /* Get the text in the scrap one line at a time.  Paste each line, and pass it to 
   * doCallCmd. 
   */
   	Handle		TEScrap;
   	long		len,i,j,len2;
   	char		*temp,cmd[256];
   	short		clen, pass1 = TRUE;
   	
   	TEScrap = TEScrapHandle();
   	len = TEGetScrapLen();
   	/* if it doesn't contain a return, we'll just do a normal paste */
   	i=0L;
   	while ((i < len) && ((*TEScrap)[i] != RET)) i++;
   	if (i==len) {
   		TEPaste(theText);
   		return;
   	}
   	
   	/* if it has returns, do the fancy stuff */
   	
   	/* find out what's on the command line now */
   	pickOutCmdLine(theText,cmd);
	len2 = len + strlen(cmd) + 1;
	
	if (len2 < 2048L) 
   		temp = NewPtr(LoWord(len2)); 
   	else
   		temp = NewPtr(2048); /* 2k maximum line length */
   	i = j = 0L;
   	strcpy(temp,cmd);
   	j=clen=strlen(cmd);
   	
   	while (i != len) {
   		temp[j] = (*TEScrap)[i];
   		if ((temp[j] == RET) || (j == 2046L)) {
   			if (pass1) {
   				TEInsert(&(temp[clen]),j-clen+1,theText);
   				pass1 = FALSE;
   			} else {
   				TEInsert(temp,j,theText);
   			}
   			if (j == 2046L) j++;
   			temp[j] = 0;
			doCallCmd(theText,&temp);
			j = 0L;
		} else {
			j++;
		}
		i++;
	}
	TEInsert(temp,j,theText);
	DisposPtr((Ptr)temp);
}



chopAtReturn(str)
	char	*str;
{ /* look through the string.  When we hit a return, replace it with NULL. */
	int		len,i;
	
	len = strlen(str);
	i = 0;
	while ((str[i] != RET) && (i != len)) i++;
	str[i] = 0;
}

chopAtPound(str)
	char	*str;
{ /* look through the string.  When we hit a #, replace it with NULL. 
   * As special cases, don't do anything if the character that preceeds the 
   * '#' is '$', or if the characters that preceed the '#' are '{' and '$'.  
   * (to make shell variables work correctly) */
	int		len,i,done;
	
	len = strlen(str);
	i = 0; done = FALSE;
	
	while ((i < len) && (!done)) {
		while ((str[i] != '#') && (i < len)) i++;
		if (str[i] == '#') {
			if (i == 0) {
				str[i] = 0;
				done = TRUE;
			} else {
				if (str[i-1] == '$') {
					i++;
				} else if ((i>1) && (str[i-1] == '{')) {
					if (str[i-2] == '$') {
						i++;
					} else {
						str[i] = 0;
						done = TRUE;
					}
				} else {
					str[i] = 0;
					done = TRUE;				
				}					
			}			
		}
	}
}

int	checkInterrupt()
{	/* Return true if Command-Period is next in the event queue. */
	int		i,gotone;
	char	c;
	EventRecord		myevent;
	char	*msg = "Cancel.\r";
	gotone = FALSE;
	
	GetNextEvent(keyDownMask, &myevent);
	c = myevent.message & charCodeMask;
	if ((myevent.modifiers & cmdKey) && (c == PERIOD)) {
		gotone = TRUE;
		showDiagnostic(msg);
	}
	return(gotone);	
}


disposStdioMem(stdiop)
	StdioPtr	stdiop;
{
	DisposPtr((Ptr)stdiop->inBuf);
	DisposPtr((Ptr)stdiop->outBuf);
	DisposPtr((Ptr)stdiop);
}

short	setStdioDefaults(stdiop)
	StdioPtr	stdiop;
{
	TEHandle	teh;
	char		*memerrmsg = "Low memory error. \r";
	
	teh = unpackTEH(interactiveWin);
	
	if ((!(stdiop->inBuf = NewPtr(4096))) ||
		(!(stdiop->outBuf = NewPtr(4096)))) {
		showDiagnostic(memerrmsg);
		return(1);
	}
	
	stdiop->inSize = 4096;
	stdiop->outSize = 4096;
	stdiop->inPos = 0;
	stdiop->outPos = 0;
	stdiop->inTEH = teh;
	stdiop->outTEH = teh;
	stdiop->inType = 1;  /* 1 -> TE Rec */
	stdiop->outType = 1;  /* 1 -> TE Rec */
	stdiop->inRefnum = 0; 
	stdiop->outRefnum = 0; 
	stdiop->inPBP = 0L; 
	stdiop->outPBP = 0L; 
	(stdiop->inName)[0] = 0; 
	(stdiop->outName)[0] = 0;
	stdiop->inInherit = FALSE;
	stdiop->outInherit = FALSE; 
	stdiop->inLen = 0;
	stdiop->outLen = 0;

	return(0);

}

int callUtility(stdiop,argcp,argv)
	int			*argcp;
	char		**argv;
    StdioPtr	stdiop;
{	/* This is the point from which all utilities are dispatched.
     * It could be much more efficient if we use a hash table.  
     * It should return  true/false 
  	 * This is called by  findExec, and handleAllLines */
  	int			argc, err = 0;
  	char		*error = "Command not found.\r";
  	char		*himsg = "Hello there!\r";
	CursHandle		watchH;

	watchH = GetCursor(watchCursor);

  	argc = *argcp; 
	if (strcmp(argv[0], "exit") == 0) {
		closeAndQuit();
	} else if (strcmp(argv[0], "hi") == 0) { 
		SOInsert(stdiop, himsg, strlen(himsg));
	} else if (strcmp(argv[0], "cd") == 0) { 		/* cd */
		doCd(argc, argv, stdiop);
	} else if (strcmp(argv[0], "pwd") == 0) {  		/* pwd */
		getWD(stdiop); 
	} else if (strcmp(argv[0], "ls") == 0) {  		/* ls */
		doLs(argc, argv, stdiop);
	} else if (strcmp(argv[0], "cat") == 0) {  		/* cat */
		SetCursor(*watchH);
		doCat(argc, argv, stdiop); 
		SetCursor(&arrow);
	} else if (strcmp(argv[0], "seg") == 0) {  		
		doSeg(argc, argv, stdiop); 
	} else if (strcmp(argv[0], "xn") == 0) {  		
		doXnull(argc, argv, stdiop); 
	} else if (strcmp(argv[0], "egrep") == 0) {  	
		SetCursor(*watchH);
		doEgrep(argc, argv, stdiop);
		SetCursor(&arrow);  
	} else if (strcmp(argv[0], "cp") == 0) {  		
		SetCursor(*watchH);
		doCp(argc, argv, stdiop); 
		SetCursor(&arrow);
	} else if (strcmp(argv[0], "set") == 0) {  		/* set */
		doSet(argc, argv, stdiop); 
	} else if (strcmp(argv[0], "unset") == 0) {  	/* unset */
		doUnset(argc, argv, stdiop); 
	} else if (strcmp(argv[0], "history") == 0) {  	/* history */
		doHistory(argc, argv, stdiop); 
	} else if (strcmp(argv[0], "unalias") == 0) {  	/* unalias */
		doUnalias(argc, argv, stdiop); 
	} else if (strcmp(argv[0], "alias") == 0) {  	/* alias */
		doAlias(argc, argv, stdiop); 
	} else if (strcmp(argv[0], "mv") == 0) {  		/* mv */
		doMv(argc, argv, stdiop); 
	} else if (strcmp(argv[0], "rm") == 0) {  		/* rm */
		doRm(argc, argv, stdiop); 
	} else if (strcmp(argv[0], "mkdir") == 0) {  	/* mkdir */
		doMkdir(argc, argv, stdiop); 
	} else if (strcmp(argv[0], "mem") == 0) {  		/* show heap data */
		doMem();
	} else if (strcmp(argv[0], "free") == 0) { 		/* free space */
		doFree();
	} else if (strcmp(argv[0], "compact") == 0) {  	/* compact and report largest contiguous */
		doCompact();
	} else if (strcmp(argv[0], "grep") == 0) {  	/* grep */
		SetCursor(*watchH);
		doGrep(argc, argv, stdiop);
		SetCursor(&arrow);
	} else if (strcmp(argv[0], "find") == 0) {  	/* find file */
		SetCursor(*watchH);
		err = doFind(argc, argv, stdiop);
		SetCursor(&arrow);
	} else if (strcmp(argv[0], "more") == 0) {  	/* more */
		doMore(argc, argv, stdiop);
	} else if (strcmp(argv[0], "head") == 0) {  	/* head */
		err = doHead(argc, argv, stdiop);
	} else if (strcmp(argv[0], "tail") == 0) {  	/* tail */
		err = doTail(argc, argv, stdiop);
	} else if (strcmp(argv[0], "man") == 0) {  	 	/* man */
		doMan(argc, argv, stdiop);
	} else if (strcmp(argv[0], "help") == 0) {  	/* help is the same as man */
		doMan(argc, argv, stdiop);
	} else if (strcmp(argv[0], "test") == 0) {  	/* misc testing */
		doTest(argc, argv, stdiop);
	} else if (strcmp(argv[0], "echo") == 0) {  	/* echo */
		doEcho(argc, argv, stdiop);
	} else if (strcmp(argv[0], "chtype") == 0) {  	/* change type */
		err = doChtype(argc, argv, stdiop);
	} else if (strcmp(argv[0], "chcreator") == 0) {  	/* change creator */
		err = doChcreator(argc, argv, stdiop);
	} else if (strcmp(argv[0], "bintohex") == 0) {  	/* bin to hex */
		err = doBintohex(argc, argv, stdiop);
	} else if (strcmp(argv[0], "source") == 0) {
		err = doSource(argc,argv,stdiop);
	} else if (strcmp(argv[0], "rehash") == 0) {
		err = doRehash(argc,argv,stdiop);
	} else if (strcmp(argv[0], "which") == 0) {
		err = doWhich(argc,argv,stdiop);
	} else if (argc > 0) { /* if all else fails, try external code resource */
		err = callExtCmd(argc, argv, stdiop); 
	}
	return(err);
}

pstrcpy( s1, s2 )
register char *s1, *s2;
{
	register int length;
	
	for (length=*s2; length>=0; --length) *s1++=*s2++;
}

pstrcat(s1, s2)
register char *s1, *s2;
{
	register char *p;
	register short len, i;
	
	if (*s1+*s2<=255) {
		p = *s1 + s1 + 1;
		*s1 += (len = *s2++);
	}
	else {
		*s1 = 255;
		p = s1 + 256 - (len = *s2++);
	}
	for (i=len; i; --i) *p++ = *s2++;
}

closeAndQuit()
{
/* close windows, then set done to true */
	WindowPtr	window;
	
	while(isAppWindow(window = FrontWindow())) {
		if ((window == interactiveWin) || (window == diagnosticWin)) {
			HideWindow(window);
		} else {
			closeScript(window);
		}
	}
	/* do anything special with interactive and diagnostic windows? */
	gDone = TRUE;
}

HandleInteractiveCmd(Commandh,tempText)
	char		**Commandh;
	TEHandle	tempText; /* text record for the interactive window.*/
{	
/* This function takes a command line, performs pre-parse (backquote substitution)
 * and interactive-only (history) operations, then parses the line, and passes
 * it along with a null but fully allocated stdio rec on to the common
 * (interactive/script) line handling function.  
 * This function will be used only by the interactive mode.
 * The next function down the line will handle globbing, redirection of stdio, 
 * and call utilities.
 */
	char		**argv;
	int			argc, mismatch, err;
    StdioPtr	stdiop;
	char		*mismatchmsg = "Mis-matched Quotes.\r";

    /* command history:  */
	if(recallHistory(Commandh)) return;
	if(addToHistory(*Commandh)) return;
	
	/* Backquote substitution, both here and in the script handler */
	if(backquoteSubstitute(Commandh)) return;
	
	argv = (char **)NewPtr(ARGVINC*sizeof(Ptr));

	mismatch = parseLine(*Commandh,(short *)&argc,&argv); 	
	if (mismatch) {	
		DisposPtr((Ptr)argv);
		return;
	}
	stdiop	= (StdioPtr)NewPtr(sizeof(StdioRec)); /* storage for the stdio record */
	/* setup default values */
	err = setStdioDefaults(stdiop);
	if (err) return;
	
	err = handleAllLines(&argc,&argv,stdiop); 
	
	
	disposStdioMem(stdiop);	
	disposArgs(&argc, argv);	
	DisposPtr((Ptr)argv);
}



short parseLine(Command,argcp,argvp) /* return=1 -> mis-matched quotes */
	char	*Command, ***argvp;
	short		*argcp;
/* We respect single and double quoted entities as arguments.  
 * The quotes themselves are left alone here.  
 * If not quoted, ">",">>","<", and "|" form seperate arguments,
 * even if they aren't surrounded by whitespace. 
 *
 * In general we want to allow quoted spaces to be embedded
 * in arguments. Something like -d" " should be interpreted as one
 * argument, as is: "-d "
 *
 * To do:
 * We also should pass anything following a backslash unchanged.
 * All of the quoting characters remain part of the argument until
 * we do the globbing.
 *  
 * Globbing and variable and alias substitution routines need to be updated
 * if changes are made here.
 *
 * 12/95: changes:
 * -check for arguments which are longer than MAXARGLEN characters.  Report this
 *  condition.
 * -return short instead of int.
 * -dynamically increase the size of argv to accomodate more args.  Assume 
 *  we come in with space for ARGVINC arguments in argv.
 * -report our own errors.  Return TRUE for any error.
 * 
 */
{	
 char		*temp;							
 int		i,j,argc,quote; 
 char	*toolong = "Argument too long.\r";
 char	*mismatch = "Mis-matched quotes. \r";
 if (!(temp = NewPtr(MAXARGLEN))) {
 	showDiagnostic(gMEMERR);
 	*argcp=0;
 	return(1);
 }
 
 argc = 0;
 i = 0;
 while (Command[i] != 0) {
 	while ((Command[i] == SP) || (Command[i] == TAB))
 		i++; 						/* go past leading spaces and tabs */
 	if (Command[i] != 0) {			/* got one on the line. */
 		argc++;						
 		j = 0;
		if (Command[i] == GT) { /* take one or two consecutive >'s */
 			temp[j++] = Command[i++];
			if (Command[i] == GT)  	temp[j++] = Command[i++];
		} else if  ((Command[i] == LT) || (Command[i] == PIPE)) {
			/* if it's LT, or PIPE, take one character only */
 			temp[j++] = Command[i++];
 		} else {	
			/* it's not one of the special characters, *
			 * so just go to the next space, tab, null, pipe, "<", or ">" 	*/
 			while 	((Command[i] != 0) && (Command[i] != SP) && (Command[i] != TAB) &&
 					(Command[i] != GT) && (Command[i] != LT) && (Command[i] != PIPE)) {
 
 				if ((Command[i] == DQ) || (Command[i] == SQ)) { /* if it's a quote */
 					quote = Command[i];
 					temp[j++] = Command[i++]; 		/* record the quote */
 					while ((Command[i] != 0) && (Command[i] != quote) && (j<(MAXARGLEN-2))) {
 						temp[j++] = Command[i++];
 					}
 				
 					if (j==(MAXARGLEN-2)) { 
 						*argcp = 0;
 						DisposPtr((Ptr)temp);
						if ((argc) > 1) { 
							argc -= 2;
							disposArgs(&argc, *argvp);
						}
 						showDiagnostic(toolong);
 						return(1);
 					}
 					
 					if (Command[i] == 0) {		/* mismatched quotes */
 						*argcp = 0;
 						DisposPtr((Ptr)temp);
						if ((argc) > 1) { 
							argc -= 2;
							disposArgs(&argc, *argvp);
						}
 						showDiagnostic(mismatch); 
 						return(1);
 					} else {
 						temp[j++] = Command[i++];  /* record the closing quote */	
 					}
				}  else { 
					 if (j==(MAXARGLEN-1)){
						*argcp = 0;
						DisposPtr((Ptr)temp);
						if ((argc) > 1) { 
							argc -=2 ;
							disposArgs(&argc, *argvp);
						}
						showDiagnostic(toolong);
						return(1);
					}  
					temp[j++] = Command[i++];
				} 
 			} /* end while looking at a regular character */
 		} /* end if current character isn't one of the special characters */	
 		temp[j] = 0;	/* null terminate it */
 		
 		if (checkArgvSize(argvp,argc,argc+1)) { 
			if ((argc) > 1) { 
				argc -=2;
				disposArgs(&argc, *argvp);
			}
			*argcp = 0;
			return(1);
		} 
 		(*argvp)[argc-1] = NewPtr(j+1);
 		strcpy((*argvp)[argc-1], temp);
 	} /* end if (command[i] != 0) */
  } /* end while (Command[i] != 0) */
  
  if (checkArgvSize(argvp,argc,argc+1)) { 
	if ((argc) > 0) { 
		(argc)--;
		disposArgs(&argc, *argvp);
	}
	*argcp = 0;
	return(1);
  }

  (*argvp)[argc] = NewPtr(1);	
  *((*argvp)[argc]) = 0;	/* per convention */
  *argcp = argc; 
  DisposPtr((Ptr)temp);
  return(0);
}

