/* Copyright (c) 1980 Regents of the University of California */

static char sccsid[] = "@(#)pccaseop.c 1.12 6/10/83";

#include "whoami.h"
#ifdef PC
    /*
     *	and the rest of the file
     */
#include "0.h"
#include "tree.h"
#include "objfmt.h"
#include "pcops.h"
#include "pc.h"
#include "tmps.h"

    /*
     *	structure for a case: 
     *	    its constant label, line number (for errors), and location label.
     */
struct ct {
    long	cconst;
    int		cline;
    int		clabel;
};

    /*
     *	the P2FORCE operator puts its operand into a register.
     *	these to keep from thinking of it as r0 all over.
     */
#ifdef vax
#   define	FORCENAME	"r0"
#endif vax
#ifdef mc68000
#   define	FORCENAME	"d0"
#   define	ADDRTEMP	"a0"
#endif mc68000

    /*
     *	given a tree for a case statement, generate code for it.
     *	this computes the expression into a register,
     *	puts down the code for each of the cases,
     *	and then decides how to do the case switching.
     *	tcase	[0]	T_CASE
     *		[1]	lineof "case"
     *		[2]	expression
     *		[3]	list of cased statements:
     *			cstat	[0]	T_CSTAT
     *				[1]	lineof ":"
     *				[2]	list of constant labels
     *				[3]	statement
     */
pccaseop( tcase )
    int	*tcase;
{
    struct nl	*exprtype;
    struct nl	*exprnlp;
    struct nl	*rangetype;
    long	low;
    long	high;
    long	exprctype;
    long	swlabel;
    long	endlabel;
    long	label;
    long	count;
    long	*cstatlp;
    long	*cstatp;
    long	*casep;
    struct ct	*ctab;
    struct ct	*ctp;
    long	i;
    long	nr;
    long	goc;
    int		casecmp();
    bool	dupcases;

    goc = gocnt;
	/*
	 *  find out the type of the case expression
	 *  even if the expression has errors (exprtype == NIL), continue.
	 */
    line = tcase[1];
    codeoff();
    exprtype = rvalue( (int *) tcase[2] , NIL  , RREQ );
    codeon();
    if ( exprtype != NIL ) {
	if ( isnta( exprtype , "bcsi" ) ) {
	    error("Case selectors cannot be %ss" , nameof( exprtype ) );
	    exprtype = NIL;
	} else {
	    if ( exprtype -> class != RANGE ) {
		rangetype = exprtype -> type;
	    } else {
		rangetype = exprtype;
	    }
	    if ( rangetype == NIL ) {
		exprtype = NIL;
	    } else {
		low = rangetype -> range[0];
		high = rangetype -> range[1];
	    }
	}
    }
    if ( exprtype != NIL ) {
	    /*
	     *	compute and save the case expression.
	     *	also, put expression into a register
	     *	save its c-type and jump to the code to do the switch.
	     */
	exprctype = p2type( exprtype );
	exprnlp = tmpalloc( sizeof (long) , nl + T4INT , NOREG );
	putRV( 0 , cbn , exprnlp -> value[ NL_OFFS ] ,
			exprnlp -> extra_flags , P2INT );
	(void) rvalue( (int *) tcase[2] , NIL , RREQ );
	sconv(exprctype, P2INT);
	putop( P2ASSIGN , P2INT );
	putop( P2FORCE , P2INT );
	putdot( filename , line );
	swlabel = getlab();
	putjbr( swlabel );
    }
	/*
	 *  count the number of cases
	 *  and allocate table for cases, lines, and labels
	 *  default case goes in ctab[0].
	 */
    count = 1;
    for ( cstatlp = tcase[3] ; cstatlp != NIL ; cstatlp = cstatlp[2] ) {
	cstatp = cstatlp[1];
	if ( cstatp == NIL ) {
	    continue;
	}
	for ( casep = cstatp[2] ; casep != NIL ; casep = casep[2] ) {
	    count++;
	}
    }
	/*
	 */
    ctab = (struct ct *) malloc( count * sizeof( struct ct ) );
    if ( ctab == (struct ct *) 0 ) {
	error("Ran out of memory (case)");
	pexit( DIED );
    }
	/*
	 *  pick up default label and label for after case statement.
	 */
    ctab[0].clabel = getlab();
    endlabel = getlab();
	/*
	 *  generate code for each case
	 *  filling in ctab for each.
	 *  nr is for error if no case falls out bottom.
	 */
    nr = 1;
    count = 0;
    for ( cstatlp = tcase[3] ; cstatlp != NIL ; cstatlp = cstatlp[2] ) {
	cstatp = cstatlp[1];
	if ( cstatp == NIL ) {
	    continue;
	}
	line = cstatp[1];
	label = getlab();
	for ( casep = cstatp[2] ; casep != NIL ; casep = casep[2] ) {
	    gconst( casep[1] );
	    if( exprtype == NIL || con.ctype == NIL ) {
		continue;
	    }
	    if ( incompat( con.ctype , exprtype , NIL ) ) {
		cerror("Case label type clashed with case selector expression type");
		continue;
	    }
	    if ( con.crval < low || con.crval > high ) {
		error("Case label out of range");
		continue;
	    }
	    count++;
	    ctab[ count ].cconst = con.crval;
	    ctab[ count ].cline = line;
	    ctab[ count ].clabel = label;
	}
	    /*
	     *	put out the statement
	     */
	putlab( label );
	putcnt();
	level++;
	statement( cstatp[3] );
	nr = (nr && noreach);
	noreach = 0;
	level--;
	if (gotos[cbn]) {
		ungoto();
	}
	putjbr( endlabel );
    }
    noreach = nr;
	/*
	 *	default action is to call error
	 */
    putlab( ctab[0].clabel );
    putleaf( P2ICON , 0 , 0 , ADDTYPE( P2FTN | P2INT , P2PTR ) , "_CASERNG" );
    putRV( 0 , cbn , exprnlp -> value[ NL_OFFS ] ,
		    exprnlp -> extra_flags , P2INT );
    putop( P2CALL , P2INT );
    putdot( filename , line );
	/*
	 *  sort the cases
	 */
    qsort( &ctab[1] , count , sizeof (struct ct) , casecmp );
	/*
	 *  check for duplicates
	 */
    dupcases = FALSE;
    for ( ctp = &ctab[1] ; ctp < &ctab[ count ] ; ctp++ ) {
	if ( ctp[0].cconst == ctp[1].cconst ) {
	    error("Multiply defined label in case, lines %d and %d" ,
		    ctp[0].cline , ctp[1].cline );
	    dupcases = TRUE;
	}
    }
    if ( dupcases ) {
	return;
    }
	/*
	 *  choose a switch algorithm and implement it:
	 *	direct switch	>= 1/3 full and >= 4 cases.
	 *	binary switch	not direct switch and > 8 cases.
	 *	ifthenelse	not direct or binary switch.
	 */
    putlab( swlabel );
    if ( ctab[ count ].cconst - ctab[1].cconst < 3 * count && count >= 4 ) {
	directsw( ctab , count );
    } else if ( count > 8 ) {
	binarysw( ctab , count );
    } else {
	itesw( ctab , count );
    }
    putlab( endlabel );
    if ( goc != gocnt ) {
	    putcnt();
    }
}

    /*
     *	direct switch
     */
