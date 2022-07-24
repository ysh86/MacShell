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
#include "variable.h"
#include "shellutils.h"
#include "standalone.h"

/* This file contains code for the shell variable stuff */
/*
	Notes on shell variables
	------------------------
	
	Which types to support:
	boolean (no strings, variable record simply exists if true)
	integer (one string, all numeric)
	word	(one string)
	word list (list of strings)
	
	What data structure to use:
	-Global list of handles which refer to records containing:
		name pointer (variable name)
		pointer to a list of strings (variable value)
		number of strings in the list
	
	Any arg beginning with "$" is a candidate for variable substitution.  This
	should happen in handleAllLines(?).
	
	$?vname tells whether vname is set or not. (returns one or zero)
	
	$#vname gets the number of strings in vname
	
	$vname[number] selects a particular string from vname.
	
	"Set" should be a recognized function.  Takes two arguments, check if the 
	variable already exists, then either updates or creates one.
	 
	"set a" creates a null handle so that a subsequent "set a[1]=z" works.
	Conversely "set a=()" creates no handle at all.
		
	note:(*vh)->listh[(*vh)->listc] is not an allocated handle 
	  (unlike the analogous situation with argc and argv)
	
*/

VarHandle	gVarList[MAXVARS];
short		gVarCount;


short varSubstitute(argcp,argvp)
	short	*argcp;
	char	***argvp;
{
/* if any shell variables are found in argv, replace them with their 
 * values, and adjust argc accordingly. 
 * When a successful substitution is done, dispose of old argument.
 * Return 1 for an error, 0 otherwise.  
 * This will be called by handleAllLines and separately under doRun  when
 * flow control statements are implemented. 
 * 
 * for each argument:
 *   parse into varname and literal segments
 *	 for each segment:
 *      lookup value if it's a varname
 *      add values to temp arg list
 *   merge values for entire arg into argv 
 *   
 * The variable substitution mechanism should ignore double quotes, and proceed
 * as if arguments were not quoted.   
 *
 * We use fixed space for ARGVINC pointers in tempargv. This should be ok as
 * long as we limit the length of the whole command line to ARGVINC or fewer
 * characters (which we do currently).  --> 12/95: won't be enough for use with 
 * backquote substitution. -->1/96: limit removed.
 *
 * accumulateVal enforces a more stingent limit on the maximum sumvalc (MAXARGS).
 * Maybe this limit should be removed, but at least it seems robust. -->1/96: limit removed.
 *
 * mergeVal also enforces the more stringent limit. -->1/96: limit removed.
 *
 * 12/95: Make it possible to hide '$' from this function by enclosing and
 *		argument in single quotes.  This is to accomodate the use of this
 *		character in egrep regexps.
 *      Need to remove argument limits to accomodate backquote substitution.
 *		Need to respect double quoted segments within arguments.  If these expand
 *		to multiple words, they should stay together in a single argument. It may be 
 *		possible for variables to introduce mis-matched quotes this way, so we should
 *		check for this.  1/96: all done.
 *      	
 */
 	char	**val,**sumval,**tempargv,*valstr;
 	short	valc,sumvalc,tempargc,err,i,j,one=1,inquote;
 	
 	if (!(sumval = (char **)NewPtr(ARGVINC*sizeof(Ptr)))) {
 		showDiagnostic(gMEMERR);
 		return(1);
 	}

 	if (!(tempargv = (char **)NewPtr(ARGVINC*sizeof(Ptr)))) {
 		showDiagnostic(gMEMERR);
 		return(1);
 	}
 	
	for(i=0;i<*argcp;i++) { /* note: *argcp may increase as we go! */
			err = getVSegs((*argvp)[i],tempargv,&tempargc);
			inquote=0;
			if (!err) {
				sumvalc = 0;
				for(j=0;j<tempargc;j++) {
					if (tempargv[j][0] == '$') {
						err = getValue((tempargv[j]+1), &val, &valc);					
						if (err) {
							DisposPtr((Ptr)tempargv[j]);
							return(1);
						}
						if (inquote) {
							if (unParse(val,valc,&valstr)) return(1);
							accumulateVal(&sumval,&sumvalc,&valstr,&one);
							DisposPtr((Ptr)valstr);
						} else {
							accumulateVal(&sumval,&sumvalc,val,&valc);
						}
						
						DisposPtr((Ptr)val);	
						DisposPtr((Ptr)tempargv[j]);
					} else {
						checkOddQuote(tempargv[j],&inquote);
						accumulateVal(&sumval,&sumvalc,&(tempargv[j]),&one);
					}
				}
				if ((sumvalc>1) || (strcmp((*argvp)[i], sumval[0]) != 0)) {
					err = mergeValue(sumval,&sumvalc,argvp,argcp,&i); 
					if (err) { 
						/* should clean up */
						return(1);
					}
				} else {
					DisposPtr((Ptr)sumval[0]);
				}
				i = i+sumvalc-1; 
			} else {
				return(1);
			}
	}
	DisposPtr((Ptr)sumval);
	DisposPtr((Ptr)tempargv);
	return(0);
}


VarHandle newVar()
{
/* Create storage for a new variable, and initialize it. If there isn't enough memory 
 * to create the record, return NULL */

	VarHandle	vh;
	
	vh = (VarHandle)NewHandle(sizeof(VarRec));
	if (vh) {
		(*vh)->vname[0] = '\0'; /* null string in the name. */
		(*vh)->listc = 0;
	}
	return(vh);
}

