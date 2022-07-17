/* Copyright (c) 1979 Regents of the University of California */

static	char sccsid[] = "@(#)p2put.c 1.14 9/5/83";

    /*
     *	functions to help pi put out
     *	polish postfix binary portable c compiler intermediate code
     *	thereby becoming the portable pascal compiler
     */

#include	"whoami.h"
#ifdef PC
#include	"0.h"
#include	"objfmt.h"
#include	"pcops.h"
#include	"pc.h"
#include	"align.h"
#include	"tmps.h"

    /*
     *	mash into f77's format
     *	lovely, isn't it?
     */
#define		TOF77( fop,val,rest )	( ( ( (rest) & 0177777 ) << 16 ) \
					| ( ( (val) & 0377 ) << 8 )	 \
					| ( (fop) & 0377 ) )

    /*
     *	emits an ftext operator and a string to the pcstream
     */
puttext( string )
    char	*string;
    {
	int	length = str4len( string );

	if ( !CGENNING )
	    return;
	p2word( TOF77( P2FTEXT , length , 0 ) );
#	ifdef DEBUG
	    if ( opt( 'k' ) ) {
		fprintf( stdout , "P2FTEXT | %3d | 0	" , length );
	    }
#	endif
	p2string( string );
    }

int
str4len( string )
    char	*string;
    {
	
	return ( ( strlen( string ) + 3 ) / 4 );
    }

    /*
     *	put formatted text into a buffer for printing to the pcstream.
     *	a call to putpflush actually puts out the text.
     *	none of arg1 .. arg5 need be present.
     *	and you can add more if you need them.
     */
    /* VARARGS */
putprintf( format , incomplete , arg1 , arg2 , arg3 , arg4 , arg5 )
    char	*format;
    int		incomplete;
    {
	static char	ppbuffer[ BUFSIZ ];
	static char	*ppbufp = ppbuffer;

	if ( !CGENNING )
	    return;
	sprintf( ppbufp , format , arg1 , arg2 , arg3 , arg4 , arg5 );
	ppbufp = &( ppbuffer[ strlen( ppbuffer ) ] );
	if ( ppbufp >= &( ppbuffer[ BUFSIZ ] ) )
	    panic( "putprintf" );
	if ( ! incomplete ) {
	    puttext( ppbuffer );
	    ppbufp = ppbuffer;
	}
    }

    /*
     *	emit a left bracket operator to pcstream
     *	with function number, the maximum temp register, and total local bytes
     */
putlbracket(ftnno, sizesp)
    int		ftnno;
    struct om	*sizesp;
{
    int	maxtempreg;	
    int	alignedframesize;

#   ifdef vax
	maxtempreg = sizesp->curtmps.next_avail[REG_GENERAL];
#   endif vax
#   ifdef mc68000
	    /*
	     *	this is how /lib/f1 wants it.
	     */
	maxtempreg =	(sizesp->curtmps.next_avail[REG_ADDR] << 4)
		      | (sizesp->curtmps.next_avail[REG_DATA]);
#   endif mc68000
    alignedframesize =
	roundup(BITSPERBYTE * -sizesp->curtmps.om_off, BITSPERBYTE * A_STACK);
    p2word( TOF77( P2FLBRAC , maxtempreg , ftnno ) );
    p2word(alignedframesize);
#   ifdef DEBUG
	if ( opt( 'k' ) ) {
	    fprintf(stdout, "P2FLBRAC | %3d | %d	%d\n",
		maxtempreg, ftnno, alignedframesize);
	}
#   endif
}

    /*
     *	emit a right bracket operator
     *	which for the binary interface
     *	forces the stack allocate and register mask
     */
putrbracket( ftnno )
    int	ftnno;
    {

	p2word( TOF77( P2FRBRAC , 0 , ftnno ) );
#	ifdef DEBUG
	    if ( opt( 'k' ) ) {
		fprintf( stdout , "P2FRBRAC |   0 | %d\n" , ftnno );
	    }
#	endif
    }

    /*
     *	emit an eof operator
     */
