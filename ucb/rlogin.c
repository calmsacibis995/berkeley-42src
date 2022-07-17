#ifndef lint
static char sccsid[] = "@(#)rlogin.c	4.15 (Berkeley) 83/07/02";
#endif

/*
 * rlogin - remote login
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <netinet/in.h>

#include <stdio.h>
#include <sgtty.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <netdb.h>

char	*index(), *rindex(), *malloc(), *getenv();
struct	passwd *getpwuid();
char	*name;
int	rem;
char	cmdchar = '~';
int	eight;
char	*speeds[] =
    { "0", "50", "75", "110", "134", "150", "200", "300",
      "600", "1200", "1800", "2400", "4800", "9600", "19200", "38400" };
char	term[64] = "network";
extern	int errno;
int	lostpeer();

main(argc, argv)
	int argc;
	char **argv;
{
	char *host, *cp;
	struct sgttyb ttyb;
	struct passwd *pwd;
	struct servent *sp;
	int uid, options = 0;

	host = rindex(argv[0], '/');
	if (host)
		host++;
	else
		host = argv[0];
	argv++, --argc;
	if (!strcmp(host, "rlogin"))
		host = *argv++, --argc;
another:
	if (argc > 0 && !strcmp(*argv, "-d")) {
		argv++, argc--;
		options |= SO_DEBUG;
		goto another;
	}
	if (argc > 0 && !strcmp(*argv, "-l")) {
		argv++, argc--;
		if (argc == 0)
			goto usage;
		name = *argv++; argc--;
		goto another;
	}
	if (argc > 0 && !strncmp(*argv, "-e", 2)) {
		cmdchar = argv[0][2];
		argv++, argc--;
		goto another;
	}
	if (argc > 0 && !strcmp(*argv, "-8")) {
		eight = 1;
		argv++, argc--;
		goto another;
	}
	if (host == 0)
		goto usage;
	if (argc > 0)
		goto usage;
	pwd = getpwuid(getuid());
	if (pwd == 0) {
		fprintf(stderr, "Who are you?\n");
		exit(1);
	}
	sp = getservbyname("login", "tcp");
	if (sp == 0) {
		fprintf(stderr, "rlogin: login/tcp: unknown service\n");
		exit(2);
	}
	cp = getenv("TERM");
	if (cp)
		strcpy(term, cp);
	if (ioctl(0, TIOCGETP, &ttyb)==0) {
		strcat(term, "/");
		strcat(term, speeds[ttyb.sg_ospeed]);
	}
	signal(SIGPIPE, lostpeer);
        rem = rcmd(&host, sp->s_port, pwd->pw_name,
	    name ? name : pwd->pw_name, term, 0);
        if (rem < 0)
                exit(1);
	if (options & SO_DEBUG &&
	    setsockopt(rem, SOL_SOCKET, SO_DEBUG, 0, 0) < 0)
		perror("rlogin: setsockopt (SO_DEBUG)");
	uid = getuid();
	if (setuid(uid) < 0) {
		perror("rlogin: setuid");
		exit(1);
	}
	doit();
	/*NOTREACHED*/
usage:
	fprintf(stderr,
	    "usage: rlogin host [ -ex ] [ -l username ] [ -8 ]\n");
	exit(1);
}

#define CRLF "\r\n"

int	child;
int	catchild();

int	defflags, tabflag;
char	deferase, defkill;
struct	tchars deftc;
struct	ltchars defltc;
struct	tchars notc =	{ -1, -1, -1, -1, -1, -1 };
struct	ltchars noltc =	{ -1, -1, -1, -1, -1, -1 };

doit()
{
	int exit();
	struct sgttyb sb;

	ioctl(0, TIOCGETP, (char *)&sb);
	defflags = sb.sg_flags;
	tabflag = defflags & TBDELAY;
	defflags &= ECHO | CRMOD;
	deferase = sb.sg_erase;
	defkill = sb.sg_kill;
	ioctl(0, TIOCGETC, (char *)&deftc);
	notc.t_startc = deftc.t_startc;
	notc.t_stopc = deftc.t_stopc;
	ioctl(0, TIOCGLTC, (char *)&defltc);
	signal(SIGINT, exit);
	signal(SIGHUP, exit);
	signal(SIGQUIT, exit);
	child = fork();
	if (child == -1) {
		perror("rlogin: fork");
		done();
	}
	signal(SIGINT, SIG_IGN);
	mode(1);
	if (child == 0) {
		reader();
		sleep(1);
		prf("\007Connection closed.");
		exit(3);
	}
	signal(SIGCHLD, catchild);
	writer();
	prf("Closed connection.");
	done();
}

done()
{

	mode(0);
	if (child > 0 && kill(child, SIGKILL) >= 0)
		wait((int *)0);
	exit(0);
}

catchild()
{
	union wait status;
	int pid;

again:
	pid = wait3(&status, WNOHANG|WUNTRACED, 0);
	if (pid == 0)
		return;
	/*
	 * if the child (reader) dies, just quit
	 */
	if (pid < 0 || pid == child && !WIFSTOPPED(status))
		done();
	goto again;
}

