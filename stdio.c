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
#include "shellutils.h"
#include "stdio.h"


/* This file contains code for most of the standard I/O stuff */


short SOInsert(StdioPtr stdiop,char *str,short len)
/*	StdioPtr	stdiop;
	char		*str;
	short		len; */
{ /* The maximum len is
   * 32767. (the maximum size of len as a signed short).  This is 
   * capable of managing its buffer, flushing it only as necessary.
   * 1. if len < outSize - outPos, write to the buffer, that's it.
   * 2. Otherwise flush and fill until done.
   * What do we use outLen for?
   * 
   * 3/95 revisions: done
   * outType 4: No revisions necessary.
   * outType 5: No revisions necessary.  We'll add the buffer overflow check
   *  and possible buffer resizing in  flushStdout.  
   * outType 6: Error condition, Return error immediately.  
   */
	short			err, inpos, bcount, done;
	long		templ;

	if (stdiop->outType == 6) return(1); /* buffer overflow error */
	
	err = FALSE;
	
	if (len < (stdiop->outSize - stdiop->outPos)) {
		templ = 0L;
		templ += len;
		BlockMove(str,(stdiop->outBuf+stdiop->outPos), (Size)templ);
		stdiop->outPos += len;
		stdiop->outLen += len;
	} else {
		inpos = 0;
		if (stdiop->outPos < stdiop->outSize) {	
			inpos = stdiop->outSize - stdiop->outPos;
			templ = 0L;
			templ += inpos;
			BlockMove(str,(stdiop->outBuf+stdiop->outPos),(Size)templ);
			stdiop->outPos += inpos;
			stdiop->outLen += inpos;
		}
		done = FALSE;
		while ((!done) && (!err)) {
 			err = flushStdout((StdioPtr)stdiop,(short)(stdiop->outLen));		
			if (!err) {
				if ((len - inpos) > (stdiop->outSize - stdiop->outPos)) {
					bcount = stdiop->outSize - stdiop->outPos;
				} else {
					bcount = len - inpos;
				}
				templ = 0L;
				templ += bcount;
				BlockMove(str+inpos,(stdiop->outBuf+stdiop->outPos),(Size)templ);
				stdiop->outPos += bcount;
				stdiop->outLen += bcount;
				inpos += bcount;
				if (inpos == len) {
					done = TRUE;
				}
			}
		}
	} 
	return(err);	
}
	

