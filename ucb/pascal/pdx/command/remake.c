/* Copyright (c) 1982 Regents of the University of California */

static char sccsid[] = "@(#)remake.c 1.2 3/8/82";

/*
 * Remake the object file from the source.
 */

#include "defs.h"
#include "command.h"
#include "object.h"

/*
 * Invoke "pi" on the dotpfile, then reread the symbol table information.
 *
 * We have to save tracing info before, and read it in after, because
 * it might contain symbol table pointers.
 *
 * We also have to restart the process so that px dependent information
 * is recomputed.
 */

remake()
{
    char *tmpfile;

    if (call("pi", stdin, stdout, dotpfile, NIL) == 0) {
	if (strcmp(objname, "obj") != 0) {
	    call("mv", stdin, stdout, "obj", objname, NIL);
	}
	tmpfile = mktemp("/tmp/pdxXXXX");
	setout(tmpfile);
	status();
	unsetout();
	bpfree();
	objfree();
	initstart();
	readobj(objname);
	setinput(tmpfile);
	unlink(tmpfile);
    } else {
	puts("pi unsuccessful");
    }
}