puteof()
    {
	
	p2word( P2FEOF );
#	ifdef DEBUG
	    if ( opt( 'k' ) ) {
		fprintf( stdout , "P2FEOF\n" );
	    }
#	endif
    }

    /*
     *	emit a dot operator,
     *	with a source file line number and name
     *	if line is negative, there was an error on that line, but who cares?
     */
putdot( filename , line )
    char	*filename;
    int		line;
    {
	int	length = str4len( filename );

	if ( line < 0 ) {
	    line = -line;
	}
	p2word( TOF77( P2FEXPR , length , line ) );
#	ifdef DEBUG
	    if ( opt( 'k' ) ) {
		fprintf( stdout , "P2FEXPR | %3d | %d	" , length , line );
	    }
#	endif
	p2string( filename );
    }

    /*
     *	put out a leaf node
     */
putleaf( op , lval , rval , type , name )
    int		op;
    int		lval;
    int		rval;
    int		type;
    char	*name;
    {
	if ( !CGENNING )
	    return;
	switch ( op ) {
	    default:
		panic( "[putleaf]" );
	    case P2ICON:
		p2word( TOF77( P2ICON , name != NIL , type ) );
		p2word( lval );
#		ifdef DEBUG
		    if ( opt( 'k' ) ) {
			fprintf( stdout , "P2ICON | %3d | 0x%x	" 
			       , name != NIL , type );
			fprintf( stdout , "%d\n" , lval );
		    }
#		endif
		if ( name )
		    p2name( name );
		break;
	    case P2NAME:
		p2word( TOF77( P2NAME , lval != 0 , type ) );
		if ( lval ) 
		    p2word( lval );
#		ifdef DEBUG
		    if ( opt( 'k' ) ) {
			fprintf( stdout , "P2NAME | %3d | 0x%x	" 
			       , lval != 0 , type );
			if ( lval )
			    fprintf( stdout , "%d	" , lval );
		    }
#		endif
		p2name( name );
		break;
	    case P2REG:
		p2word( TOF77( P2REG , rval , type ) );
#		ifdef DEBUG
		    if ( opt( 'k' ) ) {
			fprintf( stdout , "P2REG | %3d | 0x%x\n" ,
				rval , type );
		    }
#		endif
		break;
	}
    }

    /*
     *	rvalues are just lvalues with indirection, except
     *	special cases for registers and for named globals,
     *	whose names are their rvalues.
     */
putRV( name , level , offset , other_flags , type )
    char	*name;
    int		level;
    int		offset;
    char	other_flags;
    int		type;
    {
	char	extname[ BUFSIZ ];
	char	*printname;
	int	regnumber;

	if ( !CGENNING )
	    return;
	if ( other_flags & NREGVAR ) {
	    if ( ( offset < 0 ) || ( offset > P2FP ) ) {
		panic( "putRV regvar" );
	    }
	    putleaf( P2REG , 0 , offset , type , 0 );
	    return;
	}
	if ( whereis( level , offset , other_flags ) == GLOBALVAR ) {
	    if ( name != 0 ) {
		if ( name[0] != '_' ) {
			sprintf( extname , EXTFORMAT , name );
			printname = extname;
		} else {
			printname = name;
		}
		putleaf( P2NAME , offset , 0 , type , printname );
		return;
	    } else {
		panic( "putRV no name" );
	    }
	}
	putLV( name , level , offset , other_flags , type );
	putop( P2UNARY P2MUL , type );
    }

    /*
     *	put out an lvalue 
     *	given a level and offset
     *	special case for
     *	    named globals, whose lvalues are just their names as constants.
     */
