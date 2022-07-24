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
#include "dirtools.h"
#include "event.h"
/* This file contains code for "more". More is called directly 
 * by the user, as well as by other MacShell
 * functions.  These two cases require slightly different handling.
 */ 
 
int getRows(theWin)
	WindowPtr	theWin;
{
	TEHandle	tempText;
	Rect		vRect;
	int			lHeight, h;
	
	tempText = unpackTEH(theWin);
	vRect = (*tempText)->viewRect;
	lHeight = (*tempText)->lineHeight;
	h = (vRect.bottom - vRect.top)/lHeight;
	return(h);
}

int getCols(theWin)
	WindowPtr	theWin;
{
	TEHandle	tempText;
	Rect		vRect;
	int			cWidth, w;
	FontInfo	fi;
	
	GetFontInfo(&fi);
	cWidth = fi.widMax;
	
	tempText = unpackTEH(theWin);
	vRect = (*tempText)->viewRect;
	w = (vRect.right - vRect.left)/cWidth;
	return(w);
}

int fillScreen(sbuf, sbufposp ,buffer, bufposp,h,w,bsize,format)
	char	*sbuf, *buffer;
	int		h,w,*sbufposp,*bufposp,bsize,format;
{
	/* if *sbufposp is not 0, figure out how many lines we have already, 
	 * and how many characters are already in the last line.
	 * otherwise initialize linecount and charcount to 0.  
	 * Buffer size is bsize, and sbuf size is h*(w+1). 
	 * Return false if we're done, ie. the buffer is exhausted. */
	int	linecount,i,charcount,formatfailed,stepback;
	
	linecount = 0;
	charcount = 0;
	stepback = w/5;
	if (*sbufposp != 0) {
		i = 0;
		while (i < *sbufposp) {
			if (sbuf[i] == RET) {
				linecount++;
				charcount = 0;
			} else {
				charcount++;
			}
			i++;
		}
	}
	
	while ((linecount < h-2) && (*bufposp < bsize)) {
		sbuf[*sbufposp] = buffer[*bufposp];
		if (sbuf[*sbufposp] == RET) {
			linecount++;
			charcount = 0;
		} else {
			charcount++;
		}
		if ((charcount == (w - 3)) && (buffer[*bufposp + 1] != '\r')) {
		    /* if format is true, step back and try to replace a space with
		     * a return. if format is false, or if it's "unformatable," 
		     * (has no convenient spaces to replace), add a backslash-return */
		    formatfailed = FALSE;
		    if (format) {
		    	i = 0;
		    	while ((sbuf[*sbufposp - i] != SP) && (i <= stepback)) {
		    		i++;
		    	}
		    	if (i == (stepback + 1)) {
		    		formatfailed = TRUE;
		    	} else {
		    		sbuf[*sbufposp - i] = RET;
		    		linecount++;
		    		charcount = i;
		    		/* if this turns out to be the last line, there's an 
		    		 * ugly problem.  The solution is to save the remainder
		    		 * for next time. */
		    		if (linecount == h-2) {
		    			*bufposp = *bufposp - i;
		    			*sbufposp = *sbufposp - i;
		    		}
		    	}
		    }
		    if (!format || formatfailed) {
				sbuf[(*sbufposp)+1] = '\\';
				(*sbufposp)++;
				sbuf[(*sbufposp)+1] = RET;
				(*sbufposp)++;
				charcount = 0;
				linecount++;
			}
		}
		(*sbufposp)++;
		(*bufposp)++;	
	}

	if (*bufposp == bsize) {
		return(0);
	} else {
		return(1);
	}
}

