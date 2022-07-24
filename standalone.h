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

/* DATA STRUCTURES FOR EXTERNAL CODE RESOURCES */

/* A pointer to the struct "MSProcRec" is passed as part of the struct whose 
 * pointer is passed to utilities contained in external code resources. 
 * It contains pointers to the following MacShell functions:
 *	-SIGet
 *	-SIRead 
 *	-SOInsert
 *	-flushStdout
 *	-extGetC
 *  -showDiagnostic
 */
struct _MSProcRec {
	Ptr		SIGet;
	Ptr		SIRead;
	Ptr		SOInsert;
	Ptr		SOFlush;
	Ptr		GetC;
	Ptr		PrintDiagnostic;
};
#define MSProcRec struct _MSProcRec
typedef	MSProcRec	*MSProcPtr;


/* A pointer to the struct "extArg" is passed to utilities contained in
 * external code resources.  It contains the following data:
 *	-Pointer to Standard IO struct as defined above
 *	-argc (int)
 *	-argv (char**)
 *	-pointer to "MSProcs" struct as defined above
 */
struct _ExtArgRec {
	StdioPtr	Stdiop;
	MSProcPtr	MSProc;
	int			argc;
	char		**argv;
};
#define ExtArgRec struct _ExtArgRec
typedef	ExtArgRec	*ExtArgPtr;

/* Globals */
extern MSProcRec	gProcRec;		

/* CONSTANTS AND DATA STRUCTURES FOR PATH AND MANPATH */
#define MAXPATH 32  /* maximum number of directories in a path or manpath */
#define MAXEXE 1024 /* maximum number of executables allowed per directory */

/* we'll create one of these for each directory in the path and manpath. */
#define PathRec struct _PathRec 
typedef PathRec	*PathPtr;
typedef PathPtr	*PathHandle;
struct _PathRec {
	Handle		dname;		/* directory name */
	Handle		**listh;	/* handle to list of handles to executables */
	short		relf;		/* flag: true if relative reference */
	short		listc;		/* number of strings in list */
};

/* globals */
extern PathHandle	gPathList[MAXPATH];
extern short		gPathCount;
extern PathHandle	gManpathList[MAXPATH];
extern short		gManpathCount;



/* prototypes for standalone.c */
extern		int checkFInfo(char *fname);
extern		initProcRec(void);
extern		short callExtCmd(int argc,char **argv,StdioPtr stdiop);
extern		showCmdNotFound(char *cmd);
extern		int extGetC(void);
extern		int testExtCmd(ExtArgPtr argPtr);
extern		short callCodeRes(int argc,char **argv,StdioPtr stdiop);

extern		PathHandle newPathElement(void);

extern		short joinPath(char *key,char *dname,char **path);
extern		short lookupRelative(char *key,char *dname);
extern		short hasMagicCookie2(char *fname);

#ifndef TC7
extern		setPath(PathHandle *apath,short *cntp,VarHandle vh,short ispath); 
extern		short buildPathTable(char *dirp,PathHandle ph,short ispath); 
extern		short isExe(char *fname, short volID, long dirID,FInfo finfo);
extern		short hasMagicCookie(char *fname,short volID,long dirID);
extern		clearPath(PathHandle *apath,short *cntp,short ispath);
extern		short doWhich(short argc, char **argv, StdioPtr stdiop);
extern		short doRehash(short argc,char **argv, StdioPtr stdiop);
extern		short lookupKey(char *key,Handle *listp,short listc,short *indp);
extern		short lookupInPath(char *key,PathHandle *pathl,short pathc,char **resulth, short ispath);
#endif
