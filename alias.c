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
#include "alias.h"
#include "shellutils.h"

/* This file contains code for shell aliases */
/* 
 * MacShell Aliases
 * We'll use a data structure that's very similiar to that used by
 * shell variables.  That way we can easily adapt a lot of the
 * shell variable code.
 *
 */
AliasHandle	gAliasList[MAXALIASES];
short		gAliasCount;


short doAlias(argc, argv, stdiop)
	int argc;
	char **argv;
	StdioPtr stdiop;
{
/* 
 *   alias					  show the value of all aliases in the list.
 *   alias name 'word list'	  set name and list of value handles
 * 
 */
 	AliasHandle 	ah;
 	Handle		valh;
 	short		i,carg,alen,err,index;
 	char		*atmp;
 	char		*memerr = "Alias:Memory error.\r";
 	char		*usage = "Usage: alias [name 'word list']\r";

	if (argc == 1) {
		/* show alias list */
		showAliasList(stdiop);
		return(0);
	}
	if (argc != 3) {
		showDiagnostic(usage);
		return(1);
	}
		
	ah = newAlias();
	if (!ah) {
		showDiagnostic(memerr);
		return(1);
	}
	
	/* set name, truncate it at 63 characters. */
	alen = strlen(argv[1]);
	if (alen > 63) alen = 63;
	BlockMove(argv[1],((*ah)->aname),(long)(alen));
	((*ah)->aname)[alen] = 0;
	
	err = parseAlias(ah,argv[2]);
	
	if (err) {
		disposAlias(ah);
		return(1);
	} 
	
	err = insertAlias(ah); /* err means we're over the maximum */

	if(err){ 
		disposAlias(ah);
		return(1);
	} 

	return(0);
}




AliasHandle newAlias()
{
/* Create storage for a new alias, and initialize it. If there isn't enough memory 
 * to create the record, return NULL */

	AliasHandle	ah;
	
	ah = (AliasHandle)NewHandle(sizeof(AliasRec));
	if (ah) {
		(*ah)->aname[0] = '\0'; /* null string in the name. */
		(*ah)->listc = 0;
	}
	return(ah);
}

short insertAlias(aliash)
 AliasHandle	aliash;
{
/* insert the given handle at the appropriate place in the list.
 * If an alias is found with
 * the same name, just replace it, and dispose of the old one.
 * Assume we get aliash with something in the aname. 
 * check gAliasCount is not at the maximum.  If it is, return 1.
 * Increment gAliasCount if we add a new one.
 */
	AliasHandle 	temp[MAXALIASES];
	short			done,result, index;
	char			*maxmsg = "No more aliases allowed.\r";
	
	HLock((Handle)aliash);
	result = findAlias((*aliash)->aname,&index);
	HUnlock((Handle)aliash);
	
	if (result == 1) {
		/* replace existing handle, dispose of old one */
		/* no need to do anything with gAliasCount here */
		disposAlias(gAliasList[index]);
		gAliasList[index] = aliash;
	} else {
		if (gAliasCount == MAXALIASES) {
			showDiagnostic(maxmsg);
			return(1);
		}
		/* move things over to make room. */
		BlockMove(&(gAliasList[index]),temp,(gAliasCount-index)*4);
		BlockMove(temp,&(gAliasList[index+1]),(gAliasCount-index)*4);
		gAliasList[index] = aliash;
		gAliasCount++;
	}
	return(0);
}


distroyAlias(index)
short	index;
{
/* Given the index of a handle to an alias, remove it from list, 
 * move the rest of the
 * list back to fill the void. 
 */
 	AliasHandle	temp[MAXALIASES];

	disposAlias(gAliasList[index]);
	BlockMove(&(gAliasList[index+1]),temp,(gAliasCount-index-1)*4);
	BlockMove(temp,&(gAliasList[index]),(gAliasCount-index-1)*4);
	
	gAliasCount--;
	
}

disposAlias(aliash)
 AliasHandle	aliash;
{
/* Given a handle to an alias, dispose of the memory it occupies */
	if (aliash) {
		while ((*aliash)->listc > 0) {
			DisposHandle((Handle)((*aliash)->listh)[((*aliash)->listc-1)]);
			((*aliash)->listc)--;
		}
		DisposHandle((Handle)aliash);
	}
		
}