putLV( name , level , offset , other_flags , type )
    char	*name;
    int		level;
    int		offset;
    char	other_flags;
    int		type;
{
    char		extname[ BUFSIZ ];
    char		*printname;

    if ( !CGENNING )
	return;
    if ( other_flags & NREGVAR ) {
	panic( "putLV regvar" );
    }
    switch ( whereis( level , offset , other_flags ) ) {
	case GLOBALVAR:
	    if ( ( name != 0 ) ) {
		if ( name[0] != '_' ) {
			sprintf( extname , EXTFORMAT , name );
			printname = extname;
		} else {
			printname = name;
		}
		putleaf( P2ICON , offset , 0 , ADDTYPE( type , P2PTR )
			, printname );
		return;
	    } else {
		panic( "putLV no name" );
	    }
	case PARAMVAR:
	    if ( level == cbn ) {
		putleaf( P2REG , 0 , P2AP , ADDTYPE( type , P2PTR ) , 0 );
	    } else {
		putleaf( P2NAME , (level * sizeof(struct dispsave)) + AP_OFFSET
		    , 0 , P2PTR | P2CHAR , DISPLAYNAME );
		parts[ level ] |= NONLOCALVAR;
	    }
	    putleaf( P2ICON , offset , 0 , P2INT , 0 );
	    putop( P2PLUS , P2PTR | P2CHAR );
	    break;
	case LOCALVAR:
	    if ( level == cbn ) {
		putleaf( P2REG , 0 , P2FP , ADDTYPE( type , P2PTR ) , 0 );
	    } else {
		putleaf( P2NAME , (level * sizeof(struct dispsave)) + FP_OFFSET
		    , 0 , P2PTR | P2CHAR , DISPLAYNAME );
		parts[ level ] |= NONLOCALVAR;
	    }
	    putleaf( P2ICON , -offset , 0 , P2INT , 0 );
	    putop( P2MINUS , P2PTR | P2CHAR );
	    break;
	case NAMEDLOCALVAR:
	    if ( level == cbn ) {
		putleaf( P2REG , 0 , P2FP , ADDTYPE( type , P2PTR ) , 0 );
	    } else {
		putleaf( P2NAME , (level * sizeof(struct dispsave)) + FP_OFFSET
		    , 0 , P2PTR | P2CHAR , DISPLAYNAME );
		parts[ level ] |= NONLOCALVAR;
	    }
	    putleaf( P2ICON , 0 , 0 , P2INT , name );
	    putop( P2MINUS , P2PTR | P2CHAR );
	    break;
    }
    return;
}

    /*
     *	put out a floating point constant leaf node
     *	the constant is declared in aligned data space
     *	and a P2NAME leaf put out for it
     */
putCON8( val )
    double	val;
    {
	int	label;
	char	name[ BUFSIZ ];

	if ( !CGENNING )
	    return;
	label = getlab();
	putprintf( "	.data" , 0 );
	aligndot(A_DOUBLE);
	putlab( label );
#	ifdef vax
	    putprintf( "	.double 0d%.20e" , 0 , val );
#	endif vax
#	ifdef mc68000
	    putprintf( "	.long 	0x%x,0x%x", 0, val);
#	endif mc68000
	putprintf( "	.text" , 0 );
	sprintf( name , PREFIXFORMAT , LABELPREFIX , label );
	putleaf( P2NAME , 0 , 0 , P2DOUBLE , name );
    }

	/*
	 * put out either an lvalue or an rvalue for a constant string.
	 * an lvalue (for assignment rhs's) is the name as a constant, 
	 * an rvalue (for parameters) is just the name.
	 */
