/* Copyright (c) 1982 Regents of the University of California */

/* static char sccsid[] = "@(#)object.h 1.3 1/19/82"; */

/*
 * Object module definitions.
 *
 * The object module is the interface to the object file; in particular
 * it contains the routines that read symbol and line number information.
 */

char *objname;			/* name of object file */
int objsize;			/* size of object code */

struct {
	unsigned int stringsize;	/* size of the dumped string table */
	unsigned int nsyms;		/* number of symbols */
	unsigned int nfiles;		/* number of files */
	unsigned int nlines;		/* number of lines */
} nlhdr;

char *stringtab;	/* string table */
char *dotpfile;		/* name of compiled file */

readobj();		/* read in the object file */
objfree();		/* release storage for object file information */
