/*	fcntl.h	4.2	83/09/25	*/

/*
 * Flag values accessible to open(2) and fcntl(2)
 *  (The first three can only be set by open)
 */
#define	O_RDONLY	0
#define	O_WRONLY	1
#define	O_RDWR		2
#define	O_NDELAY	FNDELAY	/* Non-blocking I/O */
#define	O_APPEND	FAPPEND	/* append (writes guaranteed at the end) */

#ifndef	F_DUPFD
/* fcntl(2) requests */
#define	F_DUPFD	0	/* Duplicate fildes */
#define	F_GETFD	1	/* Get fildes flags */
#define	F_SETFD	2	/* Set fildes flags */
#define	F_GETFL	3	/* Get file flags */
#define	F_SETFL	4	/* Set file flags */
#define	F_GETOWN 5	/* Get owner */
#define F_SETOWN 6	/* Set owner */

/* flags for F_GETFL, F_SETFL-- copied from <sys/file.h> */
#define	FNDELAY		00004		/* non-blocking reads */
#define	FAPPEND		00010		/* append on each write */
#define	FASYNC		00100		/* signal pgrp when data ready */
#endif
