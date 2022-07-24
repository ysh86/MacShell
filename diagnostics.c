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
#include "window.h"
#include "shellutils.h"

/* This file contains code for handling errors, exceptional 
 * situations, and messages to the diagnostic window */
 
errAlert(errStr)
	char	*errStr;
{
	int		result;
	CtoPstr(errStr);
	ParamText((unsigned char *)errStr,NIL,NIL,NIL);	
	/* TC7: should cast the second argument to (ModalFilterProcPtr) */
	result = Alert(333,(ModalFilterProcPtr)0L); 
	myPtoCstr(errStr);
}

memDeathAlert(who)
	char	*who; /* 14 chars or less */
{
	char	*memErr = "Memory Error";
	char	*memErr2 = ": Can't allocate memory.";
	char	*temp;
	
	/* nice idea, but in practice we need to call this 
	 * a little before NewPtr returns NIL, or we'll just hang... */
	temp = NewPtr(strlen(who) + 25);
	strcpy(temp, who);
	strcpy((temp+strlen(who)),memErr2);
	deathAlert(memErr,temp);
}
	

deathAlert(errStr1,errStr2)
	char	*errStr1; /* up to 18 chars */
	char	*errStr2; /* up to 36 chars */

{
	int		result;
	CtoPstr(errStr1);
	CtoPstr(errStr2);
	ParamText((unsigned char *)errStr1,(unsigned char *)errStr2,NIL,NIL);
	/* TC7 cast second arg to (ModalFilterProcPtr) */	
	result = CautionAlert(129,(ModalFilterProcPtr)0L); 
	closeAndQuit(); 
}	



showDiagnostic(message)	/* put a string into the diagnostic window.
						 * We don't care if the window is showing or not */
	char	*message;
{
	TEHandle	tempText;
	
	tempText = unpackTEH(diagnosticWin);
	putCursorAtEnd(tempText);
	TEInsert(message, strlen(message), tempText); 
	adjustTEPos(diagnosticWin); 
		
}



doMem()
/* report heap status */
{
	Ptr		p;
	long	n;
	char	str[256];

	p = (Ptr) GetApplLimit();
	sprintf(str,"Top of application heap: %lX \r", p);
	showDiagnostic(str);
	
	p = (Ptr) ApplicZone();
	sprintf(str,"Bottom of application heap: %lX \r", p);
	showDiagnostic(str);
	
	n = FreeMem();
	sprintf(str,"Free bytes in application heap: %ld \r", n); /*ld=long decimal*/
	showDiagnostic(str);
	
}	

doFree()
/* report free heap */
{
	long	n;
	char	*str;
	
	str = NewPtr(256);
	n = FreeMem();
	sprintf(str,"Free bytes in application heap: %ld \r", n); /*ld=long decimal*/
	showDiagnostic(str);
	DisposPtr((Ptr)str);
	
}	

doCompact()
/* compact heap, return the size of the largest contiguous block */
{
	long	n, grow;
	char	str[256];
	
	n=MaxMem(&grow);
	sprintf(str,"Largest contiguous block in application heap: %ld \r", n); /*ld=long decimal*/
	showDiagnostic(str);
	
}

showFileDiagnostic(fname,errRef) /* fname is a pascal string */
char	*fname;
OSErr	errRef;
{	/* These are just the file system errors */
	char *dirful = "File directory full: ";
	char *dskful = "Disk full: ";
	char *nsv = "No such volume: ";
	char *ioerr = "Disk IO error: ";
	char *badnm = "Bad Name error: ";
	char *fnopen = "File not opened: ";
	char *eoferr = "EOF during read: ";
	char *poserr = "Invalid mark position: ";
	char *mfulerr = "System heap is full: ";
	char *tmfoerr = "Too many files opened: ";
	char *fnferr = "File or folder not found: ";
	char *wprerr = "Disk write protected: ";
	char *flkderr = "File locked: ";
	char *vlkderr = "Volume locked: ";
	char *fbsy = "File busy: ";
	char *dupfn = "File with this name already exists: ";
	char *opwr = "File already opened for writing: ";
	char *paramerr = "Bad volume, no default: ";
	char *rfnumerr = "Non-existent access path: ";
	char *gfperr = "Error getting position: ";
	char *voloffln = "Volume not on-line: ";
	char *permerr = "Write permission not given: ";
	char *volonln = "Volume already on-line: ";
	char *nsdrv = "No such drive number: ";
	char *nomacdk = "Not a Mac disk: ";
	char *extfs = "External file system error: ";
	char *rnerr = "Error renaming: ";
	char *badmdb = "Bad Master directory block: ";
	char *wrperm = "Write permission not given: ";
	char *lastdk = "Low-level disk error: ";
	char *dirnferr = "Directory not found: ";
	char *tmwdo = "Too many working directories opened: ";
	char *badmv = "Attempted to move into offspring: ";
	char *wrgvol = "Non-heirarchical volume: ";
	char *fserr = "Internal filesystem error: ";
	char *nonfs = "Not a file system error: ";
	char *temp;
	
	temp = NewPtr(strlen(fname) + 255);
	
	if (errRef == -33)  strcpy(temp,dirful);
	else if (errRef == -34)  strcpy(temp,dskful);
	else if (errRef == -35)  strcpy(temp,nsv);
	else if (errRef == -36)  strcpy(temp,ioerr);
	else if (errRef == -37)  strcpy(temp,badnm);
	else if (errRef == -38)  strcpy(temp,fnopen);
	else if (errRef == -39)  strcpy(temp,eoferr);
	else if (errRef == -40)  strcpy(temp,poserr);
	else if (errRef == -41)  strcpy(temp,mfulerr);
	else if (errRef == -42)  strcpy(temp,tmfoerr);
	else if (errRef == -43)  strcpy(temp,fnferr);
	else if (errRef == -44)  strcpy(temp,wprerr);
	else if (errRef == -45)  strcpy(temp,flkderr);
	else if (errRef == -46)  strcpy(temp,vlkderr);
	else if (errRef == -47)  strcpy(temp,fbsy);
	else if (errRef == -48)  strcpy(temp,dupfn);
	else if (errRef == -49)  strcpy(temp,opwr);
	else if (errRef == -50)  strcpy(temp,paramerr);
	else if (errRef == -51)  strcpy(temp,rfnumerr);
	else if (errRef == -52)  strcpy(temp,gfperr);
	else if (errRef == -53)  strcpy(temp,voloffln);
	else if (errRef == -54)  strcpy(temp,permerr);
	else if (errRef == -55)  strcpy(temp,volonln);
	else if (errRef == -56)  strcpy(temp,nsdrv);
	else if (errRef == -57)  strcpy(temp,nomacdk);
	else if (errRef == -58)  strcpy(temp,extfs);
	else if (errRef == -59)  strcpy(temp,rnerr);
	else if (errRef == -60)  strcpy(temp,badmdb);
	else if (errRef == -61)  strcpy(temp,wrperm);
	else if (errRef == -64)  strcpy(temp,lastdk);
	else if (errRef == -120)  strcpy(temp,dirnferr);
	else if (errRef == -121)  strcpy(temp,tmwdo);
	else if (errRef == -122)  strcpy(temp,badmv);
	else if (errRef == -123)  strcpy(temp,wrgvol);
	else if (errRef == -127)  strcpy(temp,fserr);
	else strcpy(temp,nonfs);
			
	myPtoCstr(fname);
	strcat(temp,fname);
	strcat(temp,RETURNSTR);
	CtoPstr(fname);
	showDiagnostic(temp);
	DisposPtr((Ptr)temp);
}