/*
 * writer: write to remote: 0 -> line.
 * ~.	terminate
 * ~^Z	suspend rlogin process.
 * ~^Y  suspend rlogin process, but leave reader alone.
 */
writer()
{
	char b[600], c;
	register char *p;
	register n;

top:
	p = b;
	for (;;) {
		int local;

		n = read(0, &c, 1);
		if (n == 0)
			break;
		if (n < 0)
			if (errno == EINTR)
				continue;
			else
				break;

		if (eight == 0)
			c &= 0177;
		/*
		 * If we're at the beginning of the line
		 * and recognize a command character, then
		 * we echo locally.  Otherwise, characters
		 * are echo'd remotely.  If the command
		 * character is doubled, this acts as a 
		 * force and local echo is suppressed.
		 */
		if (p == b)
			local = (c == cmdchar);
		if (p == b + 1 && *b == cmdchar)
			local = (c != cmdchar);
		if (!local) {
			if (write(rem, &c, 1) == 0) {
				prf("line gone");
				return;
			}
			if (eight == 0)
				c &= 0177;
		} else {
			if (c == '\r' || c == '\n') {
				char cmdc = b[1];

				if (cmdc == '.' || cmdc == deftc.t_eofc) {
					write(0, CRLF, sizeof(CRLF));
					return;
				}
				if (cmdc == defltc.t_suspc ||
				    cmdc == defltc.t_dsuspc) {
					write(0, CRLF, sizeof(CRLF));
					mode(0);
					signal(SIGCHLD, SIG_IGN);
					kill(cmdc == defltc.t_suspc ?
					  0 : getpid(), SIGTSTP);
					signal(SIGCHLD, catchild);
					mode(1);
					goto top;
				}
				*p++ = c;
				write(rem, b, p - b);
				goto top;
			}
			write(1, &c, 1);
		}
		*p++ = c;
		if (c == deferase) {
			p -= 2; 
			if (p < b)
				goto top;
		}
		if (c == defkill || c == deftc.t_eofc ||
		    c == '\r' || c == '\n')
			goto top;
		if (p >= &b[sizeof b])
			p--;
	}
}

oob()
{
	int out = 1+1, atmark;
	char waste[BUFSIZ], mark;

	ioctl(1, TIOCFLUSH, (char *)&out);
	for (;;) {
		if (ioctl(rem, SIOCATMARK, &atmark) < 0) {
			perror("ioctl");
			break;
		}
		if (atmark)
			break;
		(void) read(rem, waste, sizeof (waste));
	}
	recv(rem, &mark, 1, MSG_OOB);
	if (mark & TIOCPKT_NOSTOP) {
		notc.t_stopc = -1;
		notc.t_startc = -1;
		ioctl(0, TIOCSETC, (char *)&notc);
	}
	if (mark & TIOCPKT_DOSTOP) {
		notc.t_stopc = deftc.t_stopc;
		notc.t_startc = deftc.t_startc;
		ioctl(0, TIOCSETC, (char *)&notc);
	}
}

/*
 * reader: read from remote: line -> 1
 */
reader()
{
	char rb[BUFSIZ];
	register int cnt;

	signal(SIGURG, oob);
	{ int pid = -getpid();
	  ioctl(rem, SIOCSPGRP, (char *)&pid); }
	for (;;) {
		cnt = read(rem, rb, sizeof (rb));
		if (cnt == 0)
			break;
		if (cnt < 0) {
			if (errno == EINTR)
				continue;
			break;
		}
		write(1, rb, cnt);
	}
}

mode(f)
{
	struct tchars *tc;
	struct ltchars *ltc;
	struct sgttyb sb;

	ioctl(0, TIOCGETP, (char *)&sb);
	switch (f) {

	case 0:
		sb.sg_flags &= ~(CBREAK|RAW|TBDELAY);
		sb.sg_flags |= defflags|tabflag;
		tc = &deftc;
		ltc = &defltc;
		sb.sg_kill = defkill;
		sb.sg_erase = deferase;
		break;

	case 1:
		sb.sg_flags |= (eight ? RAW : CBREAK);
		sb.sg_flags &= ~defflags;
		/* preserve tab delays, but turn off XTABS */
		if ((sb.sg_flags & TBDELAY) == XTABS)
			sb.sg_flags &= ~TBDELAY;
		tc = &notc;
		ltc = &noltc;
		sb.sg_kill = sb.sg_erase = -1;
		break;

	default:
		return;
	}
	ioctl(0, TIOCSLTC, (char *)ltc);
	ioctl(0, TIOCSETC, (char *)tc);
	ioctl(0, TIOCSETN, (char *)&sb);
}

/*VARARGS*/
prf(f, a1, a2, a3)
	char *f;
{
	fprintf(stderr, f, a1, a2, a3);
	fprintf(stderr, CRLF);
}

lostpeer()
{
	signal(SIGPIPE, SIG_IGN);
	prf("\007Connection closed.");
	done();
}
