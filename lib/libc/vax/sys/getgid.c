/* getgid.c 4.1 82/12/04 */

#include "SYS.h"

SYSCALL(getgid)
	ret		# gid = getgid();
