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
#include "shellutils.h"
/* This file contains code for echo, filename completion, getopt, etc. */

doEcho(argc,argv,stdiop)
	short	argc;
	char	**argv;
	StdioPtr	stdiop;
{
/* put argv[1] to argv[argc-1] on standard output */
	short	i;
	
	for (i=1;i<argc;i++) {
		SOInsert(stdiop,argv[i],strlen(argv[i]));
		if (i+1 < argc) SOInsert(stdiop,SPACE,1);
	}
	/* we need a newline here.  If we're operating from a script we
	 * wouldn't get one otherwise */
	SOInsert(stdiop,RETURNSTR,1); 
}

short  getopt(argc, argv, str, optindp)
	int 	argc;
	char	**argv;
	char	*str;
	short		*optindp;
{ /* This uses the optindp to tell how many options have been scanned.
   * (*optindp) should be initialized to zero before the first call.
   *  
   * We assume all options preceed all other arguments.
   *  
   * If options are to be more than one character, or will take
   * their own arguments, the caller will need to modify optind, argc and argv
   * within the case statement.
   *
   * This does no checking against the list. 
   * 
   * We scan ahead to find which arg the current option resides in 
   *     (don't assume one per argument).
   *
   * If we find one which doesn't begin with '-' consider the job done.
   * 
   * When there are no more arguments, we return 0.
   */

	short	ac,i,j;
	i = 0;
	ac = 1;
	
	(*optindp)++;
	while ((ac<argc) && (argv[ac][0] == '-')){
		j = 0;
		while((argv[ac][j] != 0) && (i<(*optindp))) {
			i++;j++;
		}
		if (argv[ac][j] == 0) i--;
		if (i==(*optindp)) break;
		ac++;
	}
	
	if((ac<argc) && (argv[ac][0] == '-')){
		return(argv[ac][j]);
	} else {
		return(0);
	}			
}

short inList(c, list)
char	c;
char	*list;
{
	while (((*list) != c) && ((*list) != '\0')) list++;
	if (*list == '\0') return(0);
	return(1);

}

short isNum(str)
	char	*str;
{
	/* check each character in the string.  verify that it's within the
	 * numeric range. */
	short i=0;
	for (i=0;str[i] != 0;i++) {
		if ((str[i]<'0') || (str[i]>'9'))
			return(0);
	}
	return(1);
}

short checkNameCompletion(ch)
char	ch;
{ 
/* if "ch" should trigger the file name completion mechanism, return true.
 * Do it as quickly as possible, since this is called once for each keydown
 * event.
 */
 	if (( ch != '`' ) && ( ch != 0x1b ) && ( ch != TAB ))
 		return(FALSE);
 	if (!gTabNC) {
 		if (gRemapEsc &&  ch == '`')
 			return(TRUE);
 		if (!gRemapEsc && ch == 0x1b)
 			return(TRUE);
 		return(FALSE);
 	}
 	if ( ch == TAB )
		return(TRUE);
	return(FALSE);
}

doNameCompletion(tempText)
	TEHandle tempText;
{
/* do the file name completion. tempText is already locked.
 * Called from doKeyDown.
 */
 	Handle	tempTH;
 	short	i,j,k,n,tempargc;
 	long	templ;
 	char	**tempargv;
 	char	arr[256];
	char	*err = "Line too long.\r";
		
	tempTH = (Handle)(*tempText)->hText; /* get a handle to the chars */
	HLock((Handle)tempTH);
	i = ((*tempText)->teLength -1); 	/* get the TE Record length */
	while ((*tempTH)[i] != 13) 		/* go back to beginning of line */
		i--;
	i = i + strlen(PROMPT); /* add the length of the prompt */
	i++; 					/* add one for the return char itself */
	j = ((*tempText)->teLength) - i; 	/* get the length of the string*/
	if ((j >= 0) && (j < 254)) {
 		BlockMove(&((*tempTH)[i]), arr, (long)j); 		/* extract the command line */	
 	} else {
 		showDiagnostic(err);
 		j = 0;
 	}
 	HUnlock((Handle)tempTH); 					/* done with tempTH */
 	arr[j] = 0;					

	k = getCurrentArg(arr); /* find beginning of final argument */
		
	if (lookForGlob(&(arr[k]))) { /* don't even try if there's already a glob */
		SysBeep(1);
		return;
	}
	arr[j] = '*';
	arr[j+1] = 0;
	
	tempargv=(char **)NewPtr(ARGVINC*sizeof(Ptr));

	if (expandGlob(&(arr[k]),&tempargv,(int *)&tempargc)) { 
		templ = 0L;
		if (tempargc==1) {
			/* only one result */
			templ += strlen(&(tempargv[0][j-k]));
		} else {
			/* figure how many characters the results have in common */
			n = getCommonLeader(tempargv,&tempargc);
			templ += n - j + k;
			SysBeep(1);
		}
		
		TEInsert(&(tempargv[0][j-k]), templ, tempText);
		disposArgs((int *)&tempargc, tempargv);
	} else { /* no match */
		SysBeep(1);	
	}
	DisposPtr((Ptr)tempargv);
}

