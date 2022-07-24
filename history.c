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
#include "history.h"
#include "shellutils.h"

/* Globals for command history */
Handle 	gHistList[HISTLEN];
short	gHistStart;
short	gHistLen;
long	gHistIndex;
long	gHistCurrent;
char	gHistLast[256];

/* This file contains code for command hisory */

/* For now we use a fixed length history list.  
 * gHistLen starts at zero and grows to HISTLEN.
 * During this phase the most recent element is at gHistList[gHistLen-1]
 * After gHistLen reaches HISTLEN, it remains there forever.
 * At that point new entries are added to the list at the position of gHistStart.
 * Now the most recent entry is at gHistList[gHistStart-1], 
 * unless gHistStart is zero, in which
 * case the newest entry is at gHistList[HISTLEN-1].
 * gHistIndex tracks the total number of lines. 
 * It starts at 1 and grows indefinitely.  
 * gHistCurrent and gHistLast are used for the up-arrow/down-arrow 
 * substitution mechanism.  gHistCurrent
 * tracks the index of the currently displayed history entry.  gHistLast 
 * contains whatever entry the user may have typed before the first up-arrow.
 *
 * Supported substitution mechanisms: 
 *   up-arrow/down-arrow should paste previous/next on the command line
 *   !! represents the most recent command
 *   !str represents the most recent command beginning with 'str'.
 *	 !n  where nn is a number should execute entry n from the history list.
 *  
 * 
 */

short addToHistory(line)
	char	*line;
{
/* allocate a handle, copy line to the handle, and place it in the history list.
 * Dispose of old memory, and update pointers as necessary. Return 1 if we run into
 * an error.
 */
 
 	short	len;
 	Handle	temph;
 	
 	len = strlen(line);
 	
 	if(!(temph=NewHandle(len+1))) {
 		showDiagnostic(gMEMERR);
 		return(1);
 	}
 	HLock(temph);
 	BlockMove(line,(*temph),(long)len+1);
 	HUnlock(temph);
 	
 	if(gHistLen < HISTLEN) {
 		gHistList[gHistLen] = temph;
 		gHistLen++;
 	} else {
 		DisposHandle(gHistList[gHistStart]);
 		gHistList[gHistStart]=temph;
 		if (gHistStart == HISTLEN - 1) gHistStart=0;
 		else gHistStart++;
 	}
 	gHistIndex++;	
 	gHistCurrent = gHistIndex;
	return(0);
}


doHistory(argc,argv,stdiop)
	int			argc;
	char		**argv;
	StdioPtr	stdiop;
{
/* Spit out the contents of the history list from least recent to most recent.
 * Put a number in front of each entry.
 */
 long	index;
 short	i;
 
 if (gHistLen < HISTLEN) {
 	index = 1L;
 	/* report from 0 to gHistLen-1 */
 	i=0;
 	while (i<gHistLen) {
 		showHistElem(gHistList[i],index,stdiop);
 		i++;index++;
 	}
 } else {
 	index = gHistIndex-HISTLEN; 	
 	/* report from gHistStart to HISTLEN-1, then from 0 to gHistStart-1 */
 	i = gHistStart;
 	while(i<HISTLEN){
 		showHistElem(gHistList[i],index,stdiop);
 		i++;index++;
 	}
 	i=0;
 	while(i<gHistStart) {
 		showHistElem(gHistList[i],index,stdiop);
 		i++;index++;
 	}
 }
}

showHistElem(cmdh,index,stdiop)
	Handle	cmdh;
	long	index;
	StdioPtr	stdiop;
{
	char	numstr[256],*spaces="   ";
	short	len;
	
	
	NumToString(index,(unsigned char *)numstr);
	SOInsert(stdiop,numstr+1,*numstr);
	SOInsert(stdiop,spaces,3);
	HLock(cmdh);
	len = strlen(*cmdh);
	SOInsert(stdiop,*cmdh,len);
	HUnlock(cmdh);
	SOInsert(stdiop,"\r",1);
}

short histMatch(str1,str2,len)
	char *str1,*str2;
	short	len;
{
	short ind;
	ind=0;
	while(ind<len){
		if (str1[ind]!=str2[ind]) return(0);
		ind++;
	}
	return(1);

}