short findAlias(aname,indexp)
 char	*aname;
 short	*indexp;
{
/* given an alias name, return the index of the handle to its record (in indexp).  
 * If this name is not found in the list, return the index of the handle
 * it would occupy. If an exact match is found, return value is 1, otherwise 0. 
 */
	short		result,lb,ub;
	
	if (gAliasCount > 0) {
		lb = 0;
		ub = gAliasCount - 1;
		result = binAliasSearch(&lb,&ub,aname);
		*indexp = lb;
	} else { /* this is the first one */
		*indexp = 0;
		result = 0;
	}
	return(result);	
}

short binAliasSearch(lbp,ubp,target)
	short 	*lbp, *ubp; /* current lower and upper bounds */
	char	*target;	/*target string */
{
/* Recursive routine to implement binary search in the alias list.
 * Find the approximate middle point between the two bounds, do a comparison,
 * and based on the result either reset the bounds and call ourselves again, or
 * return with both lower and upper bounds equal to either the index where the
 * target was found, or if not found, the index where the target would be
 * inserted.  If we did find an exact match, return 1, otherwise return 0.
 * 
 * If we get called with the two bounds referring to adjacent items, 
 * check them only if they are at one of the endpoints (bounds in the
 * middle of the list will have already been checked).
 */
 
	short	mp; /* midpoint */
	short	result,retval;
	
	mp = (*ubp - *lbp)/2; /* integer division truncates */
	
	if (mp != 0) {
		HLock((Handle)gAliasList[*lbp + mp]);
		result = compareStrings(target,(*(gAliasList[*lbp + mp]))->aname);
		HUnlock((Handle)gAliasList[*lbp + mp]);
		if (result == 1) {
			/* target is greater */
			*lbp += mp;
			retval = binAliasSearch(lbp,ubp,target);		
		} else if (result == -1) {
			/* midpoint is greater */
			*ubp = *lbp + mp;
			retval = binAliasSearch(lbp,ubp,target);					
		} else {
			/* match */
			*lbp += mp;
			*ubp = *lbp;
			retval = 1;
		}
	} else { /* adjacent or equal bounds */
		if (*lbp == 0) {
			HLock((Handle)gAliasList[*lbp]);
			result = compareStrings(target,(*(gAliasList[*lbp]))->aname);
			HUnlock((Handle)gAliasList[*lbp]);
			if (result == 1) { /* target is bigger than lower bound. */
				if (*ubp == (gAliasCount - 1)) { /* special case with two entries. */
					HLock((Handle)gAliasList[*ubp]);
					result = compareStrings(target,(*(gAliasList[*ubp]))->aname);
					HUnlock((Handle)gAliasList[*ubp]);
					if (result == 1) {
						retval = 0;
						*lbp = *ubp = *ubp + 1;
					} else if (result == -1) {
						retval = 0;
						*lbp = *ubp;
					} else {
						retval = 1;
						*lbp = *ubp;
					}						
				} else { 
					retval = 0;
					*lbp = *ubp = 1;
				}
			} else if (result == -1) {
				retval = 0;
				*ubp = 0;
			} else {
				retval = 1;
				*ubp = 0;
			}
		} else if (*ubp == (gAliasCount - 1)) {
			HLock((Handle)gAliasList[*ubp]);
			result = compareStrings(target,(*(gAliasList[*ubp]))->aname);
			HUnlock((Handle)gAliasList[*ubp]);
			if (result == 1) {
				retval = 0;
				*lbp = *ubp = *ubp + 1;
			} else if (result == -1) {
				retval = 0;
				*lbp = *ubp;
			} else {
				retval = 1;
				*lbp = *ubp;
			}
		} else {
			*lbp = *ubp;
			retval = 0;
		}
	}
	return(retval);
}


short parseAlias(ah,astring)
 	AliasHandle 	ah;
 	char			*astring;
 {
 /* Accept a string 'astring'.
  * Parse into arguments (observing shell metacharacter rules)
  * Allocate handles for the arguments and attach them to 
  * (*(*ah)->listh).
  * Report any errors that occur, and return true.  Return false if
  * no errors occurred.
  */
  	short argc,err,alen,i;
  	Handle argh;
  	
  	char	**argv;
  	char	*mismatchmsg = "Mis-matched quotes in alias definition. \r";
  	char	*memerr = "Out of memory. \r";

	argv = (char **)NewPtr(ARGVINC*sizeof(Ptr));  
	err = parseLine(astring,&argc,&argv);
	if (err) { 
		DisposPtr((Ptr)argv);
		return(err);
	} 	

	/* copy args to handles */
	i=0;
	while (i<argc) {
		alen = strlen((argv)[i]);
		argh = NewHandle(alen+1);
		if (!argh) {
			DisposPtr((Ptr)(argv));
			showDiagnostic(memerr);
			return(1);
		}
		HLock((Handle)argh);
		BlockMove((argv)[i],(*argh),(long)(alen+1));
		(*ah)->listh[i] = argh;
		HUnlock((Handle)argh);
		i++;
	}
	(*ah)->listc = argc;
	
	/* dispose of strings */
	disposArgs((int *)&argc, argv);
	DisposPtr((Ptr)(argv));
	
 	return(0);
 
 }	


