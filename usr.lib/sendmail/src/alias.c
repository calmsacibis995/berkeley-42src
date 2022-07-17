# include <pwd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <signal.h>
# include "sendmail.h"

# ifdef DBM
SCCSID(@(#)alias.c	4.1		7/25/83	(with DBM));
# else DBM
SCCSID(@(#)alias.c	4.1		7/25/83	(without DBM));
# endif DBM

/*
**  ALIAS -- Compute aliases.
**
**	Scans the alias file for an alias for the given address.
**	If found, it arranges to deliver to the alias list instead.
**	Uses libdbm database if -DDBM.
**
**	Parameters:
**		a -- address to alias.
**		sendq -- a pointer to the head of the send queue
**			to put the aliases in.
**
**	Returns:
**		none
**
**	Side Effects:
**		Aliases found are expanded.
**
**	Notes:
**		If NoAlias (the "-n" flag) is set, no aliasing is
**			done.
**
**	Deficiencies:
**		It should complain about names that are aliased to
**			nothing.
*/


#ifdef DBM
typedef struct
{
	char	*dptr;
	int	dsize;
} DATUM;
extern DATUM fetch();
#endif DBM

alias(a, sendq)
	register ADDRESS *a;
	ADDRESS **sendq;
{
	register char *p;
	extern char *aliaslookup();

	if (NoAlias)
		return;
# ifdef DEBUG
	if (tTd(27, 1))
		printf("alias(%s)\n", a->q_paddr);
# endif

	/* don't realias already aliased names */
	if (bitset(QDONTSEND, a->q_flags))
		return;

	CurEnv->e_to = a->q_paddr;

	/*
	**  Look up this name
	*/

	p = aliaslookup(a->q_user);
	if (p == NULL)
		return;

	/*
	**  Match on Alias.
	**	Deliver to the target list.
	*/

# ifdef DEBUG
	if (tTd(27, 1))
		printf("%s (%s, %s) aliased to %s\n",
		    a->q_paddr, a->q_host, a->q_user, p);
# endif
	message(Arpa_Info, "aliased to %s", p);
	AliasLevel++;
	sendtolist(p, a, sendq);
	AliasLevel--;
}
/*
**  ALIASLOOKUP -- look up a name in the alias file.
**
**	Parameters:
**		name -- the name to look up.
**
**	Returns:
**		the value of name.
**		NULL if unknown.
**
**	Side Effects:
**		none.
**
**	Warnings:
**		The return value will be trashed across calls.
*/

char *
aliaslookup(name)
	char *name;
{
# ifdef DBM
	DATUM rhs, lhs;

	/* create a key for fetch */
	lhs.dptr = name;
	lhs.dsize = strlen(name) + 1;
	rhs = fetch(lhs);
	return (rhs.dptr);
# else DBM
	register STAB *s;

	s = stab(name, ST_ALIAS, ST_FIND);
	if (s == NULL)
		return (NULL);
	return (s->s_alias);
# endif DBM
}
/*
**  INITALIASES -- initialize for aliasing
**
**	Very different depending on whether we are running DBM or not.
**
**	Parameters:
**		aliasfile -- location of aliases.
**		init -- if set and if DBM, initialize the DBM files.
**
**	Returns:
**		none.
**
**	Side Effects:
**		initializes aliases:
**		if DBM:  opens the database.
**		if ~DBM: reads the aliases into the symbol table.
*/

# define DBMMODE	0666

initaliases(aliasfile, init)
	char *aliasfile;
	bool init;
{
#ifdef DBM
	int atcnt;
	char buf[MAXNAME];
	time_t modtime;
	int (*oldsigint)();
#endif DBM
	struct stat stb;

	if (stat(aliasfile, &stb) < 0)
	{
		NoAlias = TRUE;
		errno = 0;
		return;
	}

# ifdef DBM
	/*
	**  Check to see that the alias file is complete.
	**	If not, we will assume that someone died, and it is up
	**	to us to rebuild it.
	*/

	dbminit(aliasfile);
	atcnt = 10;
	while (SafeAlias && !init && atcnt-- >= 0 && aliaslookup("@") == NULL)
		sleep(30);

	/*
	**  See if the DBM version of the file is out of date with
	**  the text version.  If so, go into 'init' mode automatically.
	**	This only happens if our effective userid owns the DBM
	**	version or if the mode of the database is 666 -- this
	**	is an attempt to avoid protection problems.  Note the
	**	unpalatable hack to see if the stat succeeded.
	*/

	modtime = stb.st_mtime;
	(void) strcpy(buf, aliasfile);
	(void) strcat(buf, ".pag");
	stb.st_ino = 0;
	if (!init && (atcnt < 0 || stat(buf, &stb) < 0 || stb.st_mtime < modtime))
	{
		errno = 0;
		if (AutoRebuild && stb.st_ino != 0 &&
		    ((stb.st_mode & 0777) == 0666 || stb.st_uid == geteuid()))
		{
			init = TRUE;
			message(Arpa_Info, "rebuilding alias database");
		}
		else
		{
			bool oldverb = Verbose;

			Verbose = TRUE;
			message(Arpa_Info, "Warning: alias database out of date");
			Verbose = oldverb;
		}
	}

	/*
	**  If initializing, create the new files.
	**	We should lock the alias file here to prevent other
	**	instantiations of sendmail from reading an incomplete
	**	file -- or worse yet, doing a concurrent initialize.
	*/

	if (init)
	{
		oldsigint = signal(SIGINT, SIG_IGN);
		(void) strcpy(buf, aliasfile);
		(void) strcat(buf, ".dir");
		if (close(creat(buf, DBMMODE)) < 0)
		{
			syserr("cannot make %s", buf);
			(void) signal(SIGINT, oldsigint);
			return;
		}
		(void) strcpy(buf, aliasfile);
		(void) strcat(buf, ".pag");
		if (close(creat(buf, DBMMODE)) < 0)
		{
			syserr("cannot make %s", buf);
			(void) signal(SIGINT, oldsigint);
			return;
		}
	}

	/*
	**  If necessary, load the DBM file.
	**	If running without DBM, load the symbol table.
	**	After loading the DBM file, add the distinquished alias "@".
	*/

	if (init)
	{
		DATUM key;

		readaliases(aliasfile, TRUE);
		key.dsize = 2;
		key.dptr = "@";
		store(key, key);
		(void) signal(SIGINT, oldsigint);
	}
# else DBM
	readaliases(aliasfile, init);
# endif DBM
}
/*
**  READALIASES -- read and process the alias file.
**
**	This routine implements the part of initaliases that occurs
**	when we are not going to use the DBM stuff.
**
**	Parameters:
**		aliasfile -- the pathname of the alias file master.
**		init -- if set, initialize the DBM stuff.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Reads aliasfile into the symbol table.
**		Optionally, builds the .dir & .pag files.
*/

static
readaliases(aliasfile, init)
	char *aliasfile;
	bool init;
{
	register char *p;
	char *p2;
	char *rhs;
	bool skipping;
	int naliases, bytes, longest;
	FILE *af;
	ADDRESS al, bl;
	register STAB *s;
	char line[BUFSIZ];

	if ((af = fopen(aliasfile, "r")) == NULL)
	{
# ifdef DEBUG
		if (tTd(27, 1))
			printf("Can't open %s\n", aliasfile);
# endif
		errno = 0;
		NoAlias++;
		return;
	}

	/*
	**  Read and interpret lines
	*/

	FileName = aliasfile;
	LineNumber = 0;
	naliases = bytes = longest = 0;
	skipping = FALSE;
	while (fgets(line, sizeof (line), af) != NULL)
	{
		int lhssize, rhssize;

		LineNumber++;
		switch (line[0])
		{
		  case '#':
		  case '\n':
		  case '\0':
			skipping = FALSE;
			continue;

		  case ' ':
		  case '\t':
			if (!skipping)
				syserr("Non-continuation line starts with space");
			skipping = TRUE;
			continue;
		}
		skipping = FALSE;

		/*
		**  Process the LHS
		**	Find the final colon, and parse the address.
		**	It should resolve to a local name -- this will
		**	be checked later (we want to optionally do
		**	parsing of the RHS first to maximize error
		**	detection).
		*/

		for (p = line; *p != '\0' && *p != ':' && *p != '\n'; p++)
			continue;
		if (*p++ != ':')
		{
			syserr("missing colon");
			continue;
		}
		if (parseaddr(line, &al, 1, ':') == NULL)
		{
			syserr("illegal alias name");
			continue;
		}

		/*
		**  Process the RHS.
		**	'al' is the internal form of the LHS address.
		**	'p' points to the text of the RHS.
		*/

		rhs = p;
		for (;;)
		{
			register char c;

			if (init)
			{
				/* do parsing & compression of addresses */
				c = *p;
				while (c != '\0')
				{
					p2 = p;
					while (*p != '\n' && *p != ',' && *p != '\0')
						p++;
					c = *p;
					*p++ = '\0';
					if (c == '\n')
						c = '\0';
					if (*p2 == '\0')
					{
						p[-1] = c;
						continue;
					}
					(void) parseaddr(p2, &bl, -1, ',');
					p[-1] = c;
					while (isspace(*p))
						p++;
				}
			}
			else
				p = &p[strlen(p)];

			/* see if there should be a continuation line */
			c = fgetc(af);
			if (!feof(af))
				(void) ungetc(c, af);
			if (c != ' ' && c != '\t')
				break;

			/* read continuation line */
			p--;
			if (fgets(p, sizeof line - (p - line), af) == NULL)
				break;
			LineNumber++;
		}
		if (al.q_mailer != LocalMailer)
		{
			syserr("cannot alias non-local names");
			continue;
		}

		/*
		**  Insert alias into symbol table or DBM file
		*/

		lhssize = strlen(al.q_user) + 1;
		rhssize = strlen(rhs) + 1;

# ifdef DBM
		if (init)
		{
			DATUM key, content;

			key.dsize = lhssize;
			key.dptr = al.q_user;
			content.dsize = rhssize;
			content.dptr = rhs;
			store(key, content);
		}
		else
# endif DBM
		{
			s = stab(al.q_user, ST_ALIAS, ST_ENTER);
			s->s_alias = newstr(rhs);
		}

		/* statistics */
		naliases++;
		bytes += lhssize + rhssize;
		if (rhssize > longest)
			longest = rhssize;
	}
	(void) fclose(af);
	CurEnv->e_to = NULL;
	FileName = NULL;
	message(Arpa_Info, "%d aliases, longest %d bytes, %d bytes total",
			naliases, longest, bytes);
}
/*
**  FORWARD -- Try to forward mail
**
**	This is similar but not identical to aliasing.
**
**	Parameters:
**		user -- the name of the user who's mail we would like
**			to forward to.  It must have been verified --
**			i.e., the q_home field must have been filled
**			in.
**		sendq -- a pointer to the head of the send queue to
**			put this user's aliases in.
**
**	Returns:
**		none.
**
**	Side Effects:
**		New names are added to send queues.
*/

forward(user, sendq)
	ADDRESS *user;
	ADDRESS **sendq;
{
	char buf[60];
	extern bool safefile();

# ifdef DEBUG
	if (tTd(27, 1))
		printf("forward(%s)\n", user->q_paddr);
# endif DEBUG

	if (user->q_mailer != LocalMailer || bitset(QBADADDR, user->q_flags))
		return;
# ifdef DEBUG
	if (user->q_home == NULL)
		syserr("forward: no home");
# endif DEBUG

	/* good address -- look for .forward file in home */
	define('z', user->q_home, CurEnv);
	expand("$z/.forward", buf, &buf[sizeof buf - 1], CurEnv);
	if (!safefile(buf, user->q_uid, S_IREAD))
		return;

	/* we do have an address to forward to -- do it */
	include(buf, "forwarding", user, sendq);
}
