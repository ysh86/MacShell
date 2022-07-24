/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */
#define NSUBEXP  10
typedef struct regexp {
	char *startp[NSUBEXP];
	char *endp[NSUBEXP];
	char regstart;		/* Internal use only. */
	char reganch;		/* Internal use only. */
	char *regmust;		/* Internal use only. */
	int regmlen;		/* Internal use only. */
	char program[1];	/* Unwarranted chumminess with compiler. */
} regexp;

extern void regsub();
extern void regerror();

/* regexp.c */
extern regexp *regcomp(char *exp);
extern char *reg(int paren, int *flagp);
extern  char *regbranch(int *flagp);
extern  char *regpiece(int *flagp);
extern  char *regatom(int *flagp);
#ifndef TC7
extern  char *regnode(char op);
extern void reginsert(char op, char *opnd);
extern  void regc(char b);
#endif
extern  void regtail(char *p, char *val);
extern  void regoptail(char *p, char *val);
extern int regexec(regexp *prog, char *string);
extern  int regtry(regexp *prog, char *string);
extern  int regmatch(char *prog);
extern  int regrepeat(char *p);
extern  char *regnext(char *p);
extern void regdump(regexp *r);
extern  char *regprop(char *op);
extern  int strcspn(char *s1, char *s2);

extern	char *mystrchr(char *s, int c);

/* try.c */
extern  doTry(int argc, char *argv[], StdioPtr stdiop);
extern	void regerror(char *s);
extern	error(char *s1, char *s2);
extern	multiple(StdioPtr stdiop);
extern	try(char **fields);
extern	complain(char *s1, char *s2);