stringMore(str,stdiop)
	char		*str;
	StdioPtr	stdiop;
{
	/* Spit a string of text out to stdout using the "more"
	 * interface if stdout is the interactive window.  Otherwise, just 
	 * put it out there */
	int			len, h, w, done, ch, sbufpos, bufpos, sbufsize, err;
	char		*sbuf;
	char		*memerr = "stringMore: Low memory error.\r";
	
	
	len = strlen(str);
	
	if (stdiop->outType != 1) {
		/* just put it out there */
		SOInsert(stdiop,str,len);
		return;
	}

	/* calculate size of screen buffer and allocate it */
	h = getRows(interactiveWin);
	w = getCols(interactiveWin);		
	sbufsize = h*(w+2);  /* add extra in case we have to add a return, etc.*/
	if (!(sbuf = NewPtr(sbufsize))) {
		showDiagnostic(memerr);
		return;
	}

	done = FALSE;
	bufpos = 0;
	sbufpos = 0;
	while (!done) {
		/* spit out the text, a prompt, and go to inProcEventLoop */
		if (!fillScreen(sbuf, &sbufpos ,str, &bufpos,h,w,len,TRUE))
			done = TRUE;
		if (sbuf[sbufpos - 1] != RET) {
			sbuf[sbufpos] = RET;
		    sbufpos++;
		}
		SOInsert(stdiop,sbuf,sbufpos);
		if (!done) {
			err = flushStdout(stdiop,stdiop->outLen);
			morePrompt();
			ch = inProcEventLoop(FALSE);
			removeMorePrompt();
			if (ch == 'q') done = TRUE;
			sbufpos = 0;
		}
	}
	DisposPtr((Ptr)sbuf);
}