showAliasList(stdiop)
	StdioPtr stdiop;
/* Put the alias definitions on standard output */
{
	short	i;
	
	for (i=0;i<gAliasCount;i++) {
		showAlias(stdiop,i);
	}
}

showAlias(stdiop,i)
StdioPtr	stdiop;
short		i;
/* Put one alias definition on the standard output */
{
	char	*spaces = "   ";
	char	*space = " ";
	char	*quotestr = "\"";
	short	j;
	
	HLock((Handle)gAliasList[i]);
	SOInsert(stdiop,(*(gAliasList[i]))->aname,strlen((*(gAliasList[i]))->aname));
	if ((*(gAliasList[i]))->listc > 0)
		SOInsert(stdiop,spaces,3);
	SOInsert(stdiop,quotestr,1);
	for (j=0;j<(*(gAliasList[i]))->listc;j++) {
		HLock((Handle)((*(gAliasList[i]))->listh)[j]);
		SOInsert(stdiop,*(((*(gAliasList[i]))->listh)[j]),
				strlen(*(((*(gAliasList[i]))->listh)[j])));
		HUnlock((Handle)((*(gAliasList[i]))->listh)[j]);
		if (j != (*(gAliasList[i]))->listc - 1)
			SOInsert(stdiop,space,1);
	}
	SOInsert(stdiop,quotestr,1);
	SOInsert(stdiop,RETURNSTR,1);
	HUnlock((Handle)gAliasList[i]);

}


short doUnalias(argc, argv, stdiop)
	int argc;
	char **argv;
	StdioPtr stdiop;
{
/* Find the alias specified and remove it from the list */

	char	*usage = "usage: unalias aliasname1 [aliasname2 ...] \r";
	char	*notdef = ": alias not defined. \r";
	short	i,ind;
	
	if (argc<2) {
		showDiagnostic(usage);
		return;
	}

	for(i=1;i<argc;i++) {
		if(findAlias(argv[i],&ind)) { 
			distroyAlias(ind);
		} else { /* not found */
			showDiagnostic(argv[i]);
			showDiagnostic(notdef);
		}		
	}
}


short aliasSubstitute(argcp,argvp)
	short	*argcp;
	char	***argvp;
{
/* if an alias is found which matches argv[0], replace it with  
 * its corresponding values, and adjust argc accordingly. 
 * When a successful substitution is done, dispose of old argument.
 * Return 1 for an error, 0 otherwise.  
 * This will be called by handleAllLines. 
 * 
 * if argv[0] is an alias, lookup value 
 * add values to temp arg list
 * merge other args
 *
 * For now we don't translate aliases recursively. (but csh does)
 *   
 *
 *      	
 */
 	char	**tempargv;
 	short	ind,i,j,slen;
 	char	*memerr = "Out of memory. \r";
 	
 if(findAlias((*argvp)[0],&ind)) { 
 	i=1;
 	while ((i*ARGVINC) <= ((*argcp)+((*(gAliasList[ind]))->listc)) ) i++;
	if (!(tempargv = (char **)NewPtr((i * ARGVINC * sizeof(Ptr))))) {
		showDiagnostic(memerr);
		return(1);
	}
 	i = 0;
 	while (i<((*(gAliasList[ind]))->listc)) { /* copy the alias words */
		HLock((Handle)((*(gAliasList[ind]))->listh)[i]);
		slen = strlen(*(((*(gAliasList[ind]))->listh)[i])) + 1;
 		if (!(tempargv[i] = NewPtr(slen))) {
 			DisposPtr((Ptr)tempargv);
 			showDiagnostic(memerr);
 			return(1);
 		}
 		BlockMove(*(((*(gAliasList[ind]))->listh)[i]),tempargv[i],(long)slen);
		HUnlock((Handle)((*(gAliasList[ind]))->listh)[i]);
		i++;
	}
	j=1;
	while (j<=*argcp) { /* attach the remaining arguments */
		tempargv[i] = (*argvp)[j];
		i++; j++;
	}	
	DisposPtr((Ptr)(*argvp)[0]);
	DisposPtr((Ptr)*argvp);
	*argvp = tempargv;
	*argcp = i - 1;
 } /* endif findalias */
 return(0);
	
}	