Handle getHistStr(str,len)
	char *str;
	short	len;
{
/* attempt to locate an item in the history list which begins with
 * a string which matches str, where the characters that matter 
 * end at str[len] (str is not null terminated).
 */
 	short n;
 if (gHistLen < HISTLEN) {
 	n = gHistLen -1;
 	while (n>=0) {
 		HLock(gHistList[n]);
		if(histMatch(str,*(gHistList[n]),len)) {
			HUnlock(gHistList[n]);
			return(gHistList[n]);		
		}
		HUnlock(gHistList[n]);
		n--;
	}
 } else {
	n=gHistStart-1;
 	while (n>=0) {
 		HLock(gHistList[n]);
		if(histMatch(str,*(gHistList[n]),len)) {
			HUnlock(gHistList[n]);
			return(gHistList[n]);		
		}
		HUnlock(gHistList[n]);
		n--;
	}
	
	n=HISTLEN-1;
 	while (n>=gHistStart) {
 		HLock(gHistList[n]);
		if(histMatch(str,*(gHistList[n]),len)) {
			HUnlock(gHistList[n]);
			return(gHistList[n]);		
		}
		HUnlock(gHistList[n]);
		n--;
	}	
 } 
 return(NULL);
}

short recallHistory(cmdh)
	char	**cmdh;
{
/* recall entries from the history list.
 *  Support the following types of expressions:
 *		1. !!    recalls most recent
 *		2. !str  recalls most recent beginning with string 'str'
 *		3. !n    where n is an integer: recalls history index n
 *
 *  These expressions may be embedded anywhere in a command line.
 *  We'll  need to make some guesses as to where the
 *  string ends:
 *   1. !! followed by any character other than ! is replaced.
 *   2. Any of the following characters should terminate 
 *      the string:<space><tab>$!"'<NULL>
 *	 3. Any non-integer character terminates n
 *
 *  
 *  We'll attempt to replace any number of history expressions 
 *  in a single command line.
 *
 *  If we find a valid substitution, replace the commandline with 
 *  the new one, and dispose of the old one.
 *
 *  If we find '!' but no valid substitution, report an error (event not
 *  found), and return TRUE.  If a memory error or other exceptional
 *  condition occurs, return TRUE.  All other cases return FALSE.
 *
 *  Proceed as follows:
 *   if there's a '!' in the command line
 *		check for a match from the history list.
 * 		if a match is found:
 *			replace string
 *			return no error
 *		else
 *			return error
 *	 else
 *	 	return no error
 *
 */
 	short	n;
 	
	if(findbang(*cmdh,&n)) {
		if(replaceHistExps(cmdh)) return(1); 
	}
 	return(0);
}

Handle resolveHistExp(etype,str,len)
	short	etype;
	char	*str;
	short	len;
{
/* Attempt to resolve history expression found in str with length len
 * (not null terminated).  Return NULL if event is not found.
 * If a valid resolution is found, return a handle to it.
 * Possible values of etype:
 *  1: previous entry (!!)
 *  2: previous beginning with str (!str)
 *  3: entry number (!nn)
 */
 	long	num;
 	char	temp[256];
 	
 	if (etype==1) {
 		/* previous entry */
 		return(getHistPrev());
 	} else if (etype ==2) {
 		/* search for string */
 		return((Handle)getHistStr(&(str[1]),len-1));
 	} else if (etype == 3) {
 		/* lookup by number */
 		temp[0] = (unsigned char)(len-1);
 		BlockMove(&(str[1]),&(temp[1]),(long)(len-1));
		StringToNum((unsigned char *)temp,&num);
 		return(getHistNum(num));
 	}
 	/* should never get here */
	return(NULL);

}





