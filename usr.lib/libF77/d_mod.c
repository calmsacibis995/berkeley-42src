/*
 *	"@(#)d_mod.c	1.1"
 */

double d_mod(x,y)
double *x, *y;
{
double floor(), quotient = *x / *y;
if (quotient >= 0.0)
	quotient = floor(quotient);
else
	quotient = -floor(-quotient);
return(*x - (*y) * quotient );
}
