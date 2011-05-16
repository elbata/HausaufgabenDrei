/*** IPPROTO_RAW receiver ***/
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


struct rdt_header{ 
  unsigned char        srcPort;
  unsigned char        destPort;
  unsigned char        flags_rdt;         /* Flags para control de rdt TIPo -> 0x80, SYN -> 0x01, ACK ->0x02, FIN ->0x04 */
  unsigned char        nro_SEC_rdt;       /* nro de secuencia de rdt*/
  unsigned char        nro_ACK_rdt;       /* nro de ACK de rdt*/
};

int IP2asc(u_int32_t IP,char * result){
	int aux;
	aux = sprintf(result, "%u.%u.%u.%u", (IP / 16777216),((IP / 65536) % 256),((IP / 256) % 256),(IP % 256));
	return aux;
}


int main(void)
{
	int s;
	struct sockaddr_in saddr;
	char* packet = (char*) malloc(sizeof(char)*50);
	char charIP[20];

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("error:");
		exit(EXIT_FAILURE);
	}
printf("1\n");
	//lleno el paquete de 0
	memset(packet, 0, 50);
	socklen_t *len = (socklen_t *)sizeof(saddr);
	int fromlen = sizeof(saddr);
printf("2\n");
	while(1) {
		//parametros: socket, donde guardo lo recibido, bandera, donde guardo la dir del q me mando,tamaño de la estructura anterior
		if (recvfrom(s, (char *)packet,50, 0,(struct sockaddr *)&saddr, (socklen_t *)&fromlen) < 0)
			perror("packet receive error:");
printf("1\n");
		//casteo lo recibido a un datagrama IP
		struct iphdr *ipr = (struct iphdr *)packet;
		//paso el IP del q manda a char para imprimirlo
		IP2asc(ntohl(ipr->saddr),&charIP[0]);
		fprintf(stderr,"Recepcion origen IP address: %s ", charIP);
		//idem al anterior, solo con el IP del destinatario
		IP2asc(ntohl(ipr->daddr),&charIP[0]);
		fprintf(stderr,"Destino IP address: %s con contenido: ", charIP);

		//me paro en la posicion donde empiezan los datos
		struct rdt_header *rdt = (struct rdt_header *)(packet + sizeof(struct iphdr));	
		fprintf(stderr, "srcPort %d\n", rdt->srcPort);		
		fprintf(stderr, "destPort %d\n", rdt->destPort);		
		fprintf(stderr, "nro_SEC_rdt %d\n", rdt->nro_SEC_rdt);		
		fprintf(stderr, "nro_ACK_rdt %d\n", rdt->nro_ACK_rdt);		

		int i = sizeof(struct iphdr) + sizeof(struct rdt_header);
		while (i < 50) { /* print the payload */
			fprintf(stderr, "%c", packet[i]);
			i++;
		}
		printf("\n");
	}
	exit(EXIT_SUCCESS);
}

