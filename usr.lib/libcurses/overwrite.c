# include	"curses.ext"

# define	min(a,b)	(a < b ? a : b)

/*
 *	This routine writes win1 on win2 destructively.
 *
 * 3/27/83 (Berkeley) @(#)overwrite.c	1.3
 */
overwrite(win1, win2)
reg WINDOW	*win1, *win2; {

	reg int		x, y, minx, miny, startx, starty;

# ifdef DEBUG
	fprintf(outf, "OVERWRITE(0%o, 0%o);\n", win1, win2);
# endif
	miny = min(win1->_maxy, win2->_maxy);
	minx = min(win1->_maxx, win2->_maxx);
# ifdef DEBUG
	fprintf(outf, "OVERWRITE:\tminx = %d,  miny = %d\n", minx, miny);
# endif
	starty = win1->_begy - win2->_begy;
	startx = win1->_begx - win2->_begx;
	if (startx < 0)
		startx = 0;
	for (y = 0; y < miny; y++)
		if (wmove(win2, y + starty, startx) != ERR)
			for (x = 0; x < minx; x++)
				waddch(win2, win1->_y[y][x]);
}
