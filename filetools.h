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

/* record of the options for the long form of find */
struct _FindFlagsRec {
	char		rootname[256];	/* root directory for the search */
 	short		printF; 		/* true if -print is found */
 	short		nameF; 			/* true if -name is found */
 	char		name[256]; 		/* name string to match */
 	short		execF; 			/* true if valid -exec is found */
 	short		eargc;			/* argument count for exec */
 	char		**eargv;		/* arg vector for exec */
 	short		typeF;			/* true if -type is found */
 	short		type;			/* type designation */
 	short		ftypeF; 		/* true if -ftype is found */
 	long		ftype;
 	short		fcreatorF; 		/* true if -fcreator is found */
 	long		fcreator;
};
#define FindFlagsRec struct _FindFlagsRec
typedef FindFlagsRec	*FindFlagsPtr;

/* flags for rgrep */
struct _RGFlagsRec {
	short		casein;		/*true if case insensitive*/
	short		notgrep;	/*true if -v is found */
	short		hexinput;	/*true if -h is found */
	char		pattern[256];  /*string to match*/
	short		mfiles;			/* true if there are more than one input files */
};
#define RGFlagsRec struct _RGFlagsRec
typedef RGFlagsRec		*RGFlagsPtr;
	
/* prototypes */

extern	short doChtype(int argc, char **argv, StdioPtr stdiop);
extern	short doChcreator(int argc, char **argv, StdioPtr stdiop);
extern	short doFind(int argc, char **argv, StdioPtr stdiop); 
extern  findRecursive(StdioPtr stdiop, FindFlagsPtr flagsp, long dirID, int volID);
extern	doExec(StdioPtr stdiop, int *eargcp, char **eargv, char *path);

extern	short catFromStdin(StdioPtr stdiop,char *buffer,int bsize);
extern	short doRgrep(int argc, char **argv, StdioPtr stdiop);
extern	short rGrepFile(char *fname,StdioPtr stdiop,RGFlagsPtr flags);
extern	short RgType(StdioPtr stdiop,RGFlagsPtr flags,char *fname,ResType type,int fileid);
extern	short RgRes(StdioPtr stdiop,RGFlagsPtr flags,char *fname,ResType type,Handle rHandle);
extern	short binToHex(Byte *bytestr,short *len,char *hexstr);
extern	byteToHex(Byte *abyte,char *hstr);
extern	short doBintohex(int argc, char **argv, StdioPtr stdiop);
extern	short doHead(int argc, char **argv, StdioPtr stdiop);
extern	short doTail(int argc, char **argv, StdioPtr stdiop);

extern	short prepExecArg(char **execarg, char *path, char *carg);
/* fileTools.c */
extern		doCat(int argc, char **argv, StdioPtr stdiop);
extern		doGrep(int argc, char **argv, StdioPtr stdiop);
extern		int grepMatch(char *mstr, char *line, int ci);
extern		getPath(int vref,long wdref,char *path);
extern		int	getVol(char *vname);
extern		int getDir(char *dname,int vref,long *drefp);
extern		doRm(int argc,char **argv,StdioPtr stdiop);
extern		OSErr removeAFile(char *name);
extern		OSErr removeRecursive(char *dname,int volID,long dirID);
extern		doMv(int argc,char **argv,StdioPtr stdiop);
extern		OSErr moveAndRename(char *fromname, char *toname, char *fromparent,char *toparent);
extern		OSErr moveIt(char *fromname,char *toname);
extern		OSErr renameIt(char *from,char *to);
extern		OSErr getParentInfo(char *path,char *parent,char *last,long *dirIDp); 
