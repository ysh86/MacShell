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
#include "script.h"
#include "shellutils.h"
#include "variable.h"
#include "backquote.h"
/* This file contains all the stuff for execution of scripts */

short doSource(argc, argv, stdiop)
short	argc;
char	**argv;
StdioPtr	stdiop;
{
/* This is a wrapper for doRun to use when the 'source' command is called */
	char			*usage = "Usage: source scriptfile [arg1 ...]\r";
	short	result=1; /* true mean error */
	
	if (argc < 2) {
		showDiagnostic(usage);
		return(1);
	}
	result = doRun(argc-1,&(argv[1]),stdiop);
	return(result);
}

short setArgv(argv,argc,stdiop)
char	**argv;
short	argc;
StdioPtr	stdiop;
{
/* argv[0] is the script name. argv[1] and on get mapped to shell variables
 * argv[1] and on.  We'll do this by fudging a call to doSet:
 *    set argv=( <argv[1]> <argv[2]>...)
 */
 	short err;
 	char	*s1 = "set";
 	char	*s2 = "argv=(";
 	char	*s3 = ")";
 	char 	arg1[4];
 	char	arg2[7];
 	char	arg3[2];
 	char 	**tempargv;
 	short	i=1,tempargc;
 	if (argc < 1) return(0);
 	
 	strcpy(arg1,s1); 
 	strcpy(arg2,s2); 
 	strcpy(arg3,s3); 
 	
 	if (!(tempargv = (char **)NewPtr(sizeof(Ptr)*(argc+3)))) return(1);
 	tempargv[0]=arg1;
 	tempargv[1]=arg2;
	while (i< argc) {
		tempargv[i+1] = argv[i];
		i++;
	}
	tempargv[argc+1] = arg3;
	tempargv[argc+2] = '\0';
	tempargc = argc+2;
 	err = doSet(tempargc, tempargv, stdiop);
 	DisposPtr((Ptr)tempargv);
	return(err);
}

short markLines(fbuf,flen,lines,lcntp)
char	*fbuf;
short	flen;
char	***lines;
short	*lcntp;
{
/* Store a pointer to the first character in fbuf, and pointers to the 
 * character after each return (except the last) in lines.  We need to 
 * allocate storage for lines here.  Return TRUE if an error occurs. 
 * The maximum line length is 255.  We enforce this here.
 */
	short	csize = 512, cinc = 512;
	short	i = 1,clines = 1,j;
	char	**templines;
	char	*memerr = "Out of memory.\r";
	char	*toolong = "Script line too long. (256 character maximum)\r";
	
	if (!(*lines = (char **)NewPtr((long)(sizeof(Ptr)*csize)))) {
		showDiagnostic(memerr);
		return(TRUE);
	}
	
	(*lines)[0] = fbuf;
	while (TRUE) {
		j = 0;
		while ((j<255) && (fbuf[i] != '\r')) { j++; i++; }
		if (j == 255) {
			showDiagnostic(toolong);
			DisposPtr((Ptr)*lines);
			return(TRUE);
		}
		i++;
		if ( i >= flen ) break;
		if (clines >= csize) {
			csize += cinc;
			if (!(templines = (char **)NewPtr((long)(sizeof(Ptr)*csize)))) {
				DisposPtr((Ptr)*lines);
				showDiagnostic(memerr);
				return(TRUE);
			}
			BlockMove(*lines,templines,(long)((csize-cinc)*sizeof(Ptr)));
			DisposPtr((Ptr)*lines);
			*lines = templines;	
		} 
		(*lines)[clines] = fbuf+i;
		clines++;	
	}
	*lcntp = clines;
	return(FALSE);
}


