# include	"curses.ext"
# include	<signal.h>
# include	<sys/param.h>

extern char	*getenv();

/*
 *	This routine initializes the current and standard screen.
 *
 * 5/19/83 (Berkeley) @(#)initscr.c	1.3
 */
WINDOW *
initscr() {

	reg char	*sp;
	int		tstp();

# ifdef DEBUG
	fprintf(outf, "INITSCR()\n");
# endif
	if (My_term)
		setterm(Def_term);
	else {
		if (isatty(2))
			_tty_ch = 2;
		else {
			for (_tty_ch = 0; _tty_ch < NOFILE; _tty_ch++)
				if (isatty(_tty_ch))
					break;
		}
		gettmode();
		if ((sp = getenv("TERM")) == NULL)
			sp = Def_term;
		setterm(sp);
# ifdef DEBUG
		fprintf(outf, "INITSCR: term = %s\n", sp);
# endif
	}
	_puts(TI);
	_puts(VS);
# ifdef SIGTSTP
	signal(SIGTSTP, tstp);
# endif
	if (curscr != NULL) {
# ifdef DEBUG
		fprintf(outf, "INITSCR: curscr = 0%o\n", curscr);
# endif
		delwin(curscr);
	}
# ifdef DEBUG
	fprintf(outf, "LINES = %d, COLS = %d\n", LINES, COLS);
# endif
	if ((curscr = newwin(LINES, COLS, 0, 0)) == ERR)
		return ERR;
	curscr->_clear = TRUE;
	if (stdscr != NULL) {
# ifdef DEBUG
		fprintf(outf, "INITSCR: stdscr = 0%o\n", stdscr);
# endif
		delwin(stdscr);
	}
	stdscr = newwin(LINES, COLS, 0, 0);
	return stdscr;
}
