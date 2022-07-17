/* @(#)bcopy.s	4.4 9/25/83 */
/* bcopy(from, to, size) */

#include "DEFS.h"

	.globl	_bcopy
_bcopy:
	.word	0x0040
#ifdef PROF
	.data; 1:; .long 0; .text; moval 1b,r0; jsb mcount
#endif PROF
	movl	4(ap),r1
	movl	8(ap),r3
	movl	12(ap),r6
	cmpl	r1,r3
	bgtr	2f		# normal forward case
	blss	3f		# overlapping, must do backwards
	ret			# equal, nothing to do
1:
	subl2	r0,r6
	movc3	r0,(r1),(r3)
2:
	movzwl	$65535,r0
	cmpl	r6,r0
	jgtr	1b
	movc3	r6,(r1),(r3)
	ret
3:
	addl2	r6,r1
	addl2	r6,r3
	movzwl	$65535,r0
	jbr	5f
4:
	subl2	r0,r6
	subl2	r0,r1
	subl2	r0,r3
	movc3	r0,(r1),(r3)
	movzwl	$65535,r0
	subl2	r0,r1
	subl2	r0,r3
5:
	cmpl	r6,r0
	jgtr	4b
	subl2	r6,r1
	subl2	r6,r3
	movc3	r6,(r1),(r3)
	ret