short insertVar(varh)
 VarHandle	varh;
{
/* insert the given handle at the appropriate place in the list.
 * If a var is found with
 * the same name, just replace it, and dispose of the old one.
 * Assume we get varh with something in the vname. 
 * check gVarCount is not at the maximum.  If it is, return 1.
 * Increment gVarCount if we add a new one.
 */
	VarHandle 	temp[MAXVARS];
	short		done,result, index;
	char		*maxmsg = "No more shell variables allowed.\r";
	
	HLock((Handle)varh);
	result = findVar((*varh)->vname,&index);
	HUnlock((Handle)varh);
	
	if (result == 1) {
		/* replace existing handle, dispose of old one */
		/* no need to do anything with gVarCount here */
		disposVar(gVarList[index]);
		gVarList[index] = varh;
	} else {
		if (gVarCount == MAXVARS) {
			showDiagnostic(maxmsg);
			return(1);
		}
		/* move things over to make room. */
		BlockMove(&(gVarList[index]),temp,(gVarCount-index)*4);
		BlockMove(temp,&(gVarList[index+1]),(gVarCount-index)*4);
		gVarList[index] = varh;
		gVarCount++;
	}
	return(0);
}


distroyVar(index)
short	index;
{
/* Given the index of a handle to a var, remove it from list, 
 * move the rest of the
 * list back to fill the void. 
 */
 	VarHandle	temp[MAXVARS];

	disposVar(gVarList[index]);
	BlockMove(&(gVarList[index+1]),temp,(gVarCount-index-1)*4);
	BlockMove(temp,&(gVarList[index]),(gVarCount-index-1)*4);
	
	gVarCount--;
	
}

disposVar(varh)
 VarHandle	varh;
{
/* Given a handle to a var, dispose of the memory it occupies */
	if (varh) {
		while ((*varh)->listc > 0) {
			DisposHandle((Handle)((*varh)->listh)[((*varh)->listc-1)]);
			((*varh)->listc)--;
		}
		DisposHandle((Handle)varh);
	}
		
}


short findVar(vname,indexp)
 char	*vname;
 short	*indexp;
{
/* given a variable name, return the index of the handle to its record (in indexp).  
 * If this name is not found in the list, return the index of the handle
 * it would occupy. If an exact match is found, return value is 1, otherwise 0. 
 */
	short		result,lb,ub;
	
	if (gVarCount > 0) {
		lb = 0;
		ub = gVarCount - 1;
		result = binSearch(&lb,&ub,vname);
		*indexp = lb;
	} else { /* this is the first one */
		*indexp = 0;
		result = 0;
	}
	return(result);	
}

