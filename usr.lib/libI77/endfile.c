/*
char id_endfile[] = "@(#)endfile.c	1.7";
 *
 * endfile
 */

#include "fio.h"

static char	endf[]	= "endfile";

f_end (a)
alist	*a;
{
	unit	*b;

	lfname = NULL;
	elist = NO;
	errflag = a->aerr;
	lunit = a->aunit;
	if (not_legal(lunit))
		err (errflag, F_ERUNIT, endf)
	b = &units[lunit];
	if (!b->ufd)
		err (errflag, F_ERNOPEN, endf)
	if (b->uend)
		return(0);
	lfname = b->ufnm;
	b->uend = YES;
	return ( t_runc (b, errflag, endf) );
}

t_runc (b, flag, str)
unit	*b;
ioflag	flag;
char	*str;
{
	long	loc;

	if (b->uwrt)
		fflush (b->ufd);
	if (b->url || !b->useek || !b->ufnm)
		return (OK);	/* don't truncate direct access files, etc. */
	loc = ftell (b->ufd);
	if (truncate (b->ufnm, loc) != 0)
		err (flag, errno, str)
	if (b->uwrt && ! nowreading(b))
		err (flag, errno, str)
	return (OK);
}
