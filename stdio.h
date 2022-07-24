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
 
/* Prototypes for widely used things are in macshell.h */
extern 		restoreStdout(StdioPtr from,StdioPtr to);
extern 		short closeStdio(StdioPtr stdiop);
extern 		closeRedir(StdioPtr stdiop);
extern 		short setupStdio(short *argcp,char **argv,short index,short segs,StdioPtr stdiop,StdioPtr tempstdiop);
extern 		short redirStdout(StdioPtr stdiop,short *argcp,char **argv,char *fname,short *type);
extern 		int oldredirStdout(StdioPtr stdiop,int *argcp,char **argv,char *fname);
extern 		short redirStdin(StdioPtr stdiop,short *argcp,char **argv,char *fname);
extern 		short findGT(short argc, char **argv);
extern 		short findLT(short argc, char **argv);
extern 		short getStdout(short argc, char **argv, char *fname, short *typep);
extern 		short getStdin(short argc, char **argv, char *fname);
extern		short setStdioDefaults(StdioPtr stdiop);
extern		disposStdioMem(StdioPtr stdiop);
extern		long SIGetSize(StdioPtr stdiop);
extern		short flushBufOnly(StdioPtr stdiop);