putCONG( string , length , required )
    char	*string;
    int		length;
    int		required;
    {
	char	name[ BUFSIZ ];
	int	label;
	char	*cp;
	int	pad;
	int	others;

	if ( !CGENNING )
	    return;
	putprintf( "	.data" , 0 );
	aligndot(A_STRUCT);
	label = getlab();
	putlab( label );
	cp = string;
	while ( *cp ) {
	    putprintf( "	.byte	0%o" , 1 , *cp ++ );
	    for ( others = 2 ; ( others <= 8 ) && *cp ; others ++ ) {
		putprintf( ",0%o" , 1 , *cp++ );
	    }
	    putprintf( "" , 0 );
	}
	pad = length - strlen( string );
	while ( pad-- > 0 ) {
	    putprintf( "	.byte	0%o" , 1 , ' ' );
	    for ( others = 2 ; ( others <= 8 ) && ( pad-- > 0 ) ; others++ ) {
		putprintf( ",0%o" , 1 , ' ' );
	    }
	    putprintf( "" , 0 );
	}
	putprintf( "	.byte	0" , 0 );
	putprintf( "	.text"  , 0 );
	sprintf( name , PREFIXFORMAT , LABELPREFIX , label );
	if ( required == RREQ ) {
	    putleaf( P2NAME , 0 , 0 , P2ARY | P2CHAR , name );
	} else {
	    putleaf( P2ICON , 0 , 0 , P2PTR | P2CHAR , name );
	}
    }

    /*
     *	map a pascal type to a c type
     *	this would be tail recursive, but i unfolded it into a for (;;).
     *	this is sort of like isa and lwidth
     *	a note on the types used by the portable c compiler:
     *	    they are divided into a basic type (char, short, int, long, etc.)
     *	    and qualifications on those basic types (pointer, function, array).
     *	    the basic type is kept in the low 4 bits of the type descriptor,
     *	    and the qualifications are arranged in two bit chunks, with the
     *	    most significant on the right,
     *	    and the least significant on the left
     *		e.g. int *foo();
     *			(a function returning a pointer to an integer)
     *		is stored as
     *		    <ptr><ftn><int>
     *	so, we build types recursively
     *	also, we know that /lib/f1 can only deal with 6 qualifications
     *	so we stop the recursion there.  this stops infinite type recursion
     *	through mutually recursive pointer types.
     */
#define	MAXQUALS	6
int
p2type( np )
{

    return typerecur( np , 0 );
}
typerecur( np , quals )
    struct nl	*np;
    int		quals;
    {
	
	if ( np == NIL || quals > MAXQUALS ) {
	    return P2UNDEF;
	}
	switch ( np -> class ) {
	    case SCAL :
	    case RANGE :
		if ( np -> type == ( nl + TDOUBLE ) ) {
		    return P2DOUBLE;
		}
		switch ( bytes( np -> range[0] , np -> range[1] ) ) {
		    case 1:
			return P2CHAR;
		    case 2:
			return P2SHORT;
		    case 4:
			return P2INT;
		    default:
			panic( "p2type int" );
		}
	    case STR :
		return ( P2ARY | P2CHAR );
	    case RECORD :
	    case SET :
		return P2STRTY;
	    case FILET :
		return ( P2PTR | P2STRTY );
	    case CONST :
	    case VAR :
	    case FIELD :
		return p2type( np -> type );
	    case TYPE :
		switch ( nloff( np ) ) {
		    case TNIL :
			return ( P2PTR | P2UNDEF );
		    case TSTR :
			return ( P2ARY | P2CHAR );
		    case TSET :
			return P2STRTY;
		    default :
			return ( p2type( np -> type ) );
		}
	    case REF:
	    case WITHPTR:
	    case PTR :
		return ADDTYPE( typerecur( np -> type , quals + 1 ) , P2PTR );
	    case ARRAY :
		return ADDTYPE( typerecur( np -> type , quals + 1 ) , P2ARY );
	    case FUNC :
		    /*
		     * functions are really pointers to functions
		     * which return their underlying type.
		     */
		return ADDTYPE( ADDTYPE( typerecur( np -> type , quals + 2 ) ,
					P2FTN ) , P2PTR );
	    case PROC :
		    /*
		     * procedures are pointers to functions 
		     * which return integers (whether you look at them or not)
		     */
		return ADDTYPE( ADDTYPE( P2INT , P2FTN ) , P2PTR );
	    case FFUNC :
	    case FPROC :
		    /*
		     *	formal procedures and functions are pointers
		     *	to structures which describe their environment.
		     */
		return ( P2PTR | P2STRTY );
	    default :
		panic( "p2type" );
	}
    }

    /*
     *	add a most significant type modifier to a type
     */
