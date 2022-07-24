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

/* alias.h */
/* This file contains all the shell variable include stuff */
#define MAXALIASES 256		/* maximum number of aliases */
#define MAXANAMELEN 64		/* maximum length of alias name */
#define MAXAWORDS 256		/* maximum number of words per alias */

#define AliasRec struct _AliasRec
typedef AliasRec	*AliasPtr;
typedef AliasPtr	*AliasHandle;
struct _AliasRec {
	char		aname[MAXANAMELEN];			/* variable name */
	Handle		listh[MAXAWORDS];			/* list of handles to strings */
	short		listc;						/* number of strings in list */
};

/* globals */
extern AliasHandle	gAliasList[MAXALIASES];
extern short		gAliasCount;

/* prototypes */
extern  short doAlias(int argc, char **argv,StdioPtr stdiop);
extern  AliasHandle newAlias(void);
extern  short insertAlias(AliasHandle aliash);
extern  disposAlias(AliasHandle aliash);
extern  short findAlias(char *aname,short *indexp);
extern  short binAliasSearch(short *lbp,short *ubp,char *target);
extern	short parseAlias(AliasHandle ah,char *astring);
extern	showAliasList(StdioPtr stdiop);
extern	short doUnalias(int argc,char **argv, StdioPtr stdiop); 
extern	short aliasSubstitute(short *argcp,char ***argvp);

#ifndef TC7
extern  distroyAlias(short index); 
extern	showAlias(StdioPtr stdiop,short i);
#endif