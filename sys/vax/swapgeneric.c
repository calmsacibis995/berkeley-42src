/*	swapgeneric.c	6.1	83/08/05	*/

#include "mba.h"

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/systm.h"
#include "../h/reboot.h"

#include "../vax/cons.h"
#include "../vax/mtpr.h"
#include "../vaxmba/mbareg.h"
#include "../vaxmba/mbavar.h"
#include "../vaxuba/ubareg.h"
#include "../vaxuba/ubavar.h"

/*
 * Generic configuration;  all in one
 */
dev_t	rootdev, argdev, dumpdev;
int	nswap;
struct	swdevt swdevt[] = {
	{ -1,	1,	0 },
	{ 0,	0,	0 },
};
long	dumplo;
int	dmmin, dmmax, dmtext;

extern	struct mba_driver hpdriver;
extern	struct uba_driver scdriver;
extern	struct uba_driver hkdriver;
extern	struct uba_driver idcdriver;
extern	struct uba_driver hldriver;
extern	struct uba_driver udadriver;

struct	genericconf {
	caddr_t	gc_driver;
	char	*gc_name;
	dev_t	gc_root;
} genericconf[] = {
	{ (caddr_t)&hpdriver,	"hp",	makedev(0, 0),	},
	{ (caddr_t)&scdriver,	"up",	makedev(2, 0),	},
	{ (caddr_t)&udadriver,	"ra",	makedev(9, 0),	},
	{ (caddr_t)&idcdriver,	"rb",	makedev(11, 0),	},
	{ (caddr_t)&hldriver,	"rl",	makedev(14, 0),	},
	{ (caddr_t)&hkdriver,	"hk",	makedev(3, 0),	},
	{ 0 },
};

setconf()
{
	register struct mba_device *mi;
	register struct uba_device *ui;
	register struct genericconf *gc;
	int unit, swaponroot = 0;

	if (boothowto & RB_ASKNAME) {
		char name[128];
retry:
		printf("root device? ");
		gets(name);
		for (gc = genericconf; gc->gc_driver; gc++)
			if (gc->gc_name[0] == name[0] &&
			    gc->gc_name[1] == name[1])
				goto gotit;
		goto bad;
gotit:
		if (name[3] == '*') {
			name[3] = name[4];
			swaponroot++;
		}
		if (name[2] >= '0' && name[2] <= '7' && name[3] == 0) {
			unit = name[2] - '0';
			goto found;
		}
		printf("bad/missing unit number\n");
bad:
		printf("use hp%%d, up%%d, ra%%d, rb%%d, rl%%d or hk%%d\n");
		goto retry;
	}
	unit = 0;
	for (gc = genericconf; gc->gc_driver; gc++) {
		for (mi = mbdinit; mi->mi_driver; mi++) {
			if (mi->mi_alive == 0)
				continue;
			if (mi->mi_unit == 0 && mi->mi_driver ==
			    (struct mba_driver *)gc->gc_driver) {
				printf("root on %s0\n",
				    mi->mi_driver->md_dname);
				goto found;
			}
		}
		for (ui = ubdinit; ui->ui_driver; ui++) {
			if (ui->ui_alive == 0)
				continue;
			if (ui->ui_unit == 0 && ui->ui_driver ==
			    (struct uba_driver *)gc->gc_driver) {
				printf("root on %s0\n",
				    ui->ui_driver->ud_dname);
				goto found;
			}
		}
	}
	printf("no suitable root\n");
	asm("halt");
found:
	gc->gc_root = makedev(major(gc->gc_root), unit*8);
	rootdev = gc->gc_root;
	swdevt[0].sw_dev = argdev = dumpdev =
	    makedev(major(rootdev), minor(rootdev)+1);
	/* swap size and dumplo set during autoconfigure */
	if (swaponroot)
		rootdev = dumpdev;
}

getchar()
{
	register c;

	while ((mfpr(RXCS)&RXCS_DONE) == 0)
		;
	c = mfpr(RXDB)&0177;
	if (c == '\r')
		c = '\n';
	cnputc(c);
	return (c);
}

gets(cp)
	char *cp;
{
	register char *lp;
	register c;

	lp = cp;
	for (;;) {
		c = getchar() & 0177;
		switch (c) {
		case '\n':
		case '\r':
			*lp++ = '\0';
			return;
		case '\b':
		case '#':
			lp--;
			if (lp < cp)
				lp = cp;
			continue;
		case '@':
		case 'u'&037:
			lp = cp;
			cnputc('\n');
			continue;
		default:
			*lp++ = c;
		}
	}
}