short flushStdout(StdioPtr stdiop, short length)
/* 	StdioPtr	stdiop;
	short		length; */
{	/* This should not normally be called directly by the utilities. 
	 * Utilities should allocate a separate buffer, then use SOInsert.
	 *
     * If this is called by utilities, the following caveats should be noted:
     * 	1. calling this could cause the outSize and outType to change.
     *	2. calling will not necessarily cause outPos to return to zero.  
     *		
     * This should just dispense with the 'length' argument,
     * and use outLen to show how many bytes to write.  In a few cases we 
     * ignore 'length' and use outLen anyway!
     *
     * The shell will call this once before closing stdio.  Otherwise it will
     * get called by SOInsert when the buffer is full. 
     *
     * Revisions 3/95: TODO
     * outType 4: very much like (the same as?) outType 0.
     * outType 5: We need to check for buffer overflow.  We may be able to 
     *  resize the outBuf too. 
     * outType 6: Error condition -- do nothing, return error.
     *
     */
	ParmBlkPtr		pbp;
	OSErr			theErr;
	char	*myname = (char *)"\pflushStdout";
	char	*str1 = "flushStdout: count written not equal to count requested. \r";
	TEHandle		tempText;
	long			templ;
	short				offset, err;
	
	if ((stdiop->outType == 0) ||  (stdiop->outType == 4)) {	
		/* output redirection */
		pbp = stdiop->outPBP;
		pbp->ioParam.ioReqCount = 0L;
		pbp->ioParam.ioReqCount += length;
		
		theErr = PBWrite(pbp, FALSE);
		
		if (theErr) {
			showFileDiagnostic(myname,theErr);
			return(1);
		} else {
			stdiop->outLen = 0;
			stdiop->outPos = 0;
		}
		if (pbp->ioParam.ioReqCount != pbp->ioParam.ioActCount){
			showDiagnostic(str1);
			return(1);
		}

	} else if ((stdiop->outType == 1) && (length > 0)) {	 /* TE rec */
		if (length > (30*1024)) {
			offset = length - (30*1024);
			adjustTESizeTo(0,stdiop->outTEH);
		} else {
			offset = 0;
			if ((*(stdiop->outTEH))->teLength >= ((30*1024) - length)) { 
	 			adjustTESizeTo(((30*1024) - length), stdiop->outTEH);	
			}
		}
		templ = 0L;
		templ += length - offset;
		TEInsert((stdiop->outBuf+offset), templ, stdiop->outTEH); 
		/* put the bottom of the TErec back in view */
		adjustTEPos((*(stdiop->outTEH))->inPort); 
		stdiop->outLen = 0;
		stdiop->outPos = 0;	
	} else if (stdiop->outType == 2)  {	/* pipe, buffer only */
		flushPipeBuf(stdiop);
	} else if (stdiop->outType == 3)  {	/* pipe, temp file */
		err = flushPipeFile(stdiop);
		if (err) {
			return(1);
		}
	} else if (stdiop->outType == 5)  {	/* buffer only */
		if (err = flushBufOnly(stdiop)) {
			return(1);
		}
	} else if (stdiop->outType == 6)  {	/* buffer overflow error condition */
		return(1);
	}	
	return(0);
}

short flushBufOnly(stdiop)
		StdioPtr	stdiop;
{
/* Flush the 'buffer only' stdout type.  If the buffer isn't full, we do nothing.
 * If it is full, we first try to increase the size of the buffer (up to
 * 32k).  If it's already maximized, or if we encounter a memory allocation
 * problem, we return true and set the 'outType' to 6 indicating
 * buffer overflow error.
 */
 char *tempptr;
 
 if (stdiop->outLen == stdiop->outSize) { /* it's full */
   	if (stdiop->outSize < BUFMAX) {
   		/* try to increase the buffer size */
   		tempptr = stdiop->outBuf;
    	if (!(stdiop->outBuf = NewPtr(BUFMAX))) {
    		stdiop->outBuf = tempptr;
    		stdiop->outType = 6;
    		return(1);
    	} else {
   			BlockMove(tempptr,stdiop->outBuf,(long)stdiop->outSize);
   			stdiop->outSize = BUFMAX;
   			DisposPtr((Ptr)tempptr);
   		}
   	} else { /* already maximized */
 		stdiop->outType = 6;
 		return(1);
 	}
  }
  return(0);
}


restoreStdout(from,to)
	StdioPtr	from,to;
{ /* Copy the appropriate "out" fields.  Use this when we inherit
   * stdout from a script, then proceed to mess with it through the
   * handling of pipes.  Since we use only one outBuf, changes in the
   * buffer location and size are handled in doRun.  */
	pstrcpy(to->outName,from->outName);
	to->outLen = from->outLen;
	to->outType = from->outType;
	to->outPBP = from->outPBP;
	to->outRefnum = from->outRefnum;
	to->outPos = from->outPos;
	to->outInherit = from->outInherit;
	to->outTEH = from->outTEH;

}


	
short closeStdio(stdiop)
	StdioPtr	stdiop;
{
/* close files used for i/o as appropriate.
 * Don't close inherited stdio.  If a pipe used a temp file, close it. */
 /* Assume buffers are already flushed. */
 	closeRedir(stdiop); /* close files used for redirection */
 	closePipeFile(stdiop); /* close temp files used for pipes */
}