int doRun(argc, argv, stdiop)
short	argc;
char	**argv;
StdioPtr	stdiop;
{
/* 
 * First open the file specified by argv[1] and read all the lines 
 * into a buffer (Maximum 32k).  This is to ease the process of
 * performing look-ahead and look-back.  This of course limits the 
 * size of files we can use for scripts.  Next find the beginning of each
 * line and save the pointers in a list.
 *  
 * Then begin processing lines.
 * Each line of the script inherits a copy of the stdio rec passed to run.
 * However, it may be changed if the line directs it's own stdio elsewhere.
 * Set the inherit flags in the copy so that files won't be closed inadvertently.
 * Take one line at a time and pass it to the common line handling function.
 * When it comes back, update stdio to reflect changes that may have
 * occurred.
 *
 * This will dispatch to control flow commands
 * such as "if" "while," and "for."  
 * 
 *
 * If linestdiop has its own outBuf(?)  Ramifications:
 *	 	-if the script outType is a pipe, we need to accumulate bytes 
 *       from this buffer.
 *	 	 including outLen, outPos...
 *	 	-what else? 	
 *  
 * 
 */
	char			**lineargv,*fbuf,**lines,*tmpline;
	short			flen,i,lineargc,mismatch,interrupterr,lcnt;
	short			err = FALSE;
	OSErr			theErr;
	StdioPtr		linestdiop; 
	char			*mismatchmsg = "Mis-matched quotes. \r";
	
	
	err = readFile(argv[0],&fbuf,&flen);	
	if (err) return(TRUE);
	
	err = markLines(fbuf,flen,&lines,&lcnt); /* store pointers to line starts */
	if (err) {
		DisposPtr((Ptr)fbuf);
		return(TRUE); 
	}
	
	/* create shell variable $argv. */
	err = setArgv(argv,argc,stdiop);
	if (err) {
		DisposPtr((Ptr)fbuf);
		return(TRUE); 
	}
	
	interrupterr = FALSE;
	/* storage for the new stdio record */
	linestdiop	= (StdioPtr)NewPtr(sizeof(StdioRec)); 
		
	lineargv = (char **)NewPtr(ARGVINC*sizeof(Ptr));
	i=0; 
	while ((i<lcnt) && (!gDone) && (!(interrupterr = checkInterrupt()))) {	
		chopAtReturn(lines[i]);
		chopAtPound(lines[i]);
				
		/* add backquote substitution both here and in handle interactive */
		/* Note that backquoteSubstitute may dispose of the pointer pointed
		 * to by its argument and replace it with another. */
		if(!(tmpline = NewPtr(strlen(lines[i])+1))) {
			err = TRUE;
			break;
		}
		BlockMove(lines[i],tmpline,strlen(lines[i])+1);
		if(backquoteSubstitute(&tmpline)) {
			err = TRUE;
			DisposPtr((Ptr)tmpline);
			break;
		} 
		lines[i] = tmpline;

		mismatch = parseLine(lines[i],&lineargc,&lineargv); 		
		if (mismatch) { 
			err = TRUE;
			DisposPtr((Ptr)tmpline);
			break;
		}					
		BlockMove(stdiop,linestdiop,(Size)sizeof(StdioRec)); 
	   /* Inherit means opened files should not be closed, 
	    * and stdout should be restored for the last segment of a
	    * pipe which occurs in a script.*/
		linestdiop->inInherit = TRUE;
		linestdiop->outInherit = TRUE;			
		if (isCtrlFlowCmd(lineargv)) { 
			err = doCtrlFlowCmd(&i,lines,stdiop);
		} else {
			err = handleAllLines((int *)&lineargc,&lineargv,linestdiop);				
			updateStdio(stdiop,linestdiop);
		}
		disposArgs((int *)&lineargc, lineargv);	
		DisposPtr((Ptr)tmpline);
		if (err) break;
		i++;
	}		
	if (interrupterr) { err = TRUE;}	
	
	DisposPtr((Ptr)fbuf);
	DisposPtr((Ptr)lines);
	DisposPtr((Ptr)(lineargv));
	DisposPtr((Ptr)linestdiop);
	return(err);
}


updateStdio(stdiop,linestdiop)
	StdioPtr stdiop, linestdiop;
{
	/* If outType for the script is one of the types that uses a buffer
	 * which may not be flushed between lines (pipe or backquote types)
	 * then we need to record outLen and outPos to account for bytes
	 * accumulated from the last line.
	 * In the case of a pipe, a temp file may be opened, so we should 
	 * record the fields used to track this as well */
	if ((stdiop->outType == 2) || (stdiop->outType == 3)) {
		stdiop->outLen = linestdiop->outLen;
		stdiop->outPos = linestdiop->outPos;
		/* outType could change from 2 to 3 */
		stdiop->outType = linestdiop->outType;
		/* in case a temp file has been opened for a pipe */
		stdiop->outRefnum = linestdiop->outRefnum;
		stdiop->outPBP = linestdiop->outPBP;
		pstrcpy(stdiop->outName,linestdiop->outName);
	} else if ((stdiop->outType == 5) || (stdiop->outType == 6)) {
		stdiop->outLen = linestdiop->outLen;
		stdiop->outPos = linestdiop->outPos;
		/* outType could change from 5 to 6 */
		stdiop->outType = linestdiop->outType;
	}
	/* in case the buffers changed size and location: 
	 * (they would if there were pipes in the script) */
	stdiop->inSize = linestdiop->inSize;
	stdiop->outSize = linestdiop->outSize;
	stdiop->inBuf = linestdiop->inBuf;
	stdiop->outBuf = linestdiop->outBuf;							
}

short isCtrlFlowCmd(argv)
	char	**argv;
{
/* return true if the first argument is one of the supported control
 * flow commands
 */

	/* if (strcmp(argv[0], "foreach") == 0) {
		return(1);
	} */
	return(0);
	
}

