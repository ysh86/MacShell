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

/* copy.c */
extern		doCp(int argc,char **argv,StdioPtr stdiop);
extern		int countColons(char *str);
extern		int	absolutePath(char *path);	
extern		addColonFront(char *name);
extern		getLastDirName(char *path,char *dir,int volID);
extern		getFName(char *path,char *file);
extern		int copyRecursive(char *fromdir,char *todir,int fromvol,int depth);
extern		deleteDir(char *name);
extern		addColonEnd(char *name);
extern		OSErr createADir(char *name);
extern		int copyAFile(char *from,char *to); 
extern		int		copyFork(int from,int to);
extern		OSErr	openDF(char *name,int *frefp,int read); 
extern		OSErr	openRF(char *name,int *frefp,int read);
extern		OSErr	createFile(char *name,FInfo *fiPtr,long cdate,long mdate);
extern		OSErr	deleteFile(char *name);
extern		OSErr	getInfo(char *fname,FInfo *finfop,long *cdatep,long *mdatep);
extern		int validDir(char *dname, long *dirIDp);
extern		int	defaultVol(void);
extern		defaultDir(long *dirIDp);
