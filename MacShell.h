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

/* Use this to help ease problems of portability between ThinkC3 and 7. */ 
#ifndef TC7
#define TC7
#endif

/* TC7: This is already defined */
#ifndef TC7
#define ModalFilterProcPtr ProcPtr 
#endif

/* Constants */
#define	MAXSCRIPTS		5 /* maximum number of scripting windows */
#define NIL 0L
/* TC7 NULL is already defined */
#ifndef TC7
#define NULL 0L 
#endif
/* #define TRUE 1L */
#define SBARWIDTH 		16
#define _MC68881_
#define iBeamCursor		1
#define PROMPT			"MacShell> "
#define SPACE			" "
#define	RETURNSTR		"\r"
#define singleSp		0
#define oneandhalfSp	1
#define doubleSp		2
#define SP				32  /* space char */
#define TAB				9
#define RET				13
#define COLON			58
#define POUND			35
#define PERIOD			46
#define PIPE			124
#define GT				62
#define LT				60
#define HYPHEN			45
#define DQ				34 /* double quotes */
#define SQ				39 /* single quote */
#define VERT			2 	/* vertical scroll control */
#define HORIZ			1	/* horizontal scroll control */
#define scrollback		20480 /* scrollback for the interactive window */
#define BUFSIZE			4096 /* assorted buffers */
#define BUFMAX			32767	/* 32k */
/* #define MAXARGS			64 Max. CL args.  Need to get rid of this limit (used in variable.c) */
#define ARGVINC			256 /* increase the size of argv by this increment when necessary */
#define MAXARGLEN		512 /* maximum length of a single argument */
#define docCreator		'MCsh'
#define textFileType	'TEXT'
#define codeFileType	'CODE'
#define MAXWDTH			600 /* maximum right bound of MShell windows */
#define VERSION			"0.54b"
#define gMEMERR			"Out of memory. \r"

/* ioType:
 *	0: file redirect
 *	1: interactive window
 *	2: pipe, buffer
 *	3: pipe, temp file. 
 *  4: append ">>": stdout only (new with 0.54b)
 *  5: buffer only: stdout only (new with 0.54b to support backquote substitution)
 *  6: buffer overflow error condition: stdout only (new with 0.54b)
 * ioInherit tells us whether open files were inherited from a
 * script's stdio so that they are not closed inadvertently. */
struct _StdioRec {
	char			*inBuf, *outBuf;			/* buffer */
	char			inName[255], outName[255];	/* file names */
	int				inSize, outSize; 			/* buffer size */
	int				inLen, outLen;				/* number of bytes to be read or written */
	int				inInherit, outInherit;		/* open files inherited from scripts? */
	int				inPos, outPos;	 			/* insertion point in buffer */
	int				inType, outType;			/* see above */
	int				inRefnum, outRefnum;		/* io ref number for opened files */
	TEHandle		inTEH, outTEH;				/* text edit handle */
	ParmBlkPtr		inPBP,outPBP;				/* parameter block pointer for open files */
};
#define StdioRec struct _StdioRec
typedef StdioRec	*StdioPtr;
typedef StdioPtr	*StdioHandle;


/* 
 * GLOBALS
 */
extern int 			countScriptWins; /* current number of windows */
extern int			totalScripts;	 /* the number created since startup-used in titles */
extern int			gDone;				/* how we know when we're done */
extern int			gSeq;				/* Sequence number used in temp file names */
extern WindowPtr	diagnosticWin,interactiveWin; /* these always exist */
extern CursHandle 	iBeam;
extern MenuHandle	windowMenu;
extern int			gMyResFile;  /* id of my resource file */
extern int			gInProc;	/* true when using inProcEventLoop */
extern TEHandle		gTextH;

/* globals for shell preferences */
extern short		gRemapEsc; /* remap backquote to trigger name completion */
extern short		gTabNC;  /* use tab instead of escape for name completion. */
/* 
 * prototypes for string functions 
 */
char *strcat(char *s1, char *s2);
char *strcpy(char *s1, char *s2); 
char *strncpy(char *s1, char *s2, int n); 
int strlen(char *s);
int strcmp(char *s1, char *s2); 
char *strchr(char *s, int c);
int strncmp(char *s1, char *s2, int n);
int strpos(char *s, char c);

/* 
 * prototype for sprintf 
 */
int sprintf(char *dest, char *fmt, ...);

/* 
 * Prototypes for everything else that doesn't have its own include file.
 */


/* diagnostics.c */
extern		errAlert(char *errStr);
extern		memDeathAlert(char *who);
extern		deathAlert(char *errStr1, char *errStr2);
extern		showDiagnostic(char *message);
extern		doMem(void);
extern		doFree(void);
extern		doCompact(void);
#ifndef TC7
extern		showFileDiagnostic(char *fname, OSErr errRef); 
#endif

/* egrep.c */
extern		doEgrep(int argc,char **argv,StdioPtr stdiop);
extern		dumpToEol(StdioPtr stdiop);
extern		readToEol(HParmBlkPtr hpbp);
extern		int strayNulls(char *buf, int size);
extern		short getIntArg(char ***argvp,short *argcp, short *optindp);
extern		doSeg(int argc, char **argv, StdioPtr stdiop);
extern		doXnull(int argc, char **argv, StdioPtr stdiop);


/* file.c */
extern		doNewScript(void);
extern		int doSaveAs(WindowPtr window);
extern		int doSave(WindowPtr window);
extern		closeScript(WindowPtr window);
extern		doOpen(void);
#ifndef TC7
extern		openFile(char *fname, int vref, short vers, WindowPtr *wptrp); 
#endif

/* more.c */
extern		doMore(int argc, char **argv, StdioPtr stdiop);
extern		stringMore(char *str,StdioPtr stdiop);
extern		morePrompt(void);
extern		removeMorePrompt(void);
extern		int getRows(WindowPtr theWin);
extern		int getCols(WindowPtr theWin);
extern		int fillScreen(char *sbuf, int *sbufposp ,char *buffer, 
					int *bufposp,int h,int w,int bsize,int format);

/* help.c */
extern		doMan(int argc, char **argv, StdioPtr stdiop);

/* init.c */
extern		initialize(void); 
extern		int isExecutable(WindowPtr wp);
extern		runConfig(void);
extern		initPrompt(void);
extern		runStartup(char *fname,int vref);
extern		startupFiles(void);

/* MacShell.c */
extern		doTest(int argc,char **argv,StdioPtr stdiop);
extern		main(void);

/* A few widely used things from stdio.c */

extern		short SOInsert(StdioPtr stdiop,char *str,short len);
extern 		short flushStdout(StdioPtr stdiop,short length);
extern 		short SIRead(StdioPtr stdiop, short req);
extern 		short SIGet(StdioPtr stdiop,char *buffer, short req, short *actp);
extern		short SIReadLine(char *buffer, short n, StdioPtr stdiop, short *bcnt);