short getCommonLeader(argv,argcp)
	char	**argv;
	short	*argcp;
{
/* Count the number of characters from the front of the arguments which are
 * common to all arguments.  Assume at least one argument.  This is case
 * insensitive.
 */

	short 	cnt=0,carg=0;
	
	while (argv[0][cnt] != 0) {
		for(carg=1;carg<(*argcp);carg++) {
			if(!compareChars(argv[carg][cnt], argv[0][cnt], TRUE)) {
				return(cnt);
			}
		}
		cnt++;
	}
	return(cnt);
}


short getCurrentArg(str)
	char	*str;
{
/* Pick last partial argument out of the command string.  This tells the 
 * file name completion mechanism what to work on.
 * Return index into str of the beginning of the final argument.
 * This code should be very similiar to code in parseLine.
 */

 short i,wpos,qpos;
 char	quote;
 i = 0;
 wpos = 0;
 while (str[i] != 0) {
 	while ((str[i] == SP) || (str[i] == TAB))
 		i++; 						/* go past leading spaces and tabs */
 	if (str[i] != 0) {			/* got one on the line. */
 		wpos = i;
 		if ((str[i] == DQ) || (str[i] == SQ)) { /* if it's a quote */
 			quote = str[i++];
 			qpos = i;
 			while ((str[i] != 0) && (str[i] != quote)) {
 				i++;
 			}
 			if (str[i] == 0) {		/* mismatched quotes */
 				return(qpos);
 			} else {
				i++;
 			}
		} else if (str[i] == GT) { /* take one or two consecutive >'s */
			i++;
			if (str[i] == GT)  	i++;
		} else if  ((str[i] == LT) || (str[i] == PIPE)) {
			/* if it's LT, or PIPE, take one character only */
 			i++;
 		} else {	
			/* it's not a quoted arg and not one of the special characters, *
			 * so just go to the next space, tab, null, pipe, "<", or ">" 	*/
 			while 	((str[i] != 0) && (str[i] != SP) && (str[i] != TAB) &&
 					(str[i] != GT) && (str[i] != LT) && (str[i] != PIPE)) {
 				i++;
 			}
 		}		
 	}
  }
  return(wpos);
}

short compareStrings(a,b)
	char	*a,*b;
{
/* compare two strings.  
 *		If a>b, return 1
 *		If a<b, return -1
 *		If a=b, return 0
 */
 	short i = 0,result=0;
	
	while (a[i] != 0 && b[i] != 0) {
		if (a[i] > b[i]) {
			return(1);
		} else if (b[i] > a[i]) {
			return(-1);
		} 
		i++;
 	}
 	if (a[i] == 0 && b[i] != 0) {
 		return(-1);
 	} else if (b[i] == 0 && a[i] != 0) {
 		return(1);
 	}
 
 	return(0);
}

myPtoCstr(str)
	char *str;
{ /* PtoCstr requires different a argument type under ThinkC3 than under
   * TC7.  This is included explicitly to make the code portable.
   */
	short	i=0,j;
	j = *str;
	while(i<j) {
		str[i] = str[i+1];
		i++;
	}
	str[i] = '\0';
}


short doubleBuffer(bufp,buflenp,min)
	char **bufp;
	short	*buflenp;
	short	min;
{
/* Increase buffer size to at least minimum size of min and up to 32k
 * by doubling current size
 * *buflenp as many times as necessary.  Copy current contents to new 
 * buffer, and dispose of the old one.  Return False if we can't allocate
 * enough memory.  Upon successful completion, *buflenp is the size of the 
 * new buffer. 
 */
 short templen;
 char	*tempbuf;
 
 templen = *buflenp;
 while(templen < min){
 	if (templen > 16383) {
 		templen = 32767;
 		break;
 	} else {
 		templen = templen*2;
 	}
 }
 if (!(tempbuf = NewPtr(templen))) {
 	return(0);
 }
 BlockMove(*bufp,tempbuf,(long)(*buflenp));
 DisposPtr(*bufp);
 *bufp = tempbuf;
 *buflenp = templen;
 return(1);
}

short copyToBuf(frombuf,tobufh,bytes,buflenp,tostart)
	char **tobufh,*frombuf;
	short *buflenp,bytes,tostart;
{
/* Safe way to copy 'bytes' bytes from frombuf to &(tobuf[tostart]).
 * if tostart+bytes >= buflen, allocate new buffer, and copy contents
 * from old buffer to new buffer.  Return false for out of memory error.
 */
 
  	if (tostart+bytes>=*buflenp) {
		if(!(doubleBuffer(tobufh,buflenp,tostart+bytes))) {
				return(0);
		}
	}
	BlockMove(frombuf,&((*tobufh)[tostart]),(long)bytes); 
	return(1);
}