closeRedir(stdiop)
	StdioRec	*stdiop;
{	/* Flush and close any non-inherited files used for redirection. 
	 * "Inherited" files are those used to redirect stdio to and from a
	 * script that may have invoked the current utility.
	 *
	 * Revisions 3/95: done
	 * outType 4: Same as outType 0 if not inherited
	 *
	 *
	 */
	ParmBlkPtr		pbp;
	OSErr			theErr;
	char			*myname = (char *)"\pcloseRedir";
	
	if (((stdiop->outType == 0) || (stdiop->outType == 4))
		&& (!stdiop->outInherit))  { 
		pbp = stdiop->outPBP;
		theErr = PBFlushFile(pbp, FALSE);
		if (!theErr)
			theErr = PBClose(pbp, FALSE);
		if (theErr) 
			showFileDiagnostic(myname,theErr); 
		DisposPtr((Ptr)pbp);
	}
	
	if ((stdiop->inType == 0) && (!stdiop->inInherit)) { 
		pbp = stdiop->inPBP;
		theErr = PBFlushFile(pbp, FALSE);
		if (!theErr)
			theErr = PBClose(pbp, FALSE);
		if (theErr) 
			showFileDiagnostic(myname,theErr); 
		DisposPtr((Ptr)pbp);
	} 
 
}


short setupStdio(short *argcp, char **argv, short index, short segs, StdioPtr stdiop,StdioPtr tempstdiop)
/*	short			*argcp;
	char		**argv;
	short			index,segs;
	StdioPtr	stdiop,tempstdiop; */
{
/* 	setup stdio record, and create and open any files used for redirection.
 *  Memory for the record should have been allocated and default values 
 *  should have been set earlier on.
 *  
 *  Stdin:  If we've got more than one segment, and we're not on the first one,
 *  stdin comes from the pipe. The existence of the redirection operator is an error.
 *  Otherwise obey the redirection.
 *
 *  Stdout: If we've got more than one segment, and we're not on the last one, 
 *  stdout goes to the pipe. The existence of a redirection operator here is an error.
 *  Otherwise, obey the redirection.
 *
 * Revisions 3/95:Done
 * outType 4: getStdout and redirStdout need to understand what to do with
 *  ">>".  
 *
 */
 /* ioType values:
	0: file redirect
	1: interactive window
	2: pipe, buffer
	3: pipe, temp file. 
	4: file redirect (append). Stdout only
	5: buffer only (for backquote substitution). Stdout only.
	6: error condition for type 5. stdout only.
	*/

	char			*fname,*tempptr;
	char			*pipeerr = "Error opening pipe. \r";
	OSErr			theErr;
	short				i,j,gotone, err,redirtype;
	
	char		*memerr = "Low memory, can't open pipe. \r";
	char		*ambigredir = "Ambiguous redirection. \r";
	
	gotone = FALSE;
	theErr = FALSE;
	err = FALSE;
	fname=NewPtr(255);
	
	if (getStdin(*argcp, argv, fname)) { /* there's a redirection given */
		if ((segs > 1) && (index != 1)) {
			showDiagnostic(ambigredir);
			err = TRUE;
		} else {
			err = redirStdin(stdiop, argcp, argv, fname);
		}
	} else if ((segs > 1) && (index != 1)) { /* standard input from a pipe */
		if (stdiop->outType == 2) { /* buffer only */
			stdiop->inType = 2;
			if (stdiop->inSize < stdiop->outSize) {
				tempptr = stdiop->inBuf;
   				if ((stdiop->inBuf = NewPtr(BUFMAX))) {
   					stdiop->inSize = BUFMAX;
   					DisposPtr((Ptr)tempptr);
   				} else {
   					stdiop->inBuf = tempptr;
					showDiagnostic(memerr);
					return(1);
   				}
			}
			BlockMove(stdiop->outBuf,stdiop->inBuf,(Size)stdiop->outLen);
			stdiop->inSize = stdiop->outSize;
			stdiop->inLen = stdiop->outLen;
			stdiop->inPos = 0;
			
		} else if (stdiop->outType == 3) { /* temp file */
			stdiop->inType = 3;
			pstrcpy(stdiop->inName,stdiop->outName);
			stdiop->inPBP = stdiop->outPBP;
			(stdiop->inPBP)->ioParam.ioNamePtr = (StringPtr)stdiop->inName;
			stdiop->inRefnum = stdiop->outRefnum;
			stdiop->inPos = 0;
			stdiop->inLen = 0;
			err = openStdinPipe(stdiop); /* set mark to the beginning */
		}
		/* Reset outLen, outType and outPos to the defaults */
		stdiop->outLen = 0;
		stdiop->outPos = 0;	
		stdiop->outType = 1;
	} else {
		/* no explicit standard input. Interactive window is normally the default. 
		 * The record should be left alone, since we may be inheriting  
		 * stdio from a script */
	}


	if (getStdout(*argcp, argv, fname,&redirtype) && (!err)) { /* there's a redirection given */
		if ((segs > 1) && (index != segs)) {
			showDiagnostic(ambigredir);
			err = TRUE;
		} else {
			err = redirStdout(stdiop, argcp, argv, fname,&redirtype);
		}
	} else if ((segs > 1) && (index != segs)) { /* standard output to a pipe */
		stdiop->outType = 2;  /* just use the buffer to begin with */	
	} else {
		/* no explicit standard output. Interactive window is normally the default. 
		 * In the case when we inherit stdout from a script, then proceed to
		 * mess with the record through the handling of various pipe segments,
		 * we need to restore it here. */
		if ((segs > 1) && (index == segs) && (tempstdiop->outInherit == TRUE)) {
			restoreStdout(tempstdiop,stdiop); /* don't touch the buffer */
		}
	}

	DisposPtr((Ptr)fname);
	return(err);
}

