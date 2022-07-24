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
#include "backquote.h"
#include "shellutils.h"
#include "stdio.h"
#include "window.h"

/* This file contains code for the backquote substitution mechanism */

/* Notes about the backquote substitution mechanism:
 * -BQ substitution preceeds argument parsing
 * -Succeeds history in the interactive case
 * -May be recursive (once) (backslashes can hide one set of BQs)
 * -May occur many times on one command line
 * -We define a new 'buffer only' stdout type.
 *  including a way to tell if the buffer overflows.
 * -Main proc is called from two places: HandleInteractiveCmd, doRun.
 *
 * How we proceed:
 * -Determine whether BQ exists on the line.  Return quickly if not.
 * -For each matched set (return error if mismatch):
 *		-Record segment preceeding BQ
 *		-Extract BQ segment, ignoring, but stripping backslash from
 *		  those preceeded by backslash.
 *		-Call ourselves with backquoted segment.
 *		-Parse arguments
 *		-Setup stdio with 'buffer only' outType.
 *		-Call HandleAllLines
 *		-Extract results from stdiop
 *		-record result of execution:
 *			-replace returns with spaces
 *			-append to string
 * -record segment after last BQ
 *
 * 
 * 
 *
 */
 
short backquoteSubstitute(cmdh) 
	char **cmdh;
{
/* This may dispose of the string pointed to by *cmdh, and replace
 * it with a new one.
 * 
 * Return true if there's an error. 
 */
 char	*expp,*buf, **bqargv;
 short	n=0,len,start,tmp,err,buflen=256,bufind=0,bqargc,cmderr,resultlen;
 StdioPtr	bqstdiop;
 char	*mismatch="Mis-matched quotes.\r";
 
 if (!findBQ(*cmdh)) {
 	return(0);
 }
 if (!(buf = NewPtr(buflen))){ 
	showDiagnostic(gMEMERR);
	return(1);
 }
 err = extractBQSeg(&((*cmdh)[n]),&expp,&start,&len);
 while ((!err) && ((len !=0) || (start !=0))) {
 	/* record segment from n to start */
 	if(!copyToBuf(&((*cmdh)[n]),&buf,start-1,&buflen,bufind)) {
				DisposPtr(buf); DisposPtr(expp);
				showDiagnostic(gMEMERR);
				return(1);
	}
	bufind+=(start-1); 

    /* call ourself here */
    if (backquoteSubstitute(&expp)) {
    	DisposPtr(buf); DisposPtr(expp);
    	return(1);
    }
 	
 	/* parse args*/
 	bqargv = (char **)NewPtr(ARGVINC*sizeof(Ptr));
 	if (parseLine(expp,(short *)&bqargc,&bqargv)) { 	
		DisposPtr(buf); DisposPtr(expp); return(1);
	}
	
 	/* setup stdio */
 	if(setupBQStdio(&bqstdiop)) {
 		showDiagnostic(gMEMERR); /* ?? */
 	 	disposArgs((int *)&bqargc, bqargv); DisposPtr((Ptr)(bqargv));
 		DisposPtr(buf); DisposPtr(expp); return(1);
 	}
 	
 	/* call handlealllines. Errors here won't cause abort for now -- maybe should?*/
 	cmderr = handleAllLines((int *)&bqargc,&bqargv,bqstdiop);				
 	
 	/* clean/record results */
 	if ((bqstdiop->outLen) >0) {
 		resultlen = bqstdiop->outLen;
 		cleanBQResults(bqstdiop->outBuf, &resultlen);
 		if(!copyToBuf(bqstdiop->outBuf,&buf,resultlen,&buflen,bufind)) {
					DisposPtr(buf); DisposPtr(expp); disposArgs((int *)&bqargc, bqargv);	
					DisposPtr((Ptr)(bqargv)); disposStdioMem(bqstdiop);
					showDiagnostic(gMEMERR); return(1);
		}
		bufind+=resultlen;
 	}
 	
 	/* clean up a bit */
 	disposArgs((int *)&bqargc, bqargv);	
	DisposPtr((Ptr)(bqargv));
  	DisposPtr(expp);
	disposStdioMem(bqstdiop);
	
  	/* look for another backquoted segment */
 	n+=(start+len+1);
 	err = extractBQSeg(&((*cmdh)[n]),&expp,&start,&len);
 }
 if (!err) {
 	/* record seg from (*cmdh)[n] to end of (*cmdh) */
 	tmp=strlen(&((*cmdh)[n]))+1;
 	if(!copyToBuf(&((*cmdh)[n]),&buf,tmp,&buflen,bufind)) {
				DisposPtr(buf); DisposPtr(expp);
				showDiagnostic(gMEMERR);
				return(1);
	}
	bufind+=tmp; 
 	DisposPtr(*cmdh);
 	*cmdh=buf;
 	DisposPtr(expp);
 	return(0);
 } else { 
 	DisposPtr(buf);
 	return(1);
 }
}

