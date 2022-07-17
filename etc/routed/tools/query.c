#ifndef lint
static char sccsid[] = "@(#)query.c	4.5 10/7/82";
#endif

#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <net/in.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include "rip.h"

int	s;
char	packet[MAXPACKETSIZE];

main(argc, argv)
	int argc;
	char *argv[];
{
	int cc, count;
	struct sockaddr from;
	
	if (argc < 2) {
		printf("usage: query hosts...\n");
		exit(1);
	}
	s = socket(SOCK_DGRAM, 0, 0, 0);
	if (s < 0) {
		perror("socket");
		exit(2);
	}
	argv++, argc--;
	count = argc;
	while (argc > 0) {
		query(*argv);
		argv++, argc--;
	}

	/*
	 * Listen for returning packets
	 */
	while (count > 0) {
		cc = receive(s, &from, packet, sizeof (packet));
		if (cc <= 0) {
			if (cc < 0) {
				perror("receive");
				(void) close(s);
				exit(1);
			}
			continue;
		}
		rip_input(&from, cc);
		count--;
	}
}

query(host)
	char *host;
{
	struct sockaddr_in router;
	register struct rip *msg = (struct rip *)packet;
	struct hostent *hp;
	struct servent *sp;

	bzero((char *)&router, sizeof (router));
	hp = gethostbyname(host);
	if (hp == 0) {
		printf("%s: unknown\n", host);
		exit(1);
	}
	bcopy(hp->h_addr, &router.sin_addr, hp->h_length);
	router.sin_family = AF_INET;
	sp = getservbyname("router", "udp");
	if (sp == 0) {
		printf("udp/router: service unknown\n");
		exit(1);
	}
	router.sin_port = htons(sp->s_port);
	msg->rip_cmd = RIPCMD_REQUEST;
	msg->rip_nets[0].rip_dst.sa_family = AF_UNSPEC;
	msg->rip_nets[0].rip_metric = HOPCNT_INFINITY;
	if (send(s, &router, packet, sizeof (struct rip)) < 0)
		perror(host);
}

/*
 * Handle an incoming routing packet.
 */
rip_input(from, size)
	struct sockaddr_in *from;
	int size;
{
	register struct rip *msg = (struct rip *)packet;
	struct netinfo *n;
	char *name;
	struct hostent *hp;
	struct netent *np;

	if (msg->rip_cmd != RIPCMD_RESPONSE)
		return;
	hp = gethostbyaddr(&from->sin_addr, sizeof (struct in_addr), AF_INET);
	name = hp == 0 ? "???" : hp->h_name;
	printf("from %s(%x):\n", name, from->sin_addr);
	size -= sizeof (int);
	n = msg->rip_nets;
	while (size > 0) {
		register struct sockaddr_in *sin;

		if (size < sizeof (struct netinfo))
			break;
		sin = (struct sockaddr_in *)&n->rip_dst;
		if (inet_lnaof(sin->sin_addr) == INADDR_ANY) {
			np = getnetbyaddr(inet_netof(sin->sin_addr), AF_INET);
			name = np ? np->n_name : "???";
		} else {
			hp = gethostbyaddr(&sin->sin_addr,
				sizeof (struct in_addr), AF_INET);
			name = hp ? hp->h_name : "???";
		}
		printf("\t%s(%x), metric %d\n", name,
			sin->sin_addr, n->rip_metric);
		size -= sizeof (struct netinfo), n++;
	}
}