int oldredirStdout(StdioPtr stdiop, int *argcp, char **argv, char *fname)
/*	char		**argv;
	int			*argcp;
	StdioPtr	stdiop;
	char		*fname; */
{
	/* Create a file with name "fname", and prepare the stdio Rec 
	 * accordingly.
	 * Return one for error. */
	HParamBlockRec	hpb;
	ParmBlkPtr		pbp;
	OSErr			theErr;
	int				i,j,gotone;
	
	theErr = FALSE;
	
	/* try to create and open the file */	
	CtoPstr(fname);
	hpb.fileParam.ioCompletion=0L;
	hpb.fileParam.ioNamePtr =  (StringPtr)fname; 
	hpb.fileParam.ioVRefNum=0;
	hpb.fileParam.ioDirID=0L;
		
	theErr = PBHCreate(&hpb,FALSE);
		
	if (!theErr) {
		 /* set file info */
		hpb.fileParam.ioFDirIndex = 0;
 		PBHGetFInfo(&hpb,FALSE);
 		hpb.fileParam.ioFlFndrInfo.fdType = textFileType;
 		hpb.fileParam.ioFlFndrInfo.fdCreator = docCreator;
 		hpb.fileParam.ioDirID=0L;
 		PBHSetFInfo(&hpb,FALSE); 

		hpb.ioParam.ioPermssn=fsWrPerm; /* write = 2 */
		hpb.ioParam.ioMisc = 0L; /* use volume buffer */
		theErr = PBHOpen(&hpb, FALSE); 
			
		if (!theErr) {
			pbp = (ParmBlkPtr)NewPtr(sizeof(ParamBlockRec));
			pbp->ioParam.ioCompletion = 0L;
			pbp->ioParam.ioRefNum = hpb.ioParam.ioRefNum;
			pbp->ioParam.ioBuffer = stdiop->outBuf;
			pbp->ioParam.ioPosMode = fsAtMark;
			pbp->ioParam.ioPosOffset = 0L;		

			/* setup stdoutRec */
			/* buffer, size and pos should be set already */
			stdiop->outRefnum = hpb.ioParam.ioRefNum;
			stdiop->outType = 0;  /* 0 -> file */
			/* stdiop->outTEH = 0L; /* no TEH */
			stdiop->outPBP	= pbp;
			pstrcpy(stdiop->outName,fname);
			stdiop->outInherit = FALSE;
						
			/* adjust argc & argv */
			j = (findGT(*argcp, argv));
			DisposPtr((Ptr)argv[j]);
			DisposPtr((Ptr)argv[j+1]);
			while(j+2 <= *argcp) {
				argv[j] = argv[j+2];
				j++;
			}
			*argcp = *argcp - 2;
		}
	} 

	if (theErr) {
		showFileDiagnostic(fname,theErr);
		return(1); 
	}
	return(0);
}



