/* Copyright (c) 1982 Regents of the University of California */

static char sccsid[] = "@(#)tfree.c 1.1 1/18/82";

/*
 * Free a tree; this is expensive but useful.
 */

#include "defs.h"
#include "tree.h"
#include "sym.h"
#include "tree.rep"

tfree(p)
register NODE *p;
{
	if (p == NIL) {
		return;
	}
	switch(degree(p->op)) {
		case LEAF:
			switch(p->op) {
				case O_CALL:
					tfree(p->left);
					tfree(p->right);
					break;

				case O_QLINE:
					dispose(p->left->sconval);
					dispose(p->left);
					tfree(p->right);
					break;

				case O_ALIAS:
					dispose(p->left->sconval);
					dispose(p->left);
					dispose(p->right->sconval);
					dispose(p->right);
					break;

				case O_SCON:
					unmkstring(p->nodetype);
					free(p->nodetype);
					free(p->sconval);
					p->sconval = NIL;
					break;
			}
			break;

		case BINARY:
			tfree(p->right);
			/* fall through */
		case UNARY:
			tfree(p->left);
			break;

		default:
			panic("bad op %d in tfree", p->op);
	}
	dispose(p);
}