directsw( ctab , count )
    struct ct	*ctab;
    int		count;
{
    int		fromlabel = getlab();
    long	i;
    long	j;

#   ifdef vax
	if (opt('J')) {
	    /*
	     *	We have a table of absolute addresses.
	     *
	     *	subl2	to make r0 a 0-origin byte offset.
	     *	cmpl	check against upper limit.
	     *	blssu	error if out of bounds.
	     *	ashl	to make r0 a 0-origin long offset,
	     *	jmp	and indirect through it.
	     */
	    putprintf("	subl2	$%d,%s", 0, ctab[1].cconst, FORCENAME);
	    putprintf("	cmpl	$%d,%s", 0,
		    ctab[count].cconst - ctab[1].cconst, FORCENAME);
	    putprintf("	blssu	%s%d", 0, LABELPREFIX, ctab[0].clabel);
	    putprintf("	ashl	$2,%s,%s", 0, FORCENAME, FORCENAME);
	    putprintf("	jmp	*%s%d(%s)", 0,
		    LABELPREFIX, fromlabel, FORCENAME);
	} else {
	    /*
	     *	We can use the VAX casel instruction with a table
	     *	of short relative offsets.
	     */
	    putprintf("	casel	%s,$%d,$%d" , 0 , FORCENAME ,
		    ctab[1].cconst , ctab[ count ].cconst - ctab[1].cconst );
	}
#   endif vax
#   ifdef mc68000
	/*
	 *	subl	to make d0 a 0-origin byte offset.
	 *	cmpl	check against upper limit.
	 *	bhi	error if out of bounds.
	 */
	putprintf("	subl	#%d,%s", 0, ctab[1].cconst, FORCENAME);
	putprintf("	cmpl	#%d,%s", 0,
		ctab[count].cconst - ctab[1].cconst, FORCENAME);
	putprintf("	bhi	%s%d", 0, LABELPREFIX, ctab[0].clabel);
	if (opt('J')) {
	    /*
	     *	We have a table of absolute addresses.
	     *
	     *	asll	to make d0 a 0-origin long offset.
	     *	movl	pick up a jump-table entry
	     *	jmp	and indirect through it.
	     */
	    putprintf("	asll	#2,%s", 0, FORCENAME, FORCENAME);
	    putprintf("	movl	pc@(4,%s:l),%s", 0, FORCENAME, ADDRTEMP);
	    putprintf("	jmp	%s@", 0, ADDRTEMP);
	} else {
	    /*
	     *	We have a table of relative addresses.
	     *
	     *	addw	to make d0 a 0-origin word offset.
	     *	movw	pick up a jump-table entry
	     *	jmp	and indirect through it.
	     */
	    putprintf("	addw	%s,%s", 0, FORCENAME, FORCENAME);
	    putprintf("	movw	pc@(6,%s:w),%s", 0, FORCENAME, FORCENAME);
	    putprintf("	jmp	pc@(2,%s:w)", 0, FORCENAME);
	}
#   endif mc68000
    putlab( fromlabel );
    i = 1;
    j = ctab[1].cconst;
    while ( i <= count ) {
	if ( j == ctab[ i ].cconst ) {
	    if (opt('J')) {
		putprintf( "	.long	" , 1 );
		putprintf( PREFIXFORMAT , 0 , LABELPREFIX , ctab[ i ].clabel );
	    } else {
		putprintf( "	.word	" , 1 );
		putprintf( PREFIXFORMAT , 1 , LABELPREFIX , ctab[ i ].clabel );
		putprintf( "-" , 1 );
		putprintf( PREFIXFORMAT , 0 , LABELPREFIX , fromlabel );
	    }
	    i++;
	} else {
	    if (opt('J')) {
		putprintf( "	.long	" , 1 );
		putprintf( PREFIXFORMAT , 0 , LABELPREFIX , ctab[ 0 ].clabel );
	    } else {
		putprintf( "	.word	" , 1 );
		putprintf( PREFIXFORMAT , 1 , LABELPREFIX , ctab[ 0 ].clabel );
		putprintf( "-" , 1 );
		putprintf( PREFIXFORMAT , 0 , LABELPREFIX , fromlabel );
	    }
	}
	j++;
    }
#   ifdef vax
	    /*
	     *	execution continues here if value not in range of case.
	     */
	if (!opt('J'))
	    putjbr( ctab[0].clabel );
#   endif vax
}

    /*
     *	binary switch
     *	special case out default label and start recursion.
     */