short redirStdout(stdiop, argcp, argv, fname,type)
	char		**argv;
	short			*argcp;
	StdioPtr	stdiop;
	char		*fname;
	short		*type;
{
	/* Create a file with name "fname", and prepare the stdio Rec 
	 * accordingly.
	 * Return one for error.
	 *
	 * Revisions 3/95: To test.
	 * type is 0 for ">" and 1 for ">>". Add append function. If type is 1
	 * and the file doesn't exist, we'll create it anyway.
	 *
	 */
	HParamBlockRec	hpb;
	ParmBlkPtr		pbp;
	OSErr			theErr;
	short				i,j,gotone,new=0;
	long			end=0L;
	
	theErr = FALSE;
	
	
	/* try to create and open the file */	
	CtoPstr(fname);
	hpb.fileParam.ioCompletion=0L;
	hpb.fileParam.ioNamePtr =  (StringPtr)fname; 
	hpb.fileParam.ioVRefNum=0;
	hpb.fileParam.ioDirID=0L;
		
	if (*type==1) {
		/* make sure the file exists */
		hpb.fileParam.ioFDirIndex = 0;
		theErr = PBHGetFInfo(&hpb,FALSE);
		if (!theErr) 
			end = hpb.fileParam.ioFlLgLen;
		hpb.fileParam.ioDirID = 0L;
	}
	if ((*type==0) || (theErr)) {
		theErr = PBHCreate(&hpb,FALSE);
		new=1;
	}
		
	if (!theErr) {
		 /* set file info */
		if (new) {
			hpb.fileParam.ioFDirIndex = 0;
	 		PBHGetFInfo(&hpb,FALSE);
	 		hpb.fileParam.ioFlFndrInfo.fdType = textFileType;
	 		hpb.fileParam.ioFlFndrInfo.fdCreator = docCreator;
	 		hpb.fileParam.ioDirID=0L;
	 		PBHSetFInfo(&hpb,FALSE); 
		} else {
			hpb.fileParam.ioFDirIndex = 0;
	 		hpb.fileParam.ioDirID=0L;			
		}
		hpb.ioParam.ioPermssn=fsWrPerm; /* write = 2 */
		hpb.ioParam.ioMisc = 0L; /* use volume buffer */
		theErr = PBHOpen(&hpb, FALSE); 
			
		if (!theErr) {
			pbp = (ParmBlkPtr)NewPtr(sizeof(ParamBlockRec));
			pbp->ioParam.ioCompletion = 0L;
			pbp->ioParam.ioRefNum = hpb.ioParam.ioRefNum;
			pbp->ioParam.ioBuffer = stdiop->outBuf;

			stdiop->outRefnum = hpb.ioParam.ioRefNum;
			if (*type == 0) {
				stdiop->outType = 0;  /* create file */
				pbp->ioParam.ioPosMode = fsAtMark;
				pbp->ioParam.ioPosOffset = 0L;		
			} else {
				stdiop->outType = 4;  /* append file */
				pbp->ioParam.ioPosMode = fsFromStart;
				pbp->ioParam.ioPosOffset = end;		
			}
			/* stdiop buffer, size and pos should be set already */
			stdiop->outPBP	= pbp;
			pstrcpy(stdiop->outName,fname);
			stdiop->outInherit = FALSE;
						
			/* adjust argc & argv */
			j = (findGT(*argcp, argv));
			DisposPtr((Ptr)argv[j]);
			DisposPtr((Ptr)argv[j+1]);
			while(j+2 <= *argcp) {
				argv[j] = argv[j+2];
				j++;
			}
			*argcp = *argcp - 2;
		}
	} 

	if (theErr) {
		showFileDiagnostic(fname,theErr);
		return(1); 
	}
	return(0);
}