doMore(argc, argv, stdiop)
	int			argc;
	char		**argv;
	StdioPtr	stdiop;
{
	/* To Do:
	 * 	disable menu items: cut, paste, clear.
	 *
	 *  */
	char				*fname, *buffer, *sbuf;
 	WDPBRec				wdpb;
 	HParamBlockRec		hpb;
	OSErr				theErr;
	int					currarg, err, done, bcount, h, w, ch;
	int					sbufpos, sbufsize, bufpos, tmp, bsize;
	int					fileopen, fillerup, lastchunk, ok; 
	
	char		*memerr = "More: Low memory error.\r";
	char		*direrr = "More: Can't view a directory.\r";
	char		*fileerr = "More: Invalid file name.\r";
	char		*sierr = "More: Error reading standard input.\r";
	char		*outerr = "More: Must use interactive window as output.\r";
	
	if (stdiop->outType != 1) {
		showDiagnostic(outerr);
		return;
	}

	if (!(buffer = NewPtr(BUFSIZE))) {
		showDiagnostic(memerr);
		return;
	} 
	
	wdpb.ioNamePtr = (StringPtr)NewPtr(256); 
	fname = NewPtr(256); 
	
	/* calculate size of screen buffer and allocate it */
	h = getRows(interactiveWin);
	w = getCols(interactiveWin);		
	sbufsize = h*(w+2);  /* add extra in case we have to add a return, etc.*/
	if (!(sbuf = NewPtr(sbufsize))) {
		showDiagnostic(memerr);
		return;
	}
	
	if (argc == 1) { 
		/* prepare to read from stdin */
	} else { 
		/* prepare to read from a file */
		wdpb.ioCompletion=0L;
		PBHGetVol(&wdpb,FALSE); 
		currarg = 1;
	}
	
	done = FALSE;
	sbufpos = bsize = 0;
	lastchunk = FALSE;
	fillerup = TRUE;
	fileopen = FALSE;
	
	while (!done) { 
	/* loop once per screenfull of text, unless we need to read again 
	 * in order to fill the buffer.*/
		ok = TRUE;	
		if (argc == 1) {
			/* read from stdin */
			if (fillerup) {
				if (theErr = SIGet(stdiop,buffer, BUFSIZE, (short *)&bsize)) {
					showDiagnostic(sierr);
					return;
				}
				fillerup = FALSE;
				bufpos = 0;
				if (bsize != BUFSIZE) {
					lastchunk = TRUE;
				}
			}		
		} else { 
			/* open and read from a file */
			if ((!fileopen) && (currarg < argc) && (fillerup)){
				if (tmp = isDir(argv[currarg],wdpb.ioWDDirID,wdpb.ioWDVRefNum)  == 1) {
					showDiagnostic(direrr);
					currarg++;
					ok = FALSE;
				} else if (tmp == -1) {
					showDiagnostic(fileerr);
					return;
				} else { /* it's a file */
					strcpy(fname, argv[currarg]);
					CtoPstr(fname); 
	
					hpb.ioParam.ioCompletion=0L;
					hpb.ioParam.ioNamePtr=(StringPtr)fname; 				
					hpb.ioParam.ioVRefNum=0;
					hpb.ioParam.ioPermssn=fsRdPerm; 			
					hpb.ioParam.ioMisc = 0L; 					
					hpb.fileParam.ioDirID=0L;
	
					theErr = PBHOpen(&hpb, FALSE);
					if (theErr) {
						showFileDiagnostic(fname,theErr); 
						return;
					}	
					hpb.ioParam.ioReqCount = BUFSIZE; /* maximum */
					hpb.ioParam.ioBuffer = buffer;
					hpb.ioParam.ioPosOffset = 0L; 
					hpb.ioParam.ioPosMode = 0;
					fileopen = TRUE;
				}
			}
			if ((ok) && (fillerup)) {	
				theErr = PBRead((ParmBlkPtr)&hpb, FALSE); 
				fillerup = FALSE;
				bufpos = 0;
				bsize = hpb.ioParam.ioActCount;
				if (theErr == -39) {
					theErr = PBClose((ParmBlkPtr)&hpb, FALSE);
					fileopen = FALSE;
					currarg++;
					/* buffer[hpb.ioParam.ioActCount] = RET; /* need this? */
				} else if (theErr) {
					showFileDiagnostic(fname,theErr);
					return;
				}
			}
			
			if ((!fileopen) && (currarg >= argc)) {
				lastchunk = TRUE;
			}
			
		}
		
		/* fill sbuf if possible */
		if (!fillScreen(sbuf, &sbufpos ,buffer, &bufpos,h,w,bsize,FALSE) || 
														(bsize == 0)) { 
			if (lastchunk) { 
				done = TRUE;
			} else if ((argc == 1) || (fileopen)) { 
				ok = FALSE;
			} 
			fillerup = TRUE;
		}
		
		if (ok) { /* spit out the text, a prompt, and go to inProcEventLoop */
		    if (sbuf[sbufpos - 1] != RET) {
		    	sbuf[sbufpos] = RET;
		    	sbufpos++;
		    }
			SOInsert(stdiop,sbuf,sbufpos);
			sbufpos = 0;
			err = flushStdout(stdiop,stdiop->outLen);
			if (!done) {
				morePrompt();
				ch = inProcEventLoop(FALSE);
				removeMorePrompt();
				if (ch == 'q') {
					done = TRUE;
					if (fileopen) {
						fileopen = FALSE;
						theErr = PBClose((ParmBlkPtr)&hpb, FALSE);
					}
				}
			}
		}
	}
	
	DisposPtr((Ptr)wdpb.ioNamePtr);
	DisposPtr((Ptr)buffer);
	DisposPtr((Ptr)fname);
	DisposPtr((Ptr)sbuf);
}


morePrompt()
{
	char *prompt = "--More (Type: space to continue, 'q' to quit)--\r";
	TEHandle	tempText;
	int			len;
	long		templ;
	
	tempText = unpackTEH(interactiveWin);
	len = strlen(prompt);

	TEInsert(prompt, len, tempText);
	
	/* Sure would be nice to make this string appear in reverse video */
	/* templ = 0L;
	templ += (*tempText)->teLength; */	
	/* TESetSelect(templ-len, templ-1, tempText); */
			
	adjustTEPos(interactiveWin);
	updateThumbValues(interactiveWin);

}

removeMorePrompt()
{
	TEHandle	tempText;
	long		len,i;
	int			promptlen;
	
	tempText = unpackTEH(interactiveWin);
	len = 0L;
	len += (*tempText)->teLength;
	i = len;
	i--;
	i--;
	while ((*((*tempText)->hText))[i] != RET) {
		i--;
	}

	TEDeactivate(tempText); 
	
	TESetSelect((i+1),len,tempText);
	TEDelete(tempText);

	TEActivate(tempText);
}