binarysw( ctab , count )
    struct ct	*ctab;
    int		count;
{
    
    bsrecur( ctab[0].clabel , &ctab[0] , count );
}

    /*
     *	recursive log( count ) search.
     */
bsrecur( deflabel , ctab , count )
    int		deflabel;
    struct ct	*ctab;
    int		count;
{

    if ( count <= 0 ) {
	putjbr(deflabel);
	return;
    } else if ( count == 1 ) {
#	ifdef vax
	    putprintf("	cmpl	%s,$%d", 0, FORCENAME, ctab[1].cconst);
	    putprintf("	jeql	%s%d", 0, LABELPREFIX, ctab[1].clabel);
	    putjbr(deflabel);
#	endif vax
#	ifdef mc68000
	    putprintf("	cmpl	#%d,%s", 0, ctab[1].cconst, FORCENAME);
	    putprintf("	jeq	L%d", 0, LABELPREFIX, ctab[1].clabel);
	    putjbr(deflabel);
#	endif mc68000
	return;
    } else {
	int	half = ( count + 1 ) / 2;
	int	gtrlabel = getlab();

#	ifdef vax
	    putprintf("	cmpl	%s,$%d", 0, FORCENAME, ctab[half].cconst);
	    putprintf("	jgtr	%s%d", 0, LABELPREFIX, gtrlabel);
	    putprintf("	jeql	%s%d", 0, LABELPREFIX, ctab[half].clabel);
#	endif vax
#	ifdef mc68000
	    putprintf("	cmpl	#%d,%s", 0, ctab[half].cconst, FORCENAME);
	    putprintf("	jgt	%s%d", 0, LABELPREFIX, gtrlabel);
	    putprintf("	jeq	%s%d", 0, LABELPREFIX, ctab[half].clabel);
#	endif mc68000
	bsrecur( deflabel , &ctab[0] , half - 1 );
	putlab(gtrlabel);
	bsrecur( deflabel , &ctab[ half ] , count - half );
	return;
    }
}

itesw( ctab , count )
    struct ct	*ctab;
    int		count;
{
    int	i;

    for ( i = 1 ; i <= count ; i++ ) {
#	ifdef vax
	    putprintf("	cmpl	%s,$%d", 0, FORCENAME, ctab[i].cconst);
	    putprintf("	jeql	%s%d", 0, LABELPREFIX, ctab[i].clabel);
#	endif vax
#	ifdef mc68000
	    putprintf("	cmpl	#%d,%s", 0, ctab[i].cconst, FORCENAME);
	    putprintf("	jeq	%s%d", 0, LABELPREFIX, ctab[i].clabel);
#	endif mc68000
    }
    putjbr(ctab[0].clabel);
    return;
}
int
casecmp( this , that )
    struct ct 	*this;
    struct ct 	*that;
{
    if ( this -> cconst < that -> cconst ) {
	return -1;
    } else if ( this -> cconst > that -> cconst ) {
	return 1;
    } else {
	return 0;
    }
}
#endif PC