short redirStdin(stdiop, argcp, argv, fname)
	char		**argv;
	short			*argcp;
	StdioPtr	stdiop;
	char		*fname;
{
	/* Open a file with name "fname", and prepare the stdio Rec 
	 * accordingly.
	 * Return one for error. */
	HParamBlockRec	hpb;
	ParmBlkPtr		pbp;
	OSErr			theErr;
	short				j;
	
	theErr = FALSE;
	
	/* Open the file */	
	CtoPstr(fname);
	hpb.fileParam.ioCompletion=0L;
	hpb.fileParam.ioNamePtr =  (StringPtr)fname; 
	hpb.fileParam.ioVRefNum=0;
	hpb.fileParam.ioDirID=0L;
	hpb.ioParam.ioPermssn=fsRdPerm; /* read permission */
	hpb.ioParam.ioMisc = 0L; /* use volume buffer */
	theErr = PBHOpen(&hpb, FALSE); 
			
	if (!theErr) {
		pbp = (ParmBlkPtr)NewPtr(sizeof(ParamBlockRec));
		pbp->ioParam.ioCompletion = 0L;
		pbp->ioParam.ioRefNum = hpb.ioParam.ioRefNum;
		pbp->ioParam.ioBuffer = stdiop->inBuf;
		pbp->ioParam.ioPosMode = fsAtMark;
		pbp->ioParam.ioPosOffset = 0L;
		
		/* need to set these parameters to do SIGetSize */
		pbp->fileParam.ioFDirIndex = 0;
		pbp->ioParam.ioNamePtr = (StringPtr)(stdiop->inName);
		pbp->ioParam.ioVRefNum = 0;

		/* setup stdoutRec */
		/* buffer, size and pos should be set already */
		stdiop->inRefnum = hpb.ioParam.ioRefNum;
		stdiop->inType = 0;  /* 0 -> file */
		/* stdiop->inTEH = 0L; /* no TEH */
		stdiop->inPBP	= pbp;
		pstrcpy(stdiop->inName,fname);
		stdiop->inInherit = FALSE;
						
		/* adjust argc & argv */
		j = (findLT(*argcp, argv));
		DisposPtr((Ptr)argv[j]);
		DisposPtr((Ptr)argv[j+1]);
		while(j+2 <= *argcp) {
			argv[j] = argv[j+2];
			j++;
		}
		*argcp = *argcp - 2;
	} else {
		showFileDiagnostic(fname,theErr);
		return(1); 
	}
	return(0);
}


short findGT(short argc, char **argv)
/*	short		argc;
	char	**argv; */
{
	/* If ">" is in argv, return its index.  
	 * Otherwise return -1. */
	short		i;
	
	i = 0;
	while ( i < argc ) {
		if ((strcmp(argv[i], ">") == 0) || (strcmp(argv[i], ">>") == 0))
			return(i);
		i++;
	}
	return(-1);	

}


short findLT(short argc, char **argv)
/*	short		argc;
	char	**argv;*/
{
	/* If "<" is in argv, return its index.  
	 * Otherwise return -1. */
	short		i;
	
	i = 0;
	while ( i < argc ) {
		if (strcmp(argv[i], "<") == 0)
			return(i);
		i++;
	}
	return(-1);	

}


short getStdout(short argc, char **argv, char *fname,short *typep)
/* If one of argv is ">" and there is still another arg, return the 
 * following arg, else return NULL. 
 *
 * Revisions 3/95: (Done)
 * We need to know what to do with ">>" too.  We should return the type
 * of redirection to the caller.
 * type argument is 0 for ">" and 1 for ">>".  It's only valid if we return
 * true.
 */
