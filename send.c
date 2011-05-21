/*** IPPROTO_RAW sender ***/
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ACK_FLAG	0x02		/* Mascara para deteccion de ACK */
#define ORIG "127.0.0.3"
#define DEST "127.0.0.1"
struct rdt_header{ 
  unsigned char        srcPort;
  unsigned char        destPort;
  unsigned char        flags_rdt;         /* Flags para control de rdt TIPo -> 0x80, SYN -> 0x01, ACK ->0x02, FIN ->0x04 */
  unsigned char        nro_SEC_rdt;       /* nro de secuencia de rdt*/
  unsigned char        nro_ACK_rdt;       /* nro de ACK de rdt*/
};

int main(void)
{

	int s;
	struct sockaddr_in daddr;
	struct sockaddr_in saddr;

	char* packet = (char*) malloc(sizeof(char)*50);
	memset(packet, 'b', 49);
	memset(packet+50, '\0', 1);
//int i=0;
//for(i=0;i<50;i++)
//printf("%c \n",packet[i]);
	/* point the iphdr to the beginning of the packet */
printf("%d\n",strlen(packet));
	struct iphdr *ip = (struct iphdr *)packet;  
	struct rdt_header *rdt = (struct rdt_header*) (packet + sizeof(struct iphdr));

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("error:");
		exit(EXIT_FAILURE);
	}
	//el tamaño del ipheader es 20 bytes

printf("%u\n",strlen(packet));
	//lleno la estructura con la direccion del source
	saddr.sin_family = AF_INET;
	saddr.sin_port = 0; /* not needed in SOCK_RAW */
	inet_pton(AF_INET, ORIG, (struct in_addr *)&saddr.sin_addr.s_addr);
	memset(saddr.sin_zero, 0, sizeof(saddr.sin_zero));
	//lleno la estructura con la direccion del destino
	daddr.sin_family = AF_INET;
	daddr.sin_port = 0; /* not needed in SOCK_RAW */
	inet_pton(AF_INET, DEST, (struct in_addr *)&daddr.sin_addr.s_addr);
	memset(daddr.sin_zero, 0, sizeof(daddr.sin_zero));

	//seteo los datos del datagrama
	memset(packet+sizeof(struct iphdr) + sizeof(struct rdt_header), 'A', 50 - sizeof (struct iphdr)-sizeof(struct rdt_header));   /* payload will be all As */
	//seteo el header IP
	ip->ihl = 5;
	ip->version = 4;
	ip->tos = 0;
	ip->tot_len = htons(50);	/* tamaño total del datagrama, headerIP + datos*/
	ip->frag_off = 0;		/* no fragment */
	ip->ttl = 64;			/* default value */
	ip->protocol = IPPROTO_RAW;	/* protocol at L4 */
	ip->check = 0;			/* not needed in iphdr */
	ip->saddr = saddr.sin_addr.s_addr; /* direccion del source */
	ip->daddr = daddr.sin_addr.s_addr; /* direccion del destination */

	//seteo el rdt_header
	rdt->srcPort=3;
unsigned char p = '8';
	rdt->destPort=1;
	rdt->flags_rdt=0x01;
	rdt->nro_SEC_rdt=0;
	rdt->nro_ACK_rdt=0;
printf("%d\n",	rdt->destPort);
printf("%d\n",	rdt->srcPort);
//int i=0;
//for(i=0;i<50;i++)
//printf("%c \n",packet[i]);
	//while(1) {
		//sleep(1);
		//parametros: socket, datagrama, bandera, direccion de destino, largo de la direccion del destino
		if (sendto(s, (char *)packet,50, 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
			perror("packet send error:");
//}
/*	char packet2[50];
//	socklen_t *len = (socklen_t *)sizeof(saddr);
	int fromlen = sizeof(daddr);
		recvfrom(s, (char *)&packet2,50, 0,(struct sockaddr *)&daddr, (socklen_t *)&fromlen);
		struct iphdr *ip2 = (struct iphdr *)packet2;  
		struct rdt_header *rdt2 = (struct rdt_header*) (packet2 + sizeof(struct iphdr));

printf("%c\n",	rdt2->flags_rdt);
rdt->flags_rdt=ACK_FLAG;

//
//while(1)
	//	if (sendto(s, (char *)packet,50, 0,(struct sockaddr *)&daddr, (socklen_t)sizeof(daddr)) < 0)
	//		perror("packet send error:");



printf("%c\n",	rdt2->flags_rdt);
	//}*/
	exit(EXIT_SUCCESS);
}

