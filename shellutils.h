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

/* shellUtils.c */
extern		int expandGlob(char	*oldstr,char ***newargv,int *newargcp);
extern		getTempName(char *name);
extern		int handleAllLines(int *argcp,char ***argvp,StdioPtr stdiop);
extern		extractSeg(int argc, char **argv, int i, int *segargcp, char **segargv);
extern		flushPipeBuf(StdioPtr stdiop);
extern		closePipeFile(StdioPtr stdiop);
extern		int	openStdinPipe(StdioPtr stdiop);
extern		int createPipeFile(StdioPtr stdiop);
extern		int flushPipeFile(StdioPtr stdiop);
extern		readPipeFile(StdioPtr stdiop,int req);
extern		int	countSegments(int argc,char **argv);
extern		short parseLine(char *Command,short *argcp,char ***argvp);
extern		int oldparseLine(char *Command,int *argcp,char **argv);
extern		doCallCmd(TEHandle theText,char **cmdh);
extern		int shortLineError(TEHandle tempText); 	
extern		putUpPrompt(TEHandle tempText);
extern		int lookForGlob(char *anArg);
extern		disposArgs(int *argcp, char **argv);
extern		int doGlob(int *argcp, char ***argvp);
extern		short stripQuotes(char **anArg);
extern		int checkMatch(char *string,char *globstr,int ci);
extern		pasteToInteractive(TEHandle theText);
extern		chopAtReturn(char *str);
extern		chopAtPound(char *str);
extern		int	checkInterrupt(void);
extern		HandleInteractiveCmd(char **Commandh,TEHandle tempText);
extern		int callUtility(StdioPtr stdiop,int *argcp,char **argv);
extern		pstrcpy( char *s1, char *s2 );
extern		pstrcat(char *s1, char *s2);
extern		closeAndQuit(void); 
#ifndef TC7
extern		short checkArgvSize(char ***argvp,short argc,short newargc); 
#endif
extern		int	compareChars(int c1, int c2, int ci);


/* shellUtils2.c */

extern		doEcho(int argc, char **argv, StdioPtr stdiop);
extern		short  getopt(int argc, char **argv, char *str, short *optind);
extern		short isNum(char *str);
extern		doNameCompletion(TEHandle tempText);
extern		short getCurrentArg(char *str);
extern		short getCommonLeader(char **argv,short *argcp); 
extern		short pickOutCmdLine(TEHandle tempText,char *cmd);
extern		short compareStrings(char *a,char *b);
extern		myPtoCstr(char *str);
#ifndef TC7
extern		short inList(char c,char *list); 
extern      short checkNameCompletion(char ch); 
extern		short doubleBuffer(char **bufp,short *buflenp,short min);
extern 		short copyToBuf(char *frombuf,char **tobufh,short bytes,short *buflenp,short tostart);
#endif