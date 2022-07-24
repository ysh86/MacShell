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

/* This file contains all the shell variable include stuff */
#define MAXVARS 256			/* maximum number of variables */
#define MAXNAMELEN 64		/* maximum length of variable name */
#define MAXWORDS 512		/* maximum number of words per variable */

#define VarRec struct _VarRec
typedef VarRec	*VarPtr;
typedef VarPtr	*VarHandle;
struct _VarRec {
	char		vname[MAXNAMELEN];			/* variable name */
	Handle		listh[MAXWORDS];			/* list of handles to strings */
	short		listc;						/* number of strings in list */
};

/* globals */
extern VarHandle	gVarList[MAXVARS];
extern short		gVarCount;


/* prototypes */
extern	VarHandle newVar(void);
extern	short insertVar(VarHandle varp);
extern	disposVar(VarHandle varh);
extern	short findVar(char *vname,short *indexp);
extern	short doSet(int argc, char **argv, StdioPtr stdiop);
extern	short doUnset(int argc, char **argv, StdioPtr stdiop);
extern  short getValue(char *varname, char ***val, short *valcp);
extern	short binSearch(short *lbp,short *ubp,char *target);
extern	showVarList(StdioPtr stdiop);
extern	short insertIndexedVar(VarHandle vh,short *index,Handle valh); 
extern	short handleSubscript(char *name);
extern	short handleList(VarHandle vh,char *vtmp,char **argv,short *carg,short *argcp);
extern	mergeValue(char **val,short *valc,char ***argvp,short *argcp,short *index);
extern  short makeVarStrings(char *vname,short *index,char ***val,short *valcp);
extern  short getIndex(char *vname,char **index);
extern  short makeCntString(short *found,short *index,short *vind,char ***val,short *valcp);
extern  short makeTFString(short *found,short *index,short *vind,char ***val,short *valcp);
extern	short varSubstitute(short *argcp,char ***argvp);
extern	short getVSegs(char *argstr,char **tempargv,short *tempargcp);
extern	accumulateVal(char ***sumval,short *sumvalcp,char **val,short *valc);
extern	short  guessVarEnd(char *vstr);
extern	stripBrackets(char *vname);
extern	short	findClose(char *cchar,char *ochar,char *str); 
extern	removePrefs(char *vname);
extern  addPrefs(VarHandle vh);
#ifndef TC7
extern	showVar(StdioPtr stdiop,short i); 
extern	distroyVar(short index); 
#endif
extern short unParse(char **val,short valc, char **valstr);
extern checkOddQuote(char *str,short *inquote);
