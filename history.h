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

/* history.h */
/* This file contains all the command history include stuff */

#define HISTLEN 256

/* globals */
extern	short gHistStart;
extern	short gHistLen;
extern	long gHistIndex;
extern	Handle	gHistList[HISTLEN];
extern	long	gHistCurrent;
extern	char	gHistLast[256];


extern	short addToHistory(char *line);
#ifndef TC7
extern	doHistory(short argc,char **argv,StdioPtr stdiop);
#endif
extern	showHistElem(Handle cmdh,long index,StdioPtr stdiop);
extern	pasteHistPrev(TEHandle tempText);
extern	pasteHistNext(TEHandle tempText);
extern	storeHistLast(TEHandle theText);
extern	replaceCmdLine(TEHandle theText,char *newline);
extern	short recallHistory(char **cmdh);
extern	short findbang(char *str,short	*ind);
extern	short replaceHistExps(char **strh);
extern	short extractHistExp(char *str,short *etypep,short *locp,short *lenp);
extern	short guessHistExpEnd(char *str,short *endp,short *etypep);
#ifndef TC7
extern	Handle resolveHistExp(short etype,char *str,short len);
#endif
extern  Handle getHistPrev(void);
extern	Handle getHistNum(long ind);
#ifndef TC7
extern	Handle getHistStr(char *str,short len);
extern	short histMatch(char *str1,char *str2,short len);
#endif