void cleanBQResults(buf,len)
char *buf; 
short *len;
{
/* Replace returns in outBuf with spaces. If the last character is 
 * a return, don't replace it, and reduce *len by one to cause it to be
 * truncated. */
	short j;	
	for(j=0;j<*len;j++){ 
		if (buf[j] == '\r') {
			if (j == *len - 1){
				(*len)--;
			} else {
				buf[j]=' '; 
			}
		}	
	}
}

short setupBQStdio(stdioh)
StdioPtr *stdioh;
{
/* Allocate a stdio record and buffers, and set it up with outType == 5
 * and outInherit == TRUE for backquote substitution.  
 * Return true for memory error.
 */
 	TEHandle	teh;
	
	teh = unpackTEH(interactiveWin);

 	if(!(*stdioh=(StdioPtr)NewPtr(sizeof(StdioRec)))) {
 		return(1);
 	}
 	if ((!((*stdioh)->inBuf = NewPtr(4096))) ||
		(!((*stdioh)->outBuf = NewPtr(4096)))) {
		return(1);
	}
	
	(*stdioh)->inSize = 4096;
	(*stdioh)->outSize = 4096;
	(*stdioh)->inPos = 0;
	(*stdioh)->outPos = 0;
	(*stdioh)->inTEH = teh;
	(*stdioh)->outTEH = teh;
	(*stdioh)->inType = 1;  /* 1 -> TE Rec */
	(*stdioh)->outType = 5;  /* 5: buffer only */
	(*stdioh)->inRefnum = 0; 
	(*stdioh)->outRefnum = 0; 
	(*stdioh)->inPBP = 0L; 
	(*stdioh)->outPBP = 0L; 
	((*stdioh)->inName)[0] = 0; 
	((*stdioh)->outName)[0] = 0;
	(*stdioh)->inInherit = FALSE;
	(*stdioh)->outInherit = TRUE; 
	(*stdioh)->inLen = 0;
	(*stdioh)->outLen = 0;

	return(0);

}

short findBQ(str)
char	*str;
{
	short i=0;
	while ( str[i]!=0 ) {
		if (str[i]=='`')
			return(1);
		i++;
	}
	return(0);	

}

short extractBQSeg(inp,outh,startp,lenp)
	char	*inp;
	char	**outh;
	short	*startp,*lenp;
{ /* Look through inp for matched sets of backquotes. If we find BQs
   * that are preceeded by backslashes, remove the backslashes, but ignore
   * them. Stop looking when we find a matched set.  Return the index of the
   * opening BQ in *startp, and the length of the BQuoted segment in *lenp.
   * If *startp==0 and *lenp==0, this means no more BQ'ed segments were found.
   * Return true if there's a mismatch or memory error.  If err==1, *startp
   * and *lenp should be ignored.
   * Warning: this may cause *inp to get shorter.
   * We return a new pointer in *outh unless we return an error.
   * tmpin: copied into inp at the end of this proc.  This is an exact
   *	copy of inp upto index:*startp+*lenp, with "\`" reduced to "`"
   * bqseg: what we put into *outh before returning.  This is the segment
   * 	between the backquotes.
   */
 char *tmpin,*bqseg;
 char	*bqmm="Backquote mismatch\r";
 short inq=0,n=0,m=0,p,done=0;
 
 if (!(tmpin=NewPtr(strlen(inp)+1))) {
  	showDiagnostic(gMEMERR);
 	return(1);
 }
 if (!(bqseg=NewPtr(strlen(inp)+1))) {
  	DisposPtr(tmpin);
  	showDiagnostic(gMEMERR);
 	return(1);
 }
 
 *startp=0;*lenp=0;
 while((inp[n]!=0) && (!done)){
 	if ((inp[n]=='\\')&&(inp[n+1]=='`')){ 
 	  tmpin[m]=inp[n+1];
 	  n++;
 	} else if (inp[n]=='`'){
 		p=0;
 		tmpin[m]=inp[n]; m++;n++; 		
 		*startp=m;		
 		 while((inp[n]!=0) && (!done)){
 		 	if ((inp[n]=='\\')&&(inp[n+1]=='`')){
 		 		bqseg[p]=tmpin[m]=inp[n+1];n++;
 		 	} else if (inp[n]=='`'){
 		 		*lenp=m-*startp;
 		 		tmpin[m]=inp[n];
 		 		bqseg[p]='\0';
 		 		done = 1;
 		 	} else {
 		 		bqseg[p]=tmpin[m]=inp[n];
 		 	}
 		 	n++;m++;p++;
 		 }
 		 if ((inp[n]=='\0') && (!done)) {
 		 	DisposPtr((Ptr)tmpin);
 		 	DisposPtr((Ptr)bqseg);
 		 	showDiagnostic(bqmm);
 		 	return(1);
 		 }
 	} else {
 		tmpin[m]=inp[n];
 	}
 	if((inp[n]!=0)&&(!done)){
 		n++; m++;
 	}
 }
 (*outh) = (char *)bqseg;
 BlockMove(tmpin,inp,(long)m);
 BlockMove(&(inp[n]),&(inp[m]),(long)(strlen(&(inp[n]))+1));
 DisposPtr((Ptr)tmpin);
 return(0);
}

