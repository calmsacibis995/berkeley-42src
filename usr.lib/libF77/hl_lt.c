/*
 *	"@(#)hl_lt.c	1.1"
 */

short hl_lt(a,b,la,lb)
char *a, *b;
long int la, lb;
{
return(s_cmp(a,b,la,lb) < 0);
}