short binSearch(lbp,ubp,target)
	short 	*lbp, *ubp; /* current lower and upper bounds */
	char	*target;	/*target string */
{
/* Recursive routine to implement binary search in the shell variable list.
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
		HLock((Handle)gVarList[*lbp + mp]);
		result = compareStrings(target,(*(gVarList[*lbp + mp]))->vname);
		HUnlock((Handle)gVarList[*lbp + mp]);
		if (result == 1) {
			/* target is greater */
			*lbp += mp;
			retval = binSearch(lbp,ubp,target);		
		} else if (result == -1) {
			/* midpoint is greater */
			*ubp = *lbp + mp;
			retval = binSearch(lbp,ubp,target);					
		} else {
			/* match */
			*lbp += mp;
			*ubp = *lbp;
			retval = 1;
		}
	} else { /* adjacent or equal bounds */
		if (*lbp == 0) {
			HLock((Handle)gVarList[*lbp]);
			result = compareStrings(target,(*(gVarList[*lbp]))->vname);
			HUnlock((Handle)gVarList[*lbp]);
			if (result == 1) { /* target is bigger than lower bound. */
				if (*ubp == (gVarCount - 1)) { /* special case with two vars. */
					HLock((Handle)gVarList[*ubp]);
					result = compareStrings(target,(*(gVarList[*ubp]))->vname);
					HUnlock((Handle)gVarList[*ubp]);
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
		} else if (*ubp == (gVarCount - 1)) {
			HLock((Handle)gVarList[*ubp]);
			result = compareStrings(target,(*(gVarList[*ubp]))->vname);
			HUnlock((Handle)gVarList[*ubp]);
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

showVarList(stdiop)
	StdioPtr stdiop;
/* Put the variable value pairs on standard output */
{
	short	i;
	
	for (i=0;i<gVarCount;i++) {
		showVar(stdiop,i);
	}
}

showVar(stdiop, i)
StdioPtr	stdiop;
short		i;
/* Put one variable value pair on the standard output */
{
	char	*spaces = "   ";
	char	*space = " ";
	char	*openparen = "(";
	char	*closeparen = ")";
	short	j;
	
	HLock((Handle)gVarList[i]);
	SOInsert(stdiop,(*(gVarList[i]))->vname,strlen((*(gVarList[i]))->vname));
	if ((*(gVarList[i]))->listc > 0)
		SOInsert(stdiop,spaces,3);
	if ( ((*(gVarList[i]))->listc>1) || ( ((*(gVarList[i]))->listc==1) && 
		**(((*(gVarList[i]))->listh)[0])==0) )
		SOInsert(stdiop,openparen,1);
	for (j=0;j<(*(gVarList[i]))->listc;j++) {
		HLock((Handle)((*(gVarList[i]))->listh)[j]);
		SOInsert(stdiop,*(((*(gVarList[i]))->listh)[j]),
				strlen(*(((*(gVarList[i]))->listh)[j])));
		HUnlock((Handle)((*(gVarList[i]))->listh)[j]);
		if (j != (*(gVarList[i]))->listc - 1)
			SOInsert(stdiop,space,1);
	}
	if ( ((*(gVarList[i]))->listc>1) || ( ((*(gVarList[i]))->listc==1) && 
		**(((*(gVarList[i]))->listh)[0])==0) )
		SOInsert(stdiop,closeparen,1);
	SOInsert(stdiop,RETURNSTR,1);
	HUnlock((Handle)gVarList[i]);

}


short doSet(argc, argv, stdiop)
	int argc;
	char **argv;
	StdioPtr stdiop;
{
/* 
 *   set					show the value of all variables in the list.
 *   set name				set name to null value (one null value handle)
 *   set name=word			set name to word
 *   set name[index]=word	set this item in a preexisting list
 *   set name=(wordlist)	set the entire word list
 * 
 */
 	VarHandle 	vh;
 	Handle		valh;
 	short		i,carg,vlen,err,index;
 	char		*vtmp;
 	char		*memerr = "Set:Memory error.\r";
 	char		*usage = "Usage: set [var=value...]\r";
 	char		*badname = "Set:Name needs leading alpha-numeric. \r";
 	char		*multval  = "Set:Variable with subscript must refer to a single value. \r";

	if (argc == 1) {
		/* show variables */
		showVarList(stdiop);
		return(0);
	}
	
	carg = 1;
	i = 0;
	while (carg < argc) {
		vh = newVar();
		if (!vh) {
			showDiagnostic(memerr);
			return(1);
		}
	
		/* set name, truncate it at 63 characters. */
		while ((argv[carg][i] != 0) && (argv[carg][i] != '=') && (i<MAXNAMELEN-1)) {
			((*vh)->vname)[i]=argv[carg][i];
			i++;
		}
		if (i == 0) {
			showDiagnostic(badname);
			disposVar(vh);
			return(1);
		}
		((*vh)->vname)[i] = 0;		
		if (i==MAXNAMELEN-1) {
			while ((argv[carg][i] != 0) && (argv[carg][i] != '='))
				i++;
		}		
		if (argv[carg][i] == 0) {
			carg++;
			i = 0;
		}
	
		/* pull an index off the name, if it's there, and verify that it's a number. */
		/* return -1 for no index, -2 for nonnumeric */
		index = handleSubscript((*vh)->vname);
		if (index == -2) {
			disposVar(vh);
			return(1);
		} 
		
		/* get a value */
		if (argv[carg][i] == '=') {
			i++;
			if (argv[carg][i] == 0) {
				carg++;
				i = 0;
			} 		
			if (argv[carg][i] == 0) {
				/* equal sign with nothing following: null value */
				if (index == -1) { /* no index */
					(*vh)->listc = 1;
					(*vh)->listh[0] = NewHandle(1);
					**((*vh)->listh[0]) = 0;
				} else {
					valh = NewHandle(1);
					**valh = 0;
				}
			} else {
				vtmp = &argv[carg][i];
				if (vtmp[0] == '(') { /* do list */
					if (index == -1) {
						/* err means unbalenced parens */
						err = handleList(vh,vtmp,argv,&carg,(short *)&argc);
						if (err) {
							disposVar(vh);
							return(1);
						}
					} else {
						showDiagnostic(multval); 
						disposVar(vh);
						return(1);
					}
				} else { /* single value */
					vlen = strlen(vtmp);
					valh = NewHandle(vlen+1);
					BlockMove(vtmp,*valh,vlen+1);
					(*vh)->listh[0] = valh;
					(*vh)->listc = 1;
				}
			}
			carg++;
			i = 0;
		} else { /* no equal sign */
			if (index == -1) { /* no index */
				(*vh)->listc = 1;
				(*vh)->listh[0] = NewHandle(1);
				**((*vh)->listh[0]) = 0;
			} else {
				valh = NewHandle(1);
				**valh = 0;
			}
		}
		
		if (index == -1) {
			err = insertVar(vh); /* err means we're over the maximum */
		} else {
			err = insertIndexedVar(vh,&index,valh); 	/* err means index was out of 
													 * range, or var not found */
		}
		if(err){ 
			disposVar(vh);
			return(1);
		} 

	}
	addPrefs(vh);
	return(0);
}

short insertIndexedVar(vh,valind,valh)
	VarHandle	vh;
	short		*valind;
	Handle		valh;
{
/* find the variable named in vh.  Make sure it has a list of values
 * such that index is in its range.  If it does, replace the indexed 
 * value with valh, and dispose of the old handle. 
 * If the variable isn't found, or the index is out of range,
 * report the fact, dispose of valh, and return 1.
 */
	short		err, targetind, result,varind;
	Handle		tmpvalh;
	char		*outofrange = "Subscript out of range.\r";
	char		*varnotset = "Variable not set.\r";
	
	err = FALSE;
	HLock((Handle)vh);
	result = findVar((*vh)->vname,&varind);
	HUnlock((Handle)vh);
	
	if (result == 1) {
		/* Is index in range? */
		if ((*valind) <= (*(gVarList[varind]))->listc) {
			/* subscripts begin at 1 */
			tmpvalh = (*(gVarList[varind]))->listh[(*valind)-1];
			(*(gVarList[varind]))->listh[(*valind)-1] = valh;
			valh = tmpvalh;
		} else {
			showDiagnostic(outofrange);
			err = TRUE;
		}
	} else {
		/* var not found */
		showDiagnostic(varnotset);
	}

	DisposHandle((Handle)valh);
	return(err);

}


short handleSubscript(name)
	char	*name;
{
/* look for a matched set of '[' and ']' in name where '[' is not the first char
 * and ']' is the last one.  If found, replace '[' with null, verify that the 
 * value between the brackets is an integer, convert from string, and return it.
 * Return -1 means no subscript, -2 for any error. 
 */ 
 
	short	i,open,retval;
	long	result;
	char	nstr[256];
	char	*mismatch = "Mismatched brackets.\r";
	char	*leadingalpha = "Name must have leading alpha-numeric. \r";
	char	*outofrange = "Subscript out of range. \r";
	char	*nonnumeric = "Subscript must be an integer. \r";
	char	*noembedded = "Embedded brackets not allowed. \r";
	
	i = 0;
	while ((name[i]!=0) && (name[i]!='[')) i++;
	if (name[i]==0) return(-1);
	if (i==0) {
		showDiagnostic(leadingalpha);
		return(-2);
	}
	open = i;
	while ((name[i]!=0) && (name[i]!=']')) i++;
	if (name[i]==0) {
		showDiagnostic(mismatch);
		return(-2);
	}
	if (name[i+1] != 0) {
		showDiagnostic(noembedded);
		return(-2); 
	}
	
	BlockMove(&(name[open+1]),nstr,i-open-1);
	nstr[i-open-1] = 0;
	if (isNum(nstr)) {
		CtoPstr(nstr);
		StringToNum((unsigned char *)nstr,&result);
		/* subscript values may range from 1 to MAXWORDS */
		if ((result <= MAXWORDS) && (result > 0)) {
			retval = result;
			name[open]=0;
			return(retval); 
		} else {
			/* out of range */
			showDiagnostic(outofrange);
			return(-2);
		}
	} else {
		/* nonnumeric subscript */
		showDiagnostic(nonnumeric);
		return(-2);
	}
}



short handleList(vh,vtmp,argv,cargp,argcp)
	VarHandle	vh;
	char		*vtmp;
	char		**argv;
	short		*cargp,*argcp;
{
/* Take a parenthesised list of values in one or more args.  Create handles
 * and attach them to (*vh)->listh.  set (*vh)->listc.
 * carg is the current argument.  vtmp points to the current position in it,
 * namely the position where '(' was found.
 * Return 1 if no closing paren is found at the end of a remaining arg, 
 * or if the closing paren is not followed by null.
 * return 0 otherwise.
 * set *cargp to the one containing the ')' before returning.  
 * In the case of an empty list do not create any handles.
 * 1/96: Enforce the limit MAXWORDS.
 */
 
	char	*trailingchar = "Closing paren must be followed by a space. \r";
	char	*noclosing = "Mismatched parens. \r"; 
	char	*tmw = "Exceeded maximum number of words allowed in a single variable. \r";
	short	err, i, done, j,k,cargsave,numwords;
  
  	err = FALSE;
	i = 0;
	cargsave = *cargp; /* first argument */
	while ((vtmp[i] != 0) && (vtmp[i] != ')')) i++;
	if (vtmp[i] == 0) { /* not in first arg */
		/* scan ahead to find ')' */
		(*cargp)++; /* vtmp pointed into the old carg */
		done = FALSE;
		while ((*cargp < (*argcp)) && (!done)) {
			j = 0;
			while ((argv[*cargp][j] != ')') && (argv[*cargp][j] != 0))  j++;
			if (argv[*cargp][j] == 0) {
				(*cargp)++;
			} else if (argv[*cargp][j+1] != 0){
				err=TRUE;
				done = TRUE;	
				break;		
			} else {
				done = TRUE;
				break;
			}
		}
		if (*cargp == (*argcp)) { 
			showDiagnostic(noclosing);
			return(1);
		} else if (err) {
			showDiagnostic(trailingchar);
			return(1);
		}
		numwords=((*cargp) - cargsave -1);		
		if (i>1) numwords++;
		if (j>0) numwords++;
		if (numwords>MAXWORDS) {
			showDiagnostic(tmw);
			return(1);
		}
		/* everything looks ok, start making handles. */
		(*vh)->listc = 0;
		if (i > 1) { /* first argument */
			((*vh)->listh)[(*vh)->listc] = NewHandle(i);
			BlockMove(vtmp+1,(*((*vh)->listh)[(*vh)->listc]),i);
			(*vh)->listc++;
		}	
		for(cargsave++;cargsave<*cargp;cargsave++) { /* middle arguments */
			k = strlen(argv[cargsave]);
			((*vh)->listh)[(*vh)->listc] = NewHandle(k+1);
			BlockMove(argv[cargsave],(*((*vh)->listh)[(*vh)->listc]),k+1);
			(*vh)->listc++;
		}
		argv[*cargp][j] = 0; /* chop off the close paren */
		if (j>0) { /* last argument */
			((*vh)->listh)[(*vh)->listc] = NewHandle(j+1);
			BlockMove(argv[*cargp],(*((*vh)->listh)[(*vh)->listc]),j+1);
			(*vh)->listc++;			
		}
	} else if (vtmp[i+1] == 0) { /*it's all in a single arg*/
		if (i>1) {
			vtmp[i] = 0; /* chop off the close paren */
			((*vh)->listh)[0] = NewHandle(i);
			BlockMove(vtmp+1,(*((*vh)->listh)[0]),i);
			(*vh)->listc = 1;	
		} else {	(*vh)->listc = 0;  }
	} else { /* "(xxx)yyy" */
		showDiagnostic(trailingchar);
		return(1);
	}
	
  	return(0);
 
}

short doUnset(argc, argv, stdiop)
	int argc;
	char **argv;
	StdioPtr stdiop;
{
/* Find the var specified and remove it from the list */
/* unset with csh:
	-allows patterns to be used, eg: "unset *"
	-unset with no arguments does nothing, but is not an error
	-unset with an indexed arg does nothing, but is not an error.
	-unset with a nonexistant var does nothing, but is not an error.
*/
	char	*novar = "Unset: No variables specified. \r";
	char	*notfound = ": Variable not found. \r";
	short	i,ind;
	
	if (argc<2) {
		showDiagnostic(novar);
		return;
	}

	for(i=1;i<argc;i++) {
		if(findVar(argv[i],&ind)) { /* found it */
			distroyVar(ind);
			removePrefs(argv[i]);
		} else { /* not found */
			showDiagnostic(argv[i]);
			showDiagnostic(notfound);
		}		
	}
}



	

short getVSegs(argstr,tempargv,tempargcp)
	char	*argstr;
	char	**tempargv;
	short	*tempargcp;
{
/* Parse a string containing an argument into pieces in
 * preparation for variable substitutuion.
 * 
 * We can assume that there are no mismatched quotes, since parseline
 * has already checked this.
 *
 * 12/95: Any single quoted sections should not be variable substituted.
 *
 * while not eol
 *  while not $ or eol i++
 *  if eol
 *    copy last arg
 *  else
 *    extract vname, copy arg
 *
 * We limit an argument to ARGVINC segments.  This limit should be ok for now(??)
 */

	short	i=0,j=0,vend;
	char *tma="Too many variables in a single argument.\r";
	*tempargcp = 0;
	
	while(argstr[i] != 0) {
		while ((argstr[i] != '$') && (argstr[i] != 0)) {
			if (argstr[i] == SQ) {
				i++;
				while ((argstr[i] != SQ) && (argstr[i] != 0)) i++;
				if (argstr[i] == SQ) i++;
			} else {
				i++;
			}
		}
		if (i>j) {
			/* make arg for last literal */
			if ((*tempargcp) == (ARGVINC)) {
				for(i=0;i<(*tempargcp);i++) {
					DisposPtr((Ptr)tempargv[i]);
				}
				*tempargcp=0;
				showDiagnostic(tma);
				return(1);			
			}
			tempargv[*tempargcp] = NewPtr(i-j+1);
			BlockMove(&(argstr[j]),tempargv[*tempargcp],i-j);
			tempargv[*tempargcp][i-j] = 0;
			j = i; (*tempargcp)++;
		}
		if (argstr[i] != 0) {
			/* go to end of variable, make arg, update things */
			vend = guessVarEnd(&(argstr[i+1]));
			if (vend == -1) {
				/* error */
				for(i=0;i<(*tempargcp);i++) {
					DisposPtr((Ptr)tempargv[i]);
				}
				*tempargcp=0;
				return(1);
			}
			i +=vend+1;
			if ((*tempargcp) == (ARGVINC)) {
				for(i=0;i<(*tempargcp);i++) {
					DisposPtr((Ptr)tempargv[i]);
				}
				*tempargcp=0;
				showDiagnostic(tma);
				return(1);			
			}
			tempargv[*tempargcp] = NewPtr(i-j + 1);
			BlockMove(&(argstr[j]),tempargv[*tempargcp],i-j);
			tempargv[*tempargcp][i-j] = 0;
			j = i; (*tempargcp)++;
		}
	}
 	return(0);
}	

short  guessVarEnd(vstr)
	char *vstr;
{
/* look for the end of a variable name given in vstr.  Return the index 
 * one past the last character. Vstr should point one past the '$'.  
 * Return -1 for any error.  
 * We know we've reached the end when 
 * -we find a matching set of brackets:
 * 		${varname}junk, ${var{}name}, etc
 * -if no curly brackets are found, but sqare brackets are found, 
 *  when the close bracket occurs.
 *  	$var[jfjfj]
 * -if no brackets, when we find another dollar symbol:
 *  	$varone$vartwo, but not $varone[$vartwo]
 * -when we reach the null character.
 * -when we reach a single or double quote (only in the absence of brackets)
 */
 
	short	i=0,j;
	char	co = '{',cc = '}',so = '[',sc = ']';
	
	if (vstr[0] == '{') {
		j = findClose(&cc,&co,&(vstr[1]));
		if (j==-1) i = -1;
		else i = j+1;
	} else {
		while ((vstr[i] != '[') && (vstr[i] != '$') && (vstr[i] != 0) &&
		       (vstr[i] != SQ) && (vstr[i] != DQ)) i++;
		if (vstr[i] == '[') {
			/* find closing square bracket */
			j = findClose(&sc,&so,&(vstr[i+1]));
			if (j==-1) i = -1;
			else i = i+j+1;			
		} 
	}
	return(i);

}

short	findClose(cchar,ochar,str)
	char	*cchar,*ochar,*str;
{
/* return the index in string str to the closing "bracket" cchar.
 * The opening bracket is ochar and nested brackets are possible.
 * str points one past where the open bracket was found.
 * Return -1 for error 
 */
	short i,done,depth;
	char	*mism = "Mismatched brackets. \r";
	
	i=0;
	done = FALSE;
	depth = 0;
	while (!done) {
		while ((str[i]!=(*ochar))&&(str[i]!=(*cchar))&&(str[i]!=0)) i++;
		if (str[i]==0) {
			showDiagnostic(mism);
			return(-1);
		}
		if (str[i]==(*cchar)) {
			if (depth==0) {
				done=TRUE;
				i++;				
			} else {
				depth--; i++;
			}
		} else {
			depth++;
			i++;
		}		
	}
	return(i);
}

checkOddQuote(char *str,short *inquote)
/* toggle the value of inquote for every double quote character
 * contained in str.
 */
{
	short j=0;
	while (str[j] != 0) {
		if (str[j]=='"') {
			if (*inquote == 0) *inquote=1;
			else *inquote=0;
		}
		j++;
	}
}



short unParse(char **val,short valc, char **valstr)
/* Convert a set of strings in val into a space separated, null terminated string 
 * Return TRUE if out of memory. 
 * Dispose of strings allocated to val.
 */
{
	short i,len=0,ind=0;
	for (i=0;i<valc;i++) {
		len=len+strlen(val[i]);
		len++;
	}
	len++;
	if (!((*valstr)=NewPtr(len))) {
		showDiagnostic(gMEMERR);
		return(1);
	}
	for (i=0;i<valc;i++) {
		len=strlen(val[i]);
		BlockMove(val[i],(*valstr)+ind,(long)len);
		(*valstr)[ind+len]=' ';
		DisposPtr((Ptr)val[i]);
		ind=ind+len+1;	
	}
	(*valstr)[ind-1]=0;
	return(0);

}


accumulateVal(sumval,sumvalcp,val,valc)
	char	***sumval,**val;
	short	*sumvalcp,*valc;
{
/* Copy valc args from val to *sumvalp.  Merge the first arg of val into the
 * last of *sumvalp. Update *sumvalcp.
 * Assume we have at least one valid "val" string.
 * Dispose of val[0] and sumval[*sumvalcp - 1] after copying them into
 * a newly created pointer.
 * 12/95: remove argument limits.
 */

	char	*tma = "Too many shell variable arguments. \r";
	short	len1,len2,len,i;
	char	*tempval;
	
	/* do the first one */
	if (*sumvalcp > 0) {
		len1 = strlen((*sumval)[(*sumvalcp) - 1]);
	} else {
		len1 = 0;
		*sumvalcp = 1;
	}
	len2 = strlen(val[0]);
	len = len1 + len2 + 1;
	tempval = (char *)NewPtr(len);
	BlockMove((*sumval)[(*sumvalcp) - 1],tempval,len1);
	BlockMove(val[0],tempval+len1,len2);
	tempval[len-1] = 0;
	DisposPtr((Ptr)val[0]);	
	if (len1 > 0) {
		DisposPtr((Ptr)(*sumval)[(*sumvalcp)-1]);
	}
	(*sumval)[(*sumvalcp)-1] = tempval;

	
	/* just copy pointers for the rest */
	for (i=1;i<(*valc);i++) {
	 	if (checkArgvSize(sumval,*sumvalcp,*sumvalcp +1)) { 
			/* disposArgs(&argc, *argvp); */
			showDiagnostic(gMEMERR);
			return(1);
	 	 }

		(*sumval)[(*sumvalcp)] = val[i];
		(*sumvalcp)++;
	}
}


mergeValue(val,valc,argvp,argcp,index)
	char	**val,***argvp;
	short	*valc,*argcp,*index;
{
/* Merge valc strings from val into argv at argv[index].  Dispose of old
 * argv[index], and update *argcp. 
 */
 	char	**temp;
 	short	templen;
 	
 	templen = (*argcp)+(*valc)-1;
 	
 	if (checkArgvSize(argvp,*argcp,(templen))) { 
		/* disposArgs(&argc, *argvp); */
		showDiagnostic(gMEMERR);
		return(1);
 	 }
 	 	
 	if (!(temp = (char **)NewPtr(((*argcp)+1)*sizeof(Ptr)))) {
 		showDiagnostic(gMEMERR);
 		return(1);
 	}
 	BlockMove(*argvp,temp,((*argcp)+1)*4);
 	
 	BlockMove(val,(*argvp)+(*index),(*valc)*4);
 	BlockMove(temp+(*index)+1,(*argvp)+(*valc)+(*index),(*argcp-(*index))*4);
 	DisposPtr((Ptr)temp[(*index)]);
 	DisposPtr((Ptr)temp);
 	*argcp = templen;
 	return(0);
}


short getValue(varname, val, valcp)
	char	*varname;
	char	***val;
	short	*valcp;
{
/* Given a variable name, return a pointer to a freshly allocated list 
 * of strings that contain its value in "val".
 * Return the number of strings returned in "valcp".
 * Return 1 if variable is not found or any other error occurs, 0 otherwise.
 * 
 * -If the variable is indexed by another variable, call ourselves to
 * look up the index first.
 * -The '$' should be stripped before this is called. 
 * -It's up to the caller to merge the results into argv.
 *
 * Other leading characters have special meaning as follows:
 *   ? -  return '1' if set, '0' otherwise.
 *   # -  return the number of items in this var's list
 * */
	char	*index;
	char	**locval;
	short	locvalc,vind,err,tempint,zero=0,tmpind;
	long	indlong;
	char	*nni = "Index must be a positive integer. \r";
	char	*negind = "Zero index not allowed. \r";
	
	stripBrackets(varname);	
 	err = getIndex(varname, &index); 
 	if(err == -1) { /* error */
 		return(1); 
 	} else if (err == 0) { /* no index, look it up */		
 		err = makeVarStrings(varname,&zero,val,valcp);
 		if (err) return(1);
 	} else {
 		/* indexed */
 		if (index[0] == '$') {
 			err = getValue(index+1,&locval,&locvalc);
 			if((!err) && (locvalc == 1) && (isNum(locval[0]))) {
 				/* try to convert index and look it up */
	 			CtoPstr(locval[0]);
				StringToNum((unsigned char *)locval[0],&indlong);
				myPtoCstr(locval[0]);
				if (indlong <= 0L) {
					showDiagnostic(nni);
					DisposPtr((Ptr)locval[0]);
					DisposPtr((Ptr)locval);
					return(1);
				} else {
					tmpind = (short)indlong;
	 				err = makeVarStrings(varname,&tmpind,val,valcp);	
 				}
 			} else {
 				/* error */
 				if (!err) {
 					showDiagnostic(nni);
 				}
 				if ((locvalc > 0) && (!err)) {
 					tempint = locvalc-1;
 					disposArgs((int *)&tempint,locval); 
 					DisposPtr((Ptr)locval); 
 				}
 				return(1);
 			}
 			tempint = locvalc-1;
 			disposArgs((int *)&tempint,locval);
 			DisposPtr((Ptr)locval);
 		} else if (isNum(index)) {
 			/* try to convert index and look it up */
 			CtoPstr(index);
			StringToNum((unsigned char *)index,&indlong);
			myPtoCstr(index);
			if ((short)indlong > 0) {
				tmpind=(short)indlong;
 				err = makeVarStrings(varname,&tmpind,val,valcp);	
 				if(err) return(1);
 			} else {
 				showDiagnostic(negind);
 				return(1);
 			}
 		} else {
 			showDiagnostic(nni);
 			return(1);
 		}
	}
			
	return(0);
}

stripBrackets(vname)
	char *vname;
{
/* if the first character is '{', shift the characters back one and lop off
 * the last character.  We assume matched brackets here.
 */
 
	short	len;
	char	temp[256];
	
	if (vname[0]=='{') {
		len = strlen(vname);
		BlockMove(vname+1,temp,len-1);
		BlockMove(temp,vname,len-1);
		vname[len-2]=0;
	}


}



short getIndex(vname,index)
	char	*vname, **index;
{
/* if vname is indexed, chop off the ']' and the '['. return a pointer to 
 * the beginning of the index string.  Return -1 for error, and 
 * 0 for no index.  Return 1 if an index is found. */
 
	short	i,len;
	char	*vse = "Variable syntax error. \r";
	char	*lae = "Variable name must have leading alpha-numeric. \r";
	
 	i = 0;
	while(vname[i] != '[' && vname[i] != 0) i++;
	if (vname[i] == 0) {
		return(0);
	} else {
		if (i==0) {
			showDiagnostic(lae);
			return(-1);
		}
		len = strlen(vname);
		if (vname[len-1] != ']') {
			showDiagnostic(vse);
			return(-1);
		}
		vname[len-1] = 0;
		vname[i]=0;
		*index = &(vname[i+1]);
		return(1);
	}			
}

short makeVarStrings(vname,index,val,valcp)
	char *vname,***val;
	short	*valcp,*index;
{
/* look up vname or vname[(*index)]. (if non-indexed, (*index)==0).  
 * Create strings in val and set *valcp. 
 * If null value, return one null pointer.
 * If variable is not found, return 1, otherwise 0. 
 * 
 * Leading characters have special meaning as follows:
 *   ? -  return '1' if set, '0' otherwise.
 *   # -  return the number of items in this var's list
 */

	short	vindex, found,i,templen,err;
 	char	*oor = "Variable index out of range. \r";
 	char	*vnf = "Variable not found. \r";
 	Handle 	*templh,temph;
 	char	*tempvn;
 	
 	if ((vname[0]=='?') || (vname[0] =='#')){
 		tempvn = vname+1;
 	} else {
 		tempvn = vname;
 	}
 	
	found = findVar(tempvn,&vindex);
	if (vname[0]=='?') {
		err = makeTFString(&found,index,&vindex,val,valcp);
		/* this never returns an error */
	} else if (vname[0] =='#') {
		err = makeCntString(&found,index,&vindex,val,valcp);
		if (err) return(1);		
	} else if(!found){
		*valcp = 0;
		showDiagnostic(vnf);
		return(1);
	} else {
		if ((*index) > (*(gVarList[vindex]))->listc) {
			*valcp = 0;
			showDiagnostic(oor);
			return(1);
		}
		if ((*(gVarList[vindex]))->listc == 0) {
			*val = (char **)NewPtr(sizeof(Ptr));
			(*val)[0] = (char *)NewPtr(1);
			(*val)[0][0] = 0;
			*valcp = 1;
		} else if ((*index) == 0) { /* no index */
			*val = (char **)NewPtr(sizeof(Ptr)*(*(gVarList[vindex]))->listc);	
			HLock((Handle)gVarList[vindex]);
			templh = (*(gVarList[vindex]))->listh;
			for(i=0;i<(*(gVarList[vindex]))->listc;i++) {
				HLock((Handle)templh[i]);
				templen = strlen(*(templh[i]));
				(*val)[i] = NewPtr(templen+1);
				BlockMove(*(templh[i]),(*val)[i],templen+1);				
				HUnlock((Handle)templh[i]);
			}
			*valcp = (*(gVarList[vindex]))->listc;
			HUnlock((Handle)gVarList[vindex]);
		} else { /* indexed value */
			*val = (char **)NewPtr(sizeof(Ptr));
			HLock((Handle)((*(gVarList[vindex]))->listh)[(*index)-1]);
			temph = ((*(gVarList[vindex]))->listh)[(*index)-1];
			templen = strlen(*temph);
			(*val)[0] = NewPtr(templen+1);
			BlockMove(*temph,(*val)[0],templen+1);
			*valcp = 1;
			HUnlock((Handle)((*(gVarList[vindex]))->listh)[(*index)-1]);
		}
	}
	return(0);
}


short makeTFString(found,index,vind,val,valcp)
	short	*found,*vind,*index,*valcp;
	char	***val;
{
/* Return a value string of 1 or 0 depending on whether or not a variable whose
 * handle is found at vind, and value index is 'index' is set. */
 
 	*val = (char **)NewPtr(sizeof(Ptr));
	(*val)[0] = (char *)NewPtr(4);
	(*val)[0][1] = 0;
	*valcp = 1;

	if ((!(*found)) || ((*index)>(*(gVarList[(*vind)]))->listc)){
		/* false */
		(*val)[0][0] = '0';
	} else {
		/* true */
		(*val)[0][0] = '1';
	}
	return(0);
}


short makeCntString(found,index,vind,val,valcp)
	short	*found,*vind,*index,*valcp;
	char	***val;
{
/* Return the number of words in the value. Indexed and non-existant variables
 * cause an error (return=1).
 */
 	char	*vnf = "Variable not found. \r";
 	char	*noind = "'#' not allowed with indexed variable. \r";
	long	tmpcnt = 0L;
	char	tmpstr[256];
	short	len;
	
	if (!(*found)) {
		showDiagnostic(vnf);
		return(1);
	} else if ((*index) != 0) {
		showDiagnostic(noind);
		return(1);
	} else {
		tmpcnt += (*(gVarList[(*vind)]))->listc;
		NumToString(tmpcnt,(unsigned char *)tmpstr);
		len = tmpstr[0];
		myPtoCstr(tmpstr);	
	 	*val = (char **)NewPtr(sizeof(Ptr));
		(*val)[0] = (char *)NewPtr(len+1);
		BlockMove(tmpstr,(*val)[0],len+1);
		*valcp = 1;			
	}
	return(0);
}

addPrefs(vh)
	VarHandle vh;
{
/* called by doSet.  Update globals representing shell preferences to reflect
 * new state of the variable described by vh
 */
 	char	*remapesc = "remapesc";
	char	*tabnc = "tabnc";
	char	*path = "path";
	char	*manpath = "manpath";
	
	if (strcmp((*vh)->vname, remapesc) == 0) {
		gRemapEsc = TRUE;
	} else if (strcmp((*vh)->vname, tabnc) == 0) {
		gTabNC = TRUE;
	} else if (strcmp((*vh)->vname, path) == 0) {
		clearPath(gPathList,&gPathCount,1);
		setPath(gPathList,&gPathCount,vh,1);
	} else if (strcmp((*vh)->vname, manpath) == 0) {
		clearPath(gManpathList,&gManpathCount,0);
		setPath(gManpathList,&gManpathCount,vh,0);
	}
	
}

removePrefs(vname)
	char *vname;
{
/* called by doUnset.  Update globals representing shell preferences to reflect
 * new unset state of variable "vname".
 */
 	char	*remapesc = "remapesc";
	char	*tabnc = "tabnc";
	char	*path = "path";
	char	*manpath = "manpath";

	if (strcmp(vname, remapesc) == 0) {
		gRemapEsc = FALSE;
	} else if (strcmp(vname, tabnc) == 0) {
		gTabNC = FALSE;
	} else if (strcmp(vname, path) == 0) {
		clearPath(gPathList,&gPathCount,1);
	} else if (strcmp(vname, manpath) == 0) {
		clearPath(gManpathList,&gManpathCount,0);
	}
	
}