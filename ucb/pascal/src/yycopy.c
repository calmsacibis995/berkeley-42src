/* Copyright (c) 1979 Regents of the University of California */

static	char sccsid[] = "@(#)yycopy.c 1.2 5/11/83";

#include	"whoami.h"
#include	"0.h"
#include	"yy.h"

OYcopy ()
    {
	register int	*r0 = & OY;
	register int	*r1 = & Y;
	register int	r2 = ( sizeof ( struct yytok ) ) / ( sizeof ( int ) );

	do
	    {
		* r0 ++ = * r1 ++ ;
	    }
	    while ( -- r2 > 0 );
    }