short replaceHistExps(strh)
	char	**strh;
{
/* Replace all valid history expressions in *strh with entries from
 * the history list. If we run into an invalid expression (either 
 * symantically invalid, or not corresponding to an entry in the 
 * history list) return TRUE.  Otherwise return FALSE.
 * We already know that at least one '!' was found in the string.
 *
 * Proceed as follows:
 * 1. allocate a buffer.
 * 2. Extract an expression from the string. For each one:
 *		a. store substring preceeding exp. in buffer.
 *		b. resolve expression
 *		c. if valid, store resolved exp. in buffer
 * 3. Store segment trailing last exp in buffer.
 * 4. Allocate new pointer with the appropriate size, copy result to it.
 * 5. Dispose of old memory, return.
 *
 * As we add to the buffer, check to make sure we don't need to 
 * allocate a larger one.
 *
 */

	char	*buf,*temp;
	char	*eventnf = "Event not found.\r";
	short	i=0,bufind=0,buflen=256,etype,loc,len,tlen,rlen;
	Handle	resulth;

	if (!(buf = NewPtr(buflen))){ 
		showDiagnostic(gMEMERR);
		return(1);
	}
	while(extractHistExp(&((*strh)[i]),&etype,&loc,&len)) {
		if (loc+bufind>=buflen) { 
			if(!(doubleBuffer(&buf,&buflen,loc+bufind))) {
				DisposPtr(buf);
				showDiagnostic(gMEMERR);
				return(1);
			}
		}
		BlockMove(&((*strh)[i]),&(buf[bufind]),(long)loc); /* store leading str */
		bufind+=loc;
		/* resolve expression, and if valid, store it.*/
		if (!(resulth = (Handle)resolveHistExp(etype,&((*strh)[i+loc]),len))) {
			DisposPtr(buf);
			showDiagnostic(eventnf);
			return(1);
		} 
		HLock(resulth);
		rlen = strlen(*resulth);
		if (rlen+bufind>=buflen) { 
			if(!(doubleBuffer(&buf,&buflen,rlen+bufind))) {
				DisposPtr(buf);
				showDiagnostic(gMEMERR);
				HUnlock(resulth);
				return(1);
			}
		}
		BlockMove(*resulth,&(buf[bufind]),(long)rlen); /* store resolved expression */
		bufind+=rlen;
		HUnlock(resulth);
		i+=(loc+len); 
	}
	/* store trailing str */
	tlen = strlen(&((*strh)[i])) +1;
	if (tlen+bufind>=buflen) { 
		if(!(doubleBuffer(&buf,&buflen,tlen+bufind))) {
			DisposPtr(buf);
			showDiagnostic(gMEMERR);
			return(1);
		}
	}
	BlockMove(&((*strh)[i]),&(buf[bufind]),(long)tlen);
	
	/* clean up memory */
	tlen += (bufind);
	if (!(temp = NewPtr(tlen))){ 
		DisposPtr(buf);
		showDiagnostic(gMEMERR);
		return(1);
	}
	BlockMove(buf,temp,(long)tlen);
	DisposPtr(buf);
	DisposPtr(*strh);
	*strh=temp;
}





Handle getHistNum(ind)
	long	ind;
{
/* return a handle to the history entry with the given index.  If the index
 * does not point to an item currently in the history list, return NULL
 */
 short j,delta;

 if (gHistLen < HISTLEN) {
 	if ((ind <= gHistLen) && (ind>0L)) {
 		return(gHistList[(short)(ind -1)]);
 	} else return(NULL);
 } else {
 	if((ind<gHistIndex) && (ind>=(gHistIndex-HISTLEN))) {
 		delta = (short)gHistIndex-(short)ind;
 		if ( (j = gHistStart-delta) < 0 ) j=gHistStart-delta+HISTLEN;
 		return(gHistList[j]);
 	} else return(NULL);
 } 
}

Handle getHistPrev()
{
/* return a handle to the previous history entry.  If there isn't one,
 * return NULL.
 */
 short	j;
 
 if (gHistLen>0) {
	if (gHistLen < HISTLEN) {
		j=gHistLen-1;
	} else {
		if ((j = gHistStart-1) < 0) j=(gHistStart-1+HISTLEN);	
	}
	return(gHistList[j]);
 } else {
 	return(NULL);
 }

}



short extractHistExp(str,etypep,locp,lenp)
	char	*str;
	short	*etypep;
	short	*locp;
	short	*lenp;
{
/* Return FALSE if we don't find an exclamation point, or if we don't
 * find a valid expression in str.  Otherwise return TRUE
 * Return the index of the beginning of the expression in 
 * *locp, and the length in *lenp.  Return the expression type in *etypep.
 * Valid types:
 *	0: no expression, or invalid expression.
 *  1: previous entry (!!)
 *  2: previous beginning with str (!str)
 *  3: entry number (!nn)
 */

	short	i=0,end;
	
	while ((str[i]!='!') && (str[i]!='\0')) i++;
	if (str[i]=='\0') {
		*etypep = 0;
		return(0);
	}
	if (guessHistExpEnd(&(str[i]),&end,etypep)) {
		*locp = i;
		*lenp = end;
		return(1);
	} else {
		/* invalid exp */
		*etypep = 0;
		return(0);	
	}
}

short guessHistExpEnd(str,endp,etypep)
	char	*str;
	short	*endp,*etypep;
{
/* str is assumed to point to the first exclamation point of a
 * history expression.  Return FALSE if the expression is invalid.
 * Otherwise return the index of the character just past the end 
 * of the expression (or length of expression), and the 
 * appropriate type flag.
 *
 *  These expressions may be embedded anywhere in a command line.
 *  We'll  need to make some guesses as to where the
 *  string ends:
 *   1. !! followed by any character other than ! is ok.
 *   2. Any of the following characters should terminate 
 *      the string:<space><tab>!<NULL>
 *	 3. Any non-integer character terminates n
 */

	short	i=1;
	
	if(str[i]=='!') { /* !! */
		if (str[i+1]=='!') {
			return(0); /* !!! is invalid */
		}
		*endp = i+1;
		*etypep = 1;	
		return(1);
	} 
	
	if ((str[i] == ' ') || (str[i] == '\t') || (str[i] == '\0')) { 
		return(0); /* ! alone is invalid */
	}
	
	/* If str[1] is an integer, end the expression on the first non-integer.*/ 
	if ((str[i] >= '0') && (str[i] <= '9')) {
		while ((str[i] >= '0') && (str[i] <= '9')) i++;
		*endp = i;
		*etypep = 3;
		return(1);
	}
	
	while ((str[i] != ' ') && (str[i] != '\t') && (str[i] != '\0') 
		&& (str[i] != '!')) i++; /* find the apparent end */
	*endp = i;
	*etypep = 2;
	return(1);
	
}

