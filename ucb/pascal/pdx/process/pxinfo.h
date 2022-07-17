/* Copyright (c) 1982 Regents of the University of California */

/* static char sccsid[] = "@(#)pxinfo.h 1.2 1/18/82"; */

/*
 * This is the information we get after the initial trap that px does.
 * By passing the "-d" flag, we cause px to call a procedure with the
 * the following information:
 *
 *	address of the display
 *	address of the display pointer
 *	address of the beginning of the object code
 *
 *	the address of a variable containing the address of the first local
 *		variable in the interpreter procedure
 *		(this is used to find the frame where the pc is)
 *	the address of the main interpreter loop
 */

typedef struct {
	int nargs;
	ADDRESS *disp;
	ADDRESS *dp;
	ADDRESS objstart;
	ADDRESS pcaddrp;
	ADDRESS loopaddr;
} TRAPARGS;

ADDRESS *DISPLAY;
ADDRESS *DP;
ADDRESS ENDOFF;
ADDRESS PCADDRP;
ADDRESS LOOPADDR;
ADDRESS *pcframe;
