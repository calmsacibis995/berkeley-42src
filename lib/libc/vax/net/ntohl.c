/* @(#)ntohl.c	4.1 12/15/82 */
/* hostorder = ntohl(netorder) */

#include "DEFS.h"

ENTRY(ntohl)
	rotl	$-8,4(ap),r0
	insv	r0,$16,$8,r0
	movb	7(ap),r0
	ret