short findbang(str,ind)
	char	*str;
	short	*ind;
{
/* find the location of an exclamation point in the string str */
	short i=0;
	
	while ((str[i]!='!') && (str[i]!='\0')) i++;
	if (str[i]=='\0') return(0);
	else {
		*ind = i;
		return(1);
	}
}



storeHistLast(theText)
	TEHandle	theText;
{
	short	s,e,l;
	Handle	texth;

	texth = (Handle)(*theText)->hText; /* get a handle to the chars */
	s = e = ((*theText)->teLength);  	/* points to eol */
	while ((*texth)[s] != 13) 			/* go back to beginning of line */
		s--;
	s++; 							/* add one for the return char itself */
	s = s + strlen(PROMPT); 		/* add the length of the prompt */
	l = e-s;
	HLock(texth);
	if (l>255) l = 255;
	BlockMove(&((*texth)[s]),gHistLast,(long)l);
	gHistLast[l]='\0';
	HUnlock(texth);

}


replaceCmdLine(theText,newline)
	TEHandle	theText;
	char	*newline;
{
/* replace contents of current command line with newline */

	short	s,e;
	Handle	texth;

	texth = (Handle)(*theText)->hText; /* get a handle to the chars */
	s = e = ((*theText)->teLength);  	/* points to eol */
	while ((*texth)[s] != 13) 			/* go back to beginning of line */
		s--;
	s++; 							/* add one for the return char itself */
	s = s + strlen(PROMPT); 		/* add the length of the prompt */
	
	TEDeactivate(theText); 
	
	TESetSelect((long)s,(long)e,theText);
	TECut(theText);
	TEInsert(newline, strlen(newline), theText);

	TEActivate(theText);
}

pasteHistPrev(tempText)
TEHandle	tempText;
{
/* if gHistIndex==gHistCurrent, we're on the last line of input (which
 * has not been recorded in the history list yet), so store it in gHistLast.
 * if gHistCurrent == gHistIndex-HISTLEN, we can't go back any more, so do 
 * nothing.  Otherwise decrement gHistCurrent and paste corresponding 
 * history entry on the command line in place of whatever is there.
 */
 long	i;
 short	j;
 
	if ((gHistCurrent==1) || (gHistCurrent == gHistIndex-HISTLEN)) {
		/* do nothing. */
		return;
	} else if(gHistIndex==gHistCurrent) {
		/* store current */
		storeHistLast(tempText);
	}
	i = gHistIndex-gHistCurrent;
	if (gHistLen < HISTLEN) {
		j=gHistLen-i-1;
	} else {
		if ((j = gHistStart-(short)i-1) < 0) j=(gHistStart-(short)i-1+HISTLEN);	
	}

	HLock(gHistList[j]);
	replaceCmdLine(tempText,*(gHistList[j])); 
	HUnlock(gHistList[j]);
	gHistCurrent--;

}


pasteHistNext(tempText)
TEHandle	tempText;
{
/* if gHistCurrent==gHistIndex, we're on the last line.  Do nothing.
 * If gHistCurrent==gHistIndex-1, restore line from gHistLast and increment
 * gHistCurrent.  Otherwise paste history entry from history list to command
 * line and increment gHistCurrent.
 */
 long i;
 short j;
 if (gHistCurrent==gHistIndex) {
 	return;
 } else if (gHistCurrent==gHistIndex-1) {
 	/* restore from gHistLast */
 	replaceCmdLine(tempText,gHistLast);
 	gHistCurrent++;
 	return;
 }
 i = gHistIndex-gHistCurrent;
 if (gHistLen < HISTLEN) {
	j=gHistLen-i+1;
 } else {
	if ((j = gHistStart-(short)i+1) < 0) j=(gHistStart-(short)i+1+HISTLEN);	
 }

 /* paste gHistList[j] */
 HLock(gHistList[j]);
 replaceCmdLine(tempText,*(gHistList[j]));
 HUnlock(gHistList[j]);

 gHistCurrent++;
 
}


