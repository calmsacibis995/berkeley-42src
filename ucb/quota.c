#ifndef lint
static char sccsid[] = "@(#)quota.c	4.4 (Berkeley, from Melbourne) 6/21/83";
#endif

/*
 * Disk quota reporting program.
 */
#include <stdio.h>
#include <fstab.h>
#include <ctype.h>
#include <pwd.h>

#include <sys/param.h>
#include <sys/quota.h>
#include <sys/file.h>
#include <sys/stat.h>

int	qflag;
int	vflag;
int	done;
int	morethanone;
char	*qfname = "quotas";

main(argc, argv)
	char *argv[];
{
	register char *cp;

	argc--,argv++;
	while (argc > 0) {
		if (argv[0][0] == '-')
			for (cp = &argv[0][1]; *cp; cp++) switch (*cp) {

			case 'v':
				vflag++;
				break;

			case 'q':
				qflag++;
				break;

			default:
				fprintf(stderr, "quota: %c: unknown option\n",
					*cp);
				exit(1);
			}
		else
			break;
		argc--, argv++;
	}
	morethanone = argc > 1;
	if (argc == 0) {
		showuid(getuid());
		exit(0);
	}
	for (; argc > 0; argc--, argv++) {
		if (alldigits(*argv))
			showuid(atoi(*argv));
		else
			showname(*argv);
	}
}

showuid(uid)
	int uid;
{
	struct passwd *pwd = getpwuid(uid);

	if (pwd == NULL)
		showquotas(uid, "(no account)");
	else
		showquotas(uid, pwd->pw_name);
}

showname(name)
	char *name;
{
	struct passwd *pwd = getpwnam(name);

	if (pwd == NULL) {
		fprintf(stderr, "quota: %s: unknown user\n", name);
		return;
	}
	showquotas(pwd->pw_uid, name);
}

showquotas(uid, name)
	int uid;
	char *name;
{
	register char c, *p;
	register struct fstab *fs;
	int myuid;

	myuid = getuid();
	if (uid != myuid && myuid != 0) {
		printf("quota: %s (uid %d): permission denied\n", name, uid);
		return;
	}
	done = 0;
	setfsent();
	while (fs = getfsent()) {
		register char *msgi = (char *)0, *msgb = (char *)0;
		register enab = 1;
		dev_t	fsdev;
		struct	stat statb;
		struct	dqblk dqblk;
		char qfilename[MAXPATHLEN + 1], iwarn[8], dwarn[8];

		if (stat(fs->fs_spec, &statb) < 0)
			continue;
		fsdev = statb.st_rdev;
		(void) sprintf(qfilename, "%s/%s", fs->fs_file, qfname);
		if (stat(qfilename, &statb) < 0 || statb.st_dev != fsdev)
			continue;
		if (quota(Q_GETDLIM, uid, fsdev, &dqblk) != 0) {
			register fd = open(qfilename, O_RDONLY);

			if (fd < 0)
				continue;
			lseek(fd, (long)(uid * sizeof (dqblk)), L_SET);
			if (read(fd, &dqblk, sizeof dqblk) != sizeof (dqblk)) {
				close(fd);
				continue;
			}
			close(fd);
			if (dqblk.dqb_isoftlimit == 0 &&
			    dqblk.dqb_bsoftlimit == 0)
				continue;
			enab = 0;
		}
		if (dqblk.dqb_ihardlimit &&
		    dqblk.dqb_curinodes >= dqblk.dqb_ihardlimit)
			msgi = "File count limit reached on %s";
		else if (enab && dqblk.dqb_iwarn == 0)
			msgi = "Out of inode warnings on %s";
		else if (dqblk.dqb_isoftlimit &&
		    dqblk.dqb_curinodes >= dqblk.dqb_isoftlimit)
			msgi = "Too many files on %s";
		if (dqblk.dqb_bhardlimit &&
		    dqblk.dqb_curblocks >= dqblk.dqb_bhardlimit)
			msgb = "Block limit reached on %s";
		else if (enab && dqblk.dqb_bwarn == 0)
			msgb = "Out of block warnings on %s";
		else if (dqblk.dqb_bsoftlimit &&
		    dqblk.dqb_curblocks >= dqblk.dqb_bsoftlimit)
			msgb = "Over disc quota on %s";
		if (dqblk.dqb_iwarn < MAX_IQ_WARN)
			sprintf(iwarn, "%d", dqblk.dqb_iwarn);
		else
			iwarn[0] = '\0';
		if (dqblk.dqb_bwarn < MAX_DQ_WARN)
			sprintf(dwarn, "%d", dqblk.dqb_bwarn);
		else
			dwarn[0] = '\0';
		if (qflag) {
			if (msgi != (char *)0 || msgb != (char *)0)
				heading(uid, name);
			if (msgi != (char *)0)
				xprintf(msgi, fs->fs_file);
			if (msgb != (char *)0)
				xprintf(msgb, fs->fs_file);
			continue;
		}
		if (vflag || dqblk.dqb_curblocks || dqblk.dqb_curinodes) {
			heading(uid, name);
			printf("%10s%8d%c%7d%8d%8s%8d%c%7d%8d%8s\n"
				, fs->fs_file
				, (dqblk.dqb_curblocks / btodb(1024)) 
				, (msgb == (char *)0) ? ' ' : '*'
				, (dqblk.dqb_bsoftlimit / btodb(1024)) 
				, (dqblk.dqb_bhardlimit / btodb(1024)) 
				, dwarn
				, dqblk.dqb_curinodes
				, (msgi == (char *)0) ? ' ' : '*'
				, dqblk.dqb_isoftlimit
				, dqblk.dqb_ihardlimit
				, iwarn
			);
		}
	}
	endfsent();
	if (!done && !qflag) {
		if (morethanone)
			putchar('\n');
		xprintf("Disc quotas for %s (uid %d):", name, uid);
		xprintf("none.");
	}
	xprintf(0);
}

heading(uid, name)
	int uid;
	char *name;
{

	if (done++)
		return;
	xprintf(0);
	if (qflag) {
		if (!morethanone)
			return;
		xprintf("User %s (uid %d):", name, uid);
		xprintf(0);
		return;
	}
	putchar('\n');
	xprintf("Disc quotas for %s (uid %d):", name, uid);
	xprintf(0);
	printf("%10s%8s %7s%8s%8s%8s %7s%8s%8s\n"
		, "Filsys"
		, "current"
		, "quota"
		, "limit"
		, "#warns"
		, "files"
		, "quota"
		, "limit"
		, "#warns"
	);
}

xprintf(fmt, arg1, arg2, arg3, arg4, arg5, arg6)
	char *fmt;
{
	char	buf[100];
	static int column;

	if (fmt == 0 && column || column >= 40) {
		putchar('\n');
		column = 0;
	}
	if (fmt == 0)
		return;
	sprintf(buf, fmt, arg1, arg2, arg3, arg4, arg5, arg6);
	if (column != 0 && strlen(buf) < 39)
		while (column++ < 40)
			putchar(' ');
	else if (column) {
		putchar('\n');
		column = 0;
	}
	printf("%s", buf);
	column += strlen(buf);
}

alldigits(s)
	register char *s;
{
	register c;

	c = *s++;
	do {
		if (!isdigit(c))
			return (0);
	} while (c = *s++);
	return (1);
}
