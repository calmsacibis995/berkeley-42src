#ifndef lint
static char *sccsid = "@(#)cp.c	4.8 83/07/01";
#endif

/*
 * cp
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>

#define	BSIZE	8192

int	iflag;
int	rflag;
char	*rindex(), *sprintf();

main(argc, argv)
	int argc;
	char **argv;
{
	struct stat stb;
	int rc, i;

	argc--, argv++;
	while (argc > 0 && **argv == '-') {
		(*argv)++;
		while (**argv) switch (*(*argv)++) {

		case 'i':
			iflag++; break;

		case 'r':
			rflag++; break;

		default:
			goto usage;
		}
		argc--; argv++;
	}
	if (argc < 2) 
		goto usage;
	if (argc > 2 || rflag) {
		if (stat(argv[argc-1], &stb) < 0)
			goto usage;
		if ((stb.st_mode&S_IFMT) != S_IFDIR) 
			goto usage;
	}
	rc = 0;
	for (i = 0; i < argc-1; i++)
		rc |= copy(argv[i], argv[argc-1]);
	exit(rc);
usage:
	fprintf(stderr,
	    "Usage: cp f1 f2; or cp [ -r ] f1 ... fn d2\n");
	exit(1);
}

copy(from, to)
	char *from, *to;
{
	int fold, fnew, n;
	char *last, destname[BSIZE], buf[BSIZE];
	struct stat stfrom, stto;

	fold = open(from, 0);
	if (fold < 0) {
		Perror(from);
		return (1);
	}
	if (fstat(fold, &stfrom) < 0) {
		Perror(from);
		(void) close(fold);
		return (1);
	}
	if (stat(to, &stto) >= 0 &&
	   (stto.st_mode&S_IFMT) == S_IFDIR) {
		last = rindex(from, '/');
		if (last) last++; else last = from;
		if (strlen(to) + strlen(last) >= BSIZE - 1) {
			fprintf(stderr, "cp: %s/%s: Name too long", to, last);
			(void) close(fold);
			return(1);
		}
		(void) sprintf(destname, "%s/%s", to, last);
		to = destname;
	}
	if (rflag && (stfrom.st_mode&S_IFMT) == S_IFDIR) {
		(void) close(fold);
		if (stat(to, &stto) < 0) {
			if (mkdir(to, (int)stfrom.st_mode) < 0) {
				Perror(to);
				return (1);
			}
		} else if ((stto.st_mode&S_IFMT) != S_IFDIR) {
			fprintf(stderr, "cp: %s: Not a directory.\n", to);
			return (1);
		}
		return (rcopy(from, to));
	}
	if (stat(to, &stto) >= 0) {
		if (stfrom.st_dev == stto.st_dev &&
		   stfrom.st_ino == stto.st_ino) {
			fprintf(stderr, "cp: Cannot copy file to itself.\n");
			(void) close(fold);
			return (1);
		}
		if (iflag) {
			int i, c;

			fprintf (stderr, "overwrite %s? ", to);
			i = c = getchar();
			while (c != '\n' && c != EOF)
				c = getchar();
			if (i != 'y') {
				(void) close(fold);
				return(1);
			}
		}
	}
	fnew = creat(to, (int)stfrom.st_mode);
	if (fnew < 0) {
		Perror(to);
		(void) close(fold); return(1);
	}
	for (;;) {
		n = read(fold, buf, BSIZE);
		if (n == 0)
			break;
		if (n < 0) {
			Perror(from);
			(void) close(fold); (void) close(fnew); return (1);
		}
		if (write(fnew, buf, n) != n) {
			Perror(to);
			(void) close(fold); (void) close(fnew); return (1);
		}
	}
	(void) close(fold); (void) close(fnew); return (0);
}

rcopy(from, to)
	char *from, *to;
{
	DIR *fold = opendir(from);
	struct direct *dp;
	int errs = 0;
	char fromname[BUFSIZ];

	if (fold == 0) {
		Perror(from);
		return (1);
	}
	for (;;) {
		dp = readdir(fold);
		if (dp == 0) {
			closedir(fold);
			return (errs);
		}
		if (dp->d_ino == 0)
			continue;
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (strlen(from) + 1 + strlen(dp->d_name) >= BUFSIZ - 1) {
			fprintf(stderr, "cp: %s/%s: Name too long.\n",
			    from, dp->d_name);
			errs++;
			continue;
		}
		(void) sprintf(fromname, "%s/%s", from, dp->d_name);
		errs += copy(fromname, to);
	}
}

Perror(s)
	char *s;
{

	fprintf(stderr, "cp: ");
	perror(s);
}