long
addtype( underlying , mtype )
    long	underlying;
    long	mtype;
    {
	return ( ( ( underlying & ~P2BASETYPE ) << P2TYPESHIFT )
	       | mtype
	       | ( underlying & P2BASETYPE ) );
    }

    /*
     *	put a typed operator to the pcstream
     */
putop( op , type )
    int		op;
    int		type;
    {
	extern char	*p2opnames[];
	
	if ( !CGENNING )
	    return;
	p2word( TOF77( op , 0 , type ) );
#	ifdef DEBUG
	    if ( opt( 'k' ) ) {
		fprintf( stdout , "%s (%d) |   0 | 0x%x\n"
			, p2opnames[ op ] , op , type );
	    }
#	endif
    }

    /*
     *	put out a structure operator (STASG, STARG, STCALL, UNARY STCALL )
     *	which looks just like a regular operator, only the size and
     *	alignment go in the next consecutive words
     */
putstrop( op , type , size , alignment )
    int	op;
    int	type;
    int	size;
    int	alignment;
    {
	extern char	*p2opnames[];
	
	if ( !CGENNING )
	    return;
	p2word( TOF77( op , 0 , type ) );
	p2word( size );
	p2word( alignment );
#	ifdef DEBUG
	    if ( opt( 'k' ) ) {
		fprintf( stdout , "%s (%d) |   0 | 0x%x	%d %d\n"
			, p2opnames[ op ] , op , type , size , alignment );
	    }
#	endif
    }

    /*
     *	the string names of p2ops
     */
char	*p2opnames[] = {
	"",
	"P2UNDEFINED",		/* 1 */
	"P2NAME",		/* 2 */
	"P2STRING",		/* 3 */
	"P2ICON",		/* 4 */
	"P2FCON",		/* 5 */
	"P2PLUS",		/* 6 */
	"",
	"P2MINUS",		/* 8		also unary == P2NEG */
	"",
	"P2NEG",
	"P2MUL",		/* 11		also unary == P2INDIRECT */
	"",
	"P2INDIRECT",
	"P2AND",		/* 14		also unary == P2ADDROF */
	"",
	"P2ADDROF",
	"P2OR",			/* 17 */
	"",
	"P2ER",			/* 19 */
	"",
	"P2QUEST",		/* 21 */
	"P2COLON",		/* 22 */
	"P2ANDAND",		/* 23 */
	"P2OROR",		/* 24 */
	"",			/* 25 */
	"",			/* 26 */
	"",			/* 27 */
	"",			/* 28 */
	"",			/* 29 */
	"",			/* 30 */
	"",			/* 31 */
	"",			/* 32 */
	"",			/* 33 */
	"",			/* 34 */
	"",			/* 35 */
	"",			/* 36 */
	"",			/* 37 */
	"",			/* 38 */
	"",			/* 39 */
	"",			/* 40 */
	"",			/* 41 */
	"",			/* 42 */
	"",			/* 43 */
	"",			/* 44 */
	"",			/* 45 */
	"",			/* 46 */
	"",			/* 47 */
	"",			/* 48 */
	"",			/* 49 */
	"",			/* 50 */
	"",			/* 51 */
	"",			/* 52 */
	"",			/* 53 */
	"",			/* 54 */
	"",			/* 55 */
	"P2LISTOP",		/* 56 */
	"",
	"P2ASSIGN",		/* 58 */
	"P2COMOP",		/* 59 */
	"P2DIV",		/* 60 */
	"",
	"P2MOD",		/* 62 */
	"",
	"P2LS",			/* 64 */
	"",
	"P2RS",			/* 66 */
	"",
	"P2DOT",		/* 68 */
	"P2STREF",		/* 69 */
	"P2CALL",		/* 70		also unary */
	"",
	"P2UNARYCALL",
	"P2FORTCALL",		/* 73		also unary */
	"",
	"P2UNARYFORTCALL",
	"P2NOT",		/* 76 */
	"P2COMPL",		/* 77 */
	"P2INCR",		/* 78 */
	"P2DECR",		/* 79 */
	"P2EQ",			/* 80 */
	"P2NE",			/* 81 */
	"P2LE",			/* 82 */
	"P2LT",			/* 83 */
	"P2GE",			/* 84 */
	"P2GT",			/* 85 */
	"P2ULE",		/* 86 */
	"P2ULT",		/* 87 */
	"P2UGE",		/* 88 */
	"P2UGT",		/* 89 */
	"P2SETBIT",		/* 90 */
	"P2TESTBIT",		/* 91 */
	"P2RESETBIT",		/* 92 */
	"P2ARS",		/* 93 */
	"P2REG",		/* 94 */
	"P2OREG",		/* 95 */
	"P2CCODES",		/* 96 */
	"P2FREE",		/* 97 */
	"P2STASG",		/* 98 */
	"P2STARG",		/* 99 */
	"P2STCALL",		/* 100		also unary */
	"",
	"P2UNARYSTCALL",
	"P2FLD",		/* 103 */
	"P2SCONV",		/* 104 */
	"P2PCONV",		/* 105 */
	"P2PMCONV",		/* 106 */
	"P2PVCONV",		/* 107 */
	"P2FORCE",		/* 108 */
	"P2CBRANCH",		/* 109 */
	"P2INIT",		/* 110 */
	"P2CAST",		/* 111 */
    };

    /*
     *	low level routines
     */

    /*
     *	puts a long word on the pcstream
     */
