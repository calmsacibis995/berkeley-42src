/*	bitcopy.c	4.1	83/03/09	*/
/*
 * Copy from msrc to mdest.
 * This is done as it is because it would be much slower to do it
 * a bit at a time.
 */

#include "bit.h"

bitcopy(mdest, msrc, rows, cols)
bitmat mdest, msrc;
int rows, cols;
{
	register int size = ((cols + 7) >> 3) * rows;
	register char *p, *q;

	for (p = &mdest[size], q = &msrc[size]; p>=mdest; )
		*--p = *--q;
}