/*	char	**argv;
	short		argc,*typep;
	char	*fname; */
{
	short		i;
	
	i = 0;
	while ( i < argc-1 ) {
		if (strcmp(argv[i], ">") == 0) {
			strcpy(fname,argv[i+1]); *typep=0;
			return(TRUE);				
		} else if (strcmp(argv[i], ">>") == 0) {
			strcpy(fname,argv[i+1]); *typep=1;
			return(TRUE);
		}
		i++;
	}
	return(FALSE);
}

short getStdin(short argc, char **argv, char *fname)
/* If one of argv is "<" and there is still another arg, return the 
 * following arg, else return NULL. */
/*	char	**argv;
	short		argc;
	char	*fname;*/
{
	short		i;
	
	i = 0;
	while ( i < argc ) {
		if ((strcmp(argv[i], "<") == 0) && (i < (argc-1))) {
			strcpy(fname,argv[i+1]);
			return(TRUE);
		}
		i++;
	}
	return(FALSE);
}


short SIRead(StdioPtr stdiop, short req)
/*	StdioPtr	stdiop;
	short			req;*/
{ /* Read up to "req" bytes from standard input into inBuf. 
   * inPos gets reset to zero. inLen gets reset to the number of bytes read.  
   * The routine "SIGet" reads into a buffer allocated 
   * by the caller. SIGet calls this routine. 
   * Any "unread" bytes in the buffer are destroyed when this is called.
   * As it stands, we count on the caller not to request more bytes than the
   * input buffer can hold.  Doing so would cause a crash. */
	
	ParmBlkPtr	pbp;
	OSErr		theErr;
	short			err;
	
	if (stdiop->inType == 0) { /* file redirect */
		pbp = stdiop->inPBP;
		pbp->ioParam.ioReqCount = 0L;
		pbp->ioParam.ioReqCount += req;
		theErr = PBRead(pbp,FALSE);
		if ((theErr) && (theErr != -39)) {
			showFileDiagnostic(stdiop->inName,theErr);
			return(TRUE);
		}
		stdiop->inPos = 0;
		stdiop->inLen = LoWord(pbp->ioParam.ioActCount);
	} else if (stdiop->inType == 1) { /* text edit window */
		return(TRUE);
	} else if (stdiop->inType == 2) { /* pipe, buffer */
		stdiop->inPos = 0; /* that's it, no more */
		stdiop->inLen = 0;
	} else if (stdiop->inType == 3) { /* pipe, temp file */
		/* read from the temp file */
		err = readPipeFile(stdiop,req);
	} 
	return(0);
}


short SIGet(StdioPtr stdiop, char *buffer, short req, short *actp)
/*	StdioPtr	stdiop;
	char		*buffer;
	short			req, *actp; */
{ /* Attempt to read "req" bytes from standard input into buffer. Return 
   * the number actually read in actp.  Return true indicates an error.  We're
   * at the end of standard input when req does not equal *actp. */
   
 /* Strategy:  
  *		1.  See if we can satisfy the request from inBuf.
  *		2.  If not, write any unwritten bytes from inBuf to buffer, 
  *			then read up to inSize bytes into inBuf.
  *		3.  Write bytes from inBuf to buffer.
  */

	short		done, err, outpos, bcount;
	long	templ;
	
	err = FALSE;
	templ = 0L;
	
	if (req <= (stdiop->inLen - stdiop->inPos)) {
		/* just take the data from inBuf, then update inPos. and actp */
		templ += req;
		BlockMove((stdiop->inBuf+stdiop->inPos), buffer, (Size)templ);
		*actp = req;
		stdiop->inPos += req;
	} else {
		outpos = 0;
		if (stdiop->inPos < stdiop->inLen) { /* any available bytes in inBuf? */
			/* put inLen-inPos bytes into buffer, update inPos and actp */
			outpos = stdiop->inLen - stdiop->inPos;
			templ = 0L;
			templ += outpos;
			BlockMove((stdiop->inBuf+stdiop->inPos), buffer, (Size)templ);
			stdiop->inPos = stdiop->inLen;		
		}
		done = FALSE;
		while ((!done) && (!err)) {
			/* read up to inSize bytes into inBuf */
			err = SIRead(stdiop,stdiop->inSize);
			if (!err) {
				/* put up to req bytes into buffer. */
				if (stdiop->inLen < (req-outpos)) { /* can't fill it yet */
					bcount = stdiop->inLen;
				} else { /* got enough now */
					bcount = req - outpos;
				}
				templ = 0L;
				templ += bcount;
				BlockMove(stdiop->inBuf,(buffer+outpos),(Size)templ);
				outpos += bcount;
				stdiop->inPos += bcount;
				if ((stdiop->inLen < stdiop->inSize) || (outpos == req))
					done = TRUE;
			}
		}
		*actp = outpos;
	}
	return(err);
}

