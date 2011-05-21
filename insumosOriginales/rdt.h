
#ifndef RDT_H_
#define RDT_H_

#include <sys/types.h>
#include <netinet/in.h>

#define CERO		0x00 		/* 0 para conteo de TPDUs en stop and wait */
#define UNO		0x01		/* 1 para conteo de TPDUs en stop and wait */

#define DATA_FLAG	0x80		/* Mascara para deteccion de DATA */
#define SYN_FLAG	0x01		/* Mascara para deteccion de SYN */
#define ACK_FLAG	0x02		/* Mascara para deteccion de ACK */
#define FIN_FLAG	0x04		/* Mascara para deteccion de FIN */

#define MAX_IP_SIZE	256		/* Largo maximo de datagramas IP */
#define MAX_RDT_SIZE	200		/* Largo maximo de datos en carga de RDT */
#define MAX_READ_SIZE	512		/* Tamanio maximo de lectura */
#define MAX_WRITE_SIZE	512		/* Tamanio maximo de escritura */

struct rdt_header{ 
  unsigned char        srcPort;
  unsigned char        destPort;
  unsigned char        flags_rdt;         /* Flags para control de rdt TIPo -> 0x80, SYN -> 0x01, ACK ->0x02, FIN ->0x04 */
  unsigned char        nro_SEC_rdt;       /* nro de secuencia de rdt*/
  unsigned char        nro_ACK_rdt;       /* nro de ACK de rdt*/
};


int crearRDT(struct in_addr localIPaddr);
int aceptarRDT(unsigned char localRDTport);
int conectarRDT(unsigned char localRDTport,unsigned char peerRDTPport, struct in_addr peerIPaddr);
int escribirRDT(const void *buf, size_t len);
int leerRDT(void *buf, size_t len);
int cerrarRDT();


#endif /* RDT_H_ */