p2word( word )
    long	word;
    {

	putw( word , pcstream );
    }

    /*
     *	put a length 0 mod 4 null padded string onto the pcstream
     */
p2string( string )
    char	*string;
    {
	int	slen = strlen( string );
	int	wlen = ( slen + 3 ) / 4;
	int	plen = ( wlen * 4 ) - slen;
	char	*cp;
	int	p;

	for ( cp = string ; *cp ; cp++ )
	    putc( *cp , pcstream );
	for ( p = 1 ; p <= plen ; p++ )
	    putc( '\0' , pcstream );
#	ifdef DEBUG
	    if ( opt( 'k' ) ) {
		fprintf( stdout , "\"%s" , string );
		for ( p = 1 ; p <= plen ; p++ )
		    fprintf( stdout , "\\0" );
		fprintf( stdout , "\"\n" );
	    }
#	endif
    }

    /*
     *	puts a name on the pcstream
     */
p2name( name )
    char	*name;
    {
	int	pad;

	fprintf( pcstream , NAMEFORMAT , name );
	pad = strlen( name ) % sizeof (long);
	for ( ; pad < sizeof (long) ; pad++ ) {
	    putc( '\0' , pcstream );
	}
#	ifdef DEBUG
	    if ( opt( 'k' ) ) {
		fprintf( stdout , NAMEFORMAT , name );
		pad = strlen( name ) % sizeof (long);
		for ( ; pad < sizeof (long) ; pad++ ) {
		    fprintf( stdout , "\\0" );
		}
		fprintf( stdout , "\n" );
	    }
#	endif
    }
    
    /*
     *	put out a jump to a label
     */
putjbr( label )
    long	label;
    {

	printjbr( LABELPREFIX , label );
    }

    /*
     *	put out a jump to any kind of label
     */
printjbr( prefix , label )
    char	*prefix;
    long	label;
    {

#	ifdef vax
	    putprintf( "	jbr	" , 1 );
	    putprintf( PREFIXFORMAT , 0 , prefix , label );
#	endif vax
#	ifdef mc68000
	    putprintf( "	jra	" , 1 );
	    putprintf( PREFIXFORMAT , 0 , prefix , label );
#	endif mc68000
    }

    /*
     *	another version of put to catch calls to put
     */
put( arg1 , arg2 )
    {

	panic("put()");
    }

#endif PC