short SIReadLine(char *buffer, short n, StdioPtr stdiop, short *bcnt)
/*	StdioPtr	stdiop;
	char		*buffer;
	short			n;
	short			*bcnt; */
{ /* Put bytes into "buffer" until the return character is reached, or until
   * the number of bytes equals n-1 or until the end of stdin is reached.  
   * Put a null after the last character.
   * Return 1 if stdin is empty or any other error occured.  
   * Return 0 otherwise.   
   *
   * This much is loosely modeled after fgets.
   *
   * Because Mac lines may contain NULL characters and may not end in returns,
   * this makes it very hard in some cases to reliably determine where lines end.
   * Therefore we'll want to return the byte count as well.
   */
   
	short		done, outpos, empty, err, ret;
	
	err = FALSE;
	ret = FALSE;
	empty = FALSE;
	done = FALSE;
	outpos = 0;
	if (n < 1) return(ret);
	while ((!done) && (!err)) {
		while ((stdiop->inLen > stdiop->inPos) &&
				(outpos < n-1) && 
				((stdiop->inBuf)[stdiop->inPos] != RET)) {
			buffer[outpos++] = (stdiop->inBuf)[(stdiop->inPos)++];
		}
		if (outpos >= n-1) {
			done = TRUE;
		} else if ((stdiop->inBuf)[stdiop->inPos] == RET) {
			done = TRUE;
			buffer[outpos++] = (stdiop->inBuf)[(stdiop->inPos)++];
		} else if ((stdiop->inLen < stdiop->inSize) && (empty)) {
			done = TRUE;
		} 
		if ((!done) && (!err)) {	
			/* read up to inSize bytes into inBuf */
			err = SIRead(stdiop,stdiop->inSize);
			if (err) 
				ret = TRUE;
			if (stdiop->inLen != stdiop->inSize)
				empty = TRUE;
			if ((stdiop->inLen == 0) && (outpos == 0)) {
				ret = TRUE;
				done = TRUE;
			}
		}
	}
	buffer[outpos] = '\0';
	*bcnt = outpos;
	return(ret);
}

long SIGetSize(stdiop)
	StdioPtr	stdiop;
/* determine the total size of the standard input.
 * stdiop->inType may be:
 *	0: file redirect
 *	2: pipe, buffer
 *	3: pipe, temp file. 
 */
{
	
	ParamBlockRec	pb;
	OSErr		theErr;
	
	if ((stdiop->inType == 0) || (stdiop->inType == 3)) { /* file */
		BlockMove(stdiop->inPBP,&pb,(long)sizeof(ParamBlockRec));
		/* pb.fileParam.ioFDirIndex = 0; */
		theErr = PBGetFInfo(&pb,FALSE);
		if ((theErr) && (theErr != -39)) {
			return(0L);
		}
		return(pb.fileParam.ioFlLgLen);
	} else if (stdiop->inType == 1) { /* interactive window */
		return(0L);
	} else if (stdiop->inType == 2) { /* pipe, buffer */
		return((long)(stdiop->inLen));
	}  
	return(0L);

}

