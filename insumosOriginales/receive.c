/*** IPPROTO_RAW receiver ***/
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int IP2asc(u_int32_t IP,char * result){
	int aux;
	aux = sprintf(result, "%u.%u.%u.%u", (IP / 16777216),((IP / 65536) % 256),((IP / 256) % 256),(IP % 256));
	return aux;
}


int main(void)
{
	int s;
	struct sockaddr_in saddr;
	char packet[50];
	char charIP[20];

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("error:");
		exit(EXIT_FAILURE);
	}

	memset(packet, 0, sizeof(packet));
	socklen_t *len = (socklen_t *)sizeof(saddr);
	int fromlen = sizeof(saddr);

	while(1) {
		if (recvfrom(s, (char *)&packet, sizeof(packet), 0,
			(struct sockaddr *)&saddr, (socklen_t *)&fromlen) < 0)
			perror("packet receive error:");

		struct iphdr *ipr = (struct iphdr *)packet;
		IP2asc(ntohl(ipr->saddr),&charIP[0]);
		fprintf(stderr,"Recepcion origen IP address: %s ", charIP);
		IP2asc(ntohl(ipr->daddr),&charIP[0]);
		fprintf(stderr,"Destino IP address: %s con contenido: ", charIP);

		int i = sizeof(struct iphdr);	/* print the payload */
		while (i < sizeof(packet)) {
			fprintf(stderr, "%c", packet[i]);
			i++;
		}
		printf("\n");
	}
	exit(EXIT_SUCCESS);
}