short doCtrlFlowCmd(startp,lines, stdiop) 
	short		*startp;
	char		**lines;
	StdioPtr	stdiop; 
{
/* 
 * Startp is used to index lines.  It points to the index of the beginning 
 * of the control flow construct.  In most cases its value should be set 
 * to the line beyond the end of the construct before we return.
 *
 * If a goto is found in the construct, and the target is outside the range 
 * of the construct, we set *startp to the location
 * to jump to, and return NOError.  If the goto points to a location inside the
 * range of the construct, the task is a little bit trickier.  We'll need to
 * pass a goto flag and target pointer to the appropriate recursive routine.  
 * 
 * Stdiop is the standard io passed to the script.  Copies of this should be
 * handed to each line (with the inherit flag set). and stdio should be updated
 * upon their return.  
 *
 * Return TRUE for any kind of error.
 *
 * strategy:
 *  1. find the appropriate "end" operator, keeping in mind that things
 *   may be nested.  
 *  2. call appropriate recursive routine with range of lines to operate on:
 *     (use a different routine for each type of control flow).
 *     1. check that the rest of first line (expression, etc.)is valid.
 *     2. evaluate expresson as appropriate.
 *     3. make appropriate indexes, etc. and execute lines, checking
 *        each line against the list of control flow commands, and either
 *        going to the next recursive level, or just doing handleAllLines.
 *
 */

	return(TRUE);
}



short readFile(fname,filebufp,lenp)
	char	*fname, **filebufp;
	short	*lenp;
{
/* Open file fname and read it all.  Put a return at the end if
 * there isn't one there already.
 *
 */
 	HParamBlockRec	hpb;
 	char			temp[256];
 	char			*memerr = "Out of memory.\r";
 	char			*toobig = "Script file too big (32k limit).\r";
 	char			*nodf = "Zero length data fork.\r";
	OSErr			theErr;
	long			flen;
	
	theErr = getDFLength(fname, &flen);
	if (theErr) {
		return(TRUE);
	} else if (flen > 32766L) {
		showDiagnostic(toobig);
		return(TRUE);
	} else if (flen == 0L) {
		showDiagnostic(nodf);
		return(TRUE);
	}
		
	temp[0] = strlen(fname);
	BlockMove(fname,temp+1,temp[0]);

	hpb.ioParam.ioCompletion=0L;
	hpb.ioParam.ioNamePtr=(StringPtr)temp; 				
	hpb.ioParam.ioVRefNum=0; 
	hpb.ioParam.ioPermssn=fsRdPerm; 			
	hpb.ioParam.ioMisc = 0L; 					
	hpb.fileParam.ioDirID=0L; 
	theErr = PBHOpen(&hpb, FALSE);
	if (theErr) {
		showFileDiagnostic(temp,theErr); 
		return(TRUE);
	}	
	if (!(*filebufp = NewPtr(flen))) {
		showDiagnostic(memerr);
		PBClose((ParmBlkPtr)&hpb, FALSE);
		return(TRUE);
	}
	hpb.ioParam.ioReqCount = flen;
	hpb.ioParam.ioBuffer = *filebufp;
	hpb.ioParam.ioPosOffset = 0L; 
	hpb.ioParam.ioPosMode = fsFromStart;

	theErr = PBRead((ParmBlkPtr)&hpb, FALSE); 

	if (theErr) {
		showFileDiagnostic(fname,theErr);
		DisposPtr((Ptr)*filebufp);
		return(TRUE);
	}
	
	if ((*filebufp)[hpb.ioParam.ioActCount-1] != '\r') {
		(*filebufp)[hpb.ioParam.ioActCount] = '\r';
		*lenp = hpb.ioParam.ioActCount + 1;
	} else {
		*lenp = hpb.ioParam.ioActCount;
	}
	
	theErr = PBClose((ParmBlkPtr)&hpb, FALSE);
	return(FALSE);

}

short isExecLine(line)
	char	*line;
{
/* Return false if the line consists of nothing but whitespace, or 
 * if the first non-whitespace character is the '#'.  Lines are 
 * assumed to be terminated by nulls.
 */

	short i = 0;
	while ((line[i] == SP) || (line[i] == TAB)) i++;
	if ((line[i] == '#') || (line[i] == 0)) return(FALSE);
	return(TRUE);
}

OSErr	getDFLength(fname, lenp)
	char	*fname;
	long	*lenp;
{
/* Return the logical length of the datafork.  Fname is a C String.
 */
	HParamBlockRec	hpb;
	OSErr			theErr;
	
	CtoPstr(fname);
	hpb.fileParam.ioCompletion = 0L;
	hpb.fileParam.ioNamePtr = (StringPtr)fname;
	hpb.fileParam.ioVRefNum = 0; 
	hpb.fileParam.ioFDirIndex = 0;
	hpb.fileParam.ioDirID = 0L; 
	theErr = PBHGetFInfo(&hpb,FALSE);
	if (!theErr) {
		*lenp = hpb.fileParam.ioFlLgLen;
	} else {
		showFileDiagnostic(fname,theErr);
	}
	myPtoCstr(fname);
	return(theErr);
}





