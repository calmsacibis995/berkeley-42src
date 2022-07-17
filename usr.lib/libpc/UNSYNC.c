/* Copyright (c) 1979 Regents of the University of California */

static char sccsid[] = "@(#)UNSYNC.c 1.3 6/10/81";

#include "h00vars.h"

/*
 * push back last char read to prepare for formatted read
 */
UNSYNC(curfile)

	register struct iorec	*curfile;
{
	if (curfile->funit & FWRITE) {
		ERROR("%s: Attempt to read, but open for writing\n",
			curfile->pfname);
		return;
	}
	if (curfile->funit & EOFF) {
		ERROR("%s: Tried to read past end of file\n", curfile->pfname);
		return;
	}
	if ((curfile->funit & SYNC) == 0) {
		ungetc(*curfile->fileptr, curfile->fbuf);
	}
}
