/* Copyright (c) 1982 Regents of the University of California */

static char sccsid[] = "@(#)objaddr.c 1.2 2/17/82";

/*
 * Lookup the object address of a given line from the named file.
 *
 * Potentially all files in the file table need to be checked
 * until the line is found since a particular file name may appear
 * more than once in the file table (caused by includes).
 */

#include "defs.h"
#include "mappings.h"
#include "object.h"
#include "source.h"
#include "filetab.h"
#include "linetab.h"

ADDRESS objaddr(line, name)
LINENO line;
char *name;
{
    register FILETAB *ftp;
    register LINENO i, j;
    BOOLEAN foundfile;

    if (nlhdr.nlines == 0) {
	return(-1);
    }
    if (name == NULL) {
	name = cursource;
    }
    foundfile = FALSE;
    for (ftp = &filetab[0]; ftp < &filetab[nlhdr.nfiles]; ftp++) {
	if (streq(ftp->filename, name)) {
	    foundfile = TRUE;
	    i = ftp->lineindex;
	    if (ftp == &filetab[nlhdr.nfiles-1]) {
		j = nlhdr.nlines;
	    } else {
		j = (ftp + 1)->lineindex;
	    }
	    while (i < j) {
		if (linetab[i].line == line) {
		    return linetab[i].addr;
		}
		i++;
	    }
	}
    }
    if (!foundfile) {
	error("unknown source file \"%s\"", name);
    }
    return(-1);
}
