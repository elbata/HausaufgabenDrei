#include "rdt.h"
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define DESCONECTADO 0
#define ESTABLECIMIENTO_ACTIVO 1
#define ESTABLECIDO_ACT 2
#define TERMINAR_ENVIAR 3
#define ESPERANDO_CLOSE 4
#define ESTABLECIMIENTO_PASIVO 5
#define SYN_RECIBIDO 6
#define ESTABLECIDO_PAS 7
#define ESPERANDO_FIN 8
#define INICIO 9

#define MAX_LARGO_BUFFER_RECEIVER 2048
#define MAX_SEQ_NUMBER 256
#define PROTOCOLO_RDT	0xFF
#define MAX_INTENTOS 10


struct buffer_struct {
    int expected_seq_number;
    char arreglo [MAX_LARGO_BUFFER_RECEIVER];
    int begin;
    int end;
    int cantLibres;
};
typedef struct buffer_struct buffer_t;

//variable que se pone en true cuando termine de mandar todo en el cliente, si soy el servidor se pone en true en el estado ESPERANDO_FIN
bool close;
int estado = INICIO;
int miSocket;

struct sockaddr_in myAddr;
buffer_t* buffer;

pthread_mutex_t semBuffer=PTHREAD_MUTEX_INITIALIZER;

//estructura donde se guarda la direccion del que me envia
struct sockaddr_in remote_addr;

int IP2asc(u_int32_t IP,char * result){
	int aux;
	aux = sprintf(result, "%u.%u.%u.%u", (IP / 16777216),((IP / 65536) % 256),((IP / 256) % 256),(IP % 256));
	return aux;
}

int enviarACK (int numeroDeSecuencia){
  //variable donde almaceno lo que voy a enviar, la seteo en 0
  //el tamaño es headerIP + headerRDT
  char datagrama_ack [sizeof(struct iphdr) + sizeof(struct rdt_header)];
  memset(datagrama_ack, 0, sizeof(struct iphdr) + sizeof(struct rdt_header));

  //posiciono los punteros dentro del  datagrama
  struct iphdr *ipHeader = (struct iphdr *)datagrama_ack;  
  struct rdt_header *rdtHeader = (struct rdt_header*) (datagrama_ack + sizeof(struct iphdr));

  //creo el datagrama a enviar				
  ipHeader->ihl = 5;
  ipHeader->version = 4;
  ipHeader->tos = 0;
  ipHeader->tot_len = htons(sizeof(struct iphdr) + sizeof(struct rdt_header));	/* tamaño total del datagrama, headerIP + datos*/
  ipHeader->frag_off = 0;		/* no fragment */
  ipHeader->ttl = 64;			/* default value */
  ipHeader->protocol = PROTOCOLO_RDT;	/* protocolo*/
  ipHeader->check = 0;			/* not needed in iphdr */
  ipHeader->saddr = myAddr.sin_addr.s_addr; /* direccion del source */
  ipHeader->daddr = remote_addr.sin_addr.s_addr; /* direccion del destination */

  //seteo el rdt_header
  rdtHeader->srcPort=myAddr.sin_port;
  rdtHeader->destPort=remote_addr.sin_port;
  rdtHeader->flags_rdt= ACK_FLAG;
  rdtHeader->nro_SEC_rdt=0;
  rdtHeader->nro_ACK_rdt=numeroDeSecuencia;

  //envio el ACK
  int envio = -1;
  while (envio < 0){
    envio = sendto(miSocket, (char *)datagrama_ack, sizeof(struct iphdr) + sizeof(struct rdt_header) , 0,(struct sockaddr *)&remote_addr, (socklen_t)sizeof(remote_addr));
  }
  fprintf(stderr,"envie ack.\n");
  return envio;
  
}

int enviarFIN (){
  //variable donde almaceno lo que voy a enviar, la seteo en 0
  //el tamaño es headerIP + headerRDT
  char datagrama_fin [sizeof(struct iphdr) + sizeof(struct rdt_header)];
  memset(datagrama_fin, 0, sizeof(struct iphdr) + sizeof(struct rdt_header));

  //posiciono los punteros dentro del  datagrama
  struct iphdr *ipHeader = (struct iphdr *)datagrama_fin;  
  struct rdt_header *rdtHeader = (struct rdt_header*) (datagrama_fin + sizeof(struct iphdr));

  //creo el datagrama a enviar				
  ipHeader->ihl = 5;
  ipHeader->version = 4;
  ipHeader->tos = 0;
  ipHeader->tot_len = htons(sizeof(struct iphdr) + sizeof(struct rdt_header));	/* tamaño total del datagrama, headerIP + datos*/
  ipHeader->frag_off = 0;		/* no fragment */
  ipHeader->ttl = 64;			/* default value */
  ipHeader->protocol = PROTOCOLO_RDT;	/* protocolo*/
  ipHeader->check = 0;			/* not needed in iphdr */
  ipHeader->saddr = myAddr.sin_addr.s_addr; /* direccion del source */
  ipHeader->daddr = remote_addr.sin_addr.s_addr; /* direccion del destination */

  //seteo el rdt_header
  rdtHeader->srcPort=myAddr.sin_port;
  rdtHeader->destPort=remote_addr.sin_port;
  rdtHeader->flags_rdt= FIN_FLAG;
  rdtHeader->nro_SEC_rdt=0;
  rdtHeader->nro_ACK_rdt=0;

  //envio el FIN
  int envio = -1;
  while (envio <= 0){
    envio = sendto(miSocket, (char *)datagrama_fin, sizeof(struct iphdr) + sizeof(struct rdt_header) , 0,(struct sockaddr *)&remote_addr, (socklen_t)sizeof(remote_addr));
  }
  fprintf(stderr,"envie fin.\n");
  return envio;
  
}

int enviarFIN_ACK(){
  //variable donde almaceno lo que voy a enviar, la seteo en 0
  //el tamaño es headerIP + headerRDT
  char datagrama_fin_ack [sizeof(struct iphdr) + sizeof(struct rdt_header)];
  memset(datagrama_fin_ack, 0, sizeof(struct iphdr) + sizeof(struct rdt_header));

  //posiciono los punteros dentro del  datagrama
  struct iphdr *ipHeader = (struct iphdr *)datagrama_fin_ack;
  struct rdt_header *rdtHeader = (struct rdt_header*) (datagrama_fin_ack + sizeof(struct iphdr));

  //creo el datagrama a enviar				
  ipHeader->ihl = 5;
  ipHeader->version = 4;
  ipHeader->tos = 0;
  ipHeader->tot_len = htons(sizeof(struct iphdr) + sizeof(struct rdt_header));	/* tamaño total del datagrama, headerIP + datos*/
  ipHeader->frag_off = 0;		/* no fragment */
  ipHeader->ttl = 64;			/* default value */
  ipHeader->protocol = PROTOCOLO_RDT;	/* protocolo*/
  ipHeader->check = 0;			/* not needed in iphdr */
  ipHeader->saddr = myAddr.sin_addr.s_addr; /* direccion del source */
  ipHeader->daddr = remote_addr.sin_addr.s_addr; /* direccion del destination */

  //seteo el rdt_header
  rdtHeader->srcPort=myAddr.sin_port;
  rdtHeader->destPort=remote_addr.sin_port;
  rdtHeader->flags_rdt= FIN_FLAG + ACK_FLAG;
  rdtHeader->nro_SEC_rdt=0;
  rdtHeader->nro_ACK_rdt=0;

  //envio el FIN_ACK
  int envio = -1;
  while (envio < 0){
    envio = sendto(miSocket, (char *)datagrama_fin_ack, sizeof(struct iphdr) + sizeof(struct rdt_header) , 0,(struct sockaddr *)&remote_addr, (socklen_t)sizeof(remote_addr));
  }
  fprintf(stderr,"envie fin_ack.\n");
  return envio;
  
}


void* threadAttention(void* param){
	while (close == false){
	  //  fprintf(stderr,"estado: %d.\n",estado);
		switch (estado){
			case DESCONECTADO:
					break;
			case ESTABLECIMIENTO_ACTIVO:
					break;
			case ESTABLECIDO_ACT:{
					char datagram[MAX_IP_SIZE];
					memset(datagram, 0, MAX_IP_SIZE);
					//posiciono los punteros dentro del  datagrama
					struct iphdr *ipHeader = (struct iphdr *)datagram;
					struct rdt_header *rdtHeader = (struct rdt_header*) (datagram + sizeof(struct iphdr));
					
					//cantidad de bytes a enviar
					int cantToSend;
					//numero de secuencia a enviar y esperado por ack
					int seq_number;
					int seq_numerRecibido;
					
					//pido el semaforo
					pthread_mutex_lock(&semBuffer);
					cantToSend= MAX_LARGO_BUFFER_RECEIVER - buffer->cantLibres;
					//fprintf(stderr,"\ncantToSend = %d\n",cantToSend);
					if (cantToSend>0){
					  if (cantToSend > MAX_RDT_SIZE){
					    cantToSend=MAX_RDT_SIZE;					    
					  }
					  //copio los bytes a enviar
					  for(int i=0;i<cantToSend;i++){
					    datagram[sizeof(struct iphdr) + sizeof(struct rdt_header) + i]= buffer->arreglo[buffer->begin];
					    buffer->begin=(buffer->begin + 1) % MAX_LARGO_BUFFER_RECEIVER ;
					    buffer->cantLibres++;
					  }
					}
					//libero el semaforo
					seq_number=buffer->expected_seq_number;
					pthread_mutex_unlock(&semBuffer);
					if(cantToSend>0){
					  //creo el datagrama a enviar				
					  ipHeader->ihl = 5;
					  ipHeader->version = 4;
					  ipHeader->tos = 0;
					  ipHeader->tot_len = htons(sizeof(struct iphdr) + sizeof(struct rdt_header) + cantToSend);	/* tamaño total del datagrama, headerIP + datos*/
					  ipHeader->frag_off = 0;		/* no fragment */
					  ipHeader->ttl = 64;			/* default value */
					  ipHeader->protocol = PROTOCOLO_RDT;	/* protocolo*/
					  ipHeader->check = 0;			/* not needed in iphdr */
					  ipHeader->saddr = myAddr.sin_addr.s_addr; /* direccion del source */
					  ipHeader->daddr = remote_addr.sin_addr.s_addr; /* direccion del destination */

					  //seteo el rdt_header
					  rdtHeader->srcPort=myAddr.sin_port;
					  rdtHeader->destPort=remote_addr.sin_port;
					  rdtHeader->flags_rdt= DATA_FLAG;
					  rdtHeader->nro_SEC_rdt=seq_number;
					  rdtHeader->nro_ACK_rdt=0;
					  
					  int envioS = -1;
					  while(envioS<0){
					    envioS = sendto(miSocket, (char *)datagram, sizeof(struct iphdr) + sizeof(struct rdt_header) + cantToSend , 0,(struct sockaddr *)&remote_addr, (socklen_t)sizeof(remote_addr));
					  }
					  fprintf(stderr,"envie el paquetecon seq= %d\n",seq_number);
					  bool receiveACK=false;
					  int cantidadIntentosACK = 0;
					  //set de filedescriptors
					  fd_set fds;
					  int n;
					  //estructura para el timeout
					  struct timeval tv;

					  //setea el file descriptor del que voy a leer
					  FD_ZERO(&fds);
					  FD_SET(miSocket, &fds);

					  //setea el struct timeval para el timeout, habiamos quedado que en 1 segundo estaba bien
					  tv.tv_sec = 1;
					  tv.tv_usec = 0;


					  while (!receiveACK && (cantidadIntentosACK < MAX_INTENTOS)){
					  //while (!receiveACK){
					    //la magia del select	
					    					  FD_ZERO(&fds);
					  FD_SET(miSocket, &fds);
					    n = select(miSocket + 1, &fds, NULL, NULL, &tv);
					    //salgo por timeout
					    if (n == 0){
					      fprintf(stderr,"no llego el ACK,sali por timeout\n");
					      envioS = -1;
					      //vuelvo a enviar el paquete
					      while(envioS<0){
						envioS = sendto(miSocket, (char *)datagram, sizeof(struct iphdr) + sizeof(struct rdt_header) + cantToSend , 0,(struct sockaddr *)&remote_addr, (socklen_t)sizeof(remote_addr));
					      }
					      fprintf(stderr,"volvi a enviar el paquete\n");
					      cantidadIntentosACK++;
					      tv.tv_sec = 1;
					      tv.tv_usec = 0;
					    }
					    else{
					      if (n < 0){
						fprintf(stderr,"error al recibir ACK\n");
					      }
					      else{		  		  		  		  	  
						//datagrama ACK a recibir
						char datagrama_ack [sizeof(struct iphdr) + sizeof(struct rdt_header)];
						memset(datagrama_ack, 0, sizeof(struct iphdr) + sizeof(struct rdt_header));
						struct sockaddr_in remote_addr2;
						socklen_t *remote_addr_size2 = (socklen_t *) sizeof (remote_addr2);
						int fromlen2 = sizeof(remote_addr2);

						//en teoria esto tendria que funcionar, puedo leer sin problemas porque el select me devuelve en que socket tengo actividad y hay uno solo asi que no habria problemas
						int tamRecibidoAck = recvfrom(miSocket, (char*) &datagrama_ack, sizeof(struct iphdr) + sizeof(struct rdt_header), 0, (struct sockaddr*) &remote_addr2, (socklen_t *)&fromlen2);
						
						
						if (tamRecibidoAck < 0){
						    fprintf(stderr,"error al recibir ACK\n");

						}
						else{				

						    char charIP[20];
						    //extraigo el headerIP
						    struct iphdr *ipr = (struct iphdr *)datagrama_ack;

						    //paso el IP del q manda a char para imprimirlo
						    IP2asc(ntohl(ipr->saddr),&charIP[0]);
						    printf("Recepcion origen IP address: %s \n", charIP);

						    //idem al anterior, solo con el IP del destinatario
						    IP2asc(ntohl(ipr->daddr),&charIP[0]);
						    printf("Destino IP address: %s: \n", charIP);

						    //imprimo el protocolo 
						    printf("protocolo: %d: ", ipr->protocol);

						    //extraigo el headerRDT
						    struct rdt_header *rdt = (struct rdt_header *)(datagrama_ack + sizeof(struct iphdr));
						    printf("srcPort %d\n", rdt->srcPort);		
						    printf("destPort %d\n", rdt->destPort);
						    printf("flags %d\n", rdt->flags_rdt);				
						    printf("nro_SEC_rdt %d\n", rdt->nro_SEC_rdt);
						    printf("espero nro_SEC_ %d\n", seq_number);		
						    printf("nro_ACK_rdt %d\n", rdt->nro_ACK_rdt);		
						    seq_numerRecibido =rdt->nro_ACK_rdt;
						    //verifico el protocolo
						    bool protocoloCorrecto;
						    if (ipr->protocol == PROTOCOLO_RDT)
							    protocoloCorrecto = true;
						    else
							    protocoloCorrecto = false;

						    //verifico que es ACK
						    bool esACK;
						    //verifico que sea solo ack
						    unsigned char banderas = rdt->flags_rdt;			
						    if (banderas == ACK_FLAG)
							    esACK = true;
						    else
							    esACK = false;
						    
						    //es mi IP
						    bool miIP=false;
						    if (myAddr.sin_addr.s_addr == ipr->daddr)
							miIP = true;
						    else
							miIP = false;

						    //es la IP del otro
						    bool suIP=false;
						    if (remote_addr.sin_addr.s_addr == ipr->saddr)
							suIP = true;
						    else
							suIP = false;

						    //es mi puerto
						    bool miPuerto=false;
						    if (myAddr.sin_port == rdt->destPort)
							miPuerto= true;
						    else
							miPuerto = false;

						    //es su puerto
						    bool suPuerto=false;
						    if (remote_addr.sin_port == rdt->srcPort)
							suPuerto= true;
						    else
							suPuerto = false;
						    
						    //faltaria controlar el numero de secuencia		  
						    if (protocoloCorrecto && esACK && miIP && suIP && miPuerto && suPuerto){
						      if (seq_numerRecibido == seq_number){
							fprintf(stderr,"recibi el ACK con exito \n");
							receiveACK = true;
							cantidadIntentosACK = 0;
						      }
						      else{
							fprintf(stderr,"recibi el ACK con numero de secuencia distinto %d\n",seq_numerRecibido);
						      }
						    }
						    else{//el paquete no es para mi
							    fprintf(stderr,"el paquete no es para mi \n");
							    //tv.tv_sec = 3;
							    //tv.tv_usec = 0;
						    }

						}//fin else hubo error en el receive
					      }//fin else n<0
					    }//fin else select == 0					   
					  }//fin while !receiveACK
					  
					  //si lo envie 10 veces y nunca me llego el ack, puedo suponer qu no estoy conectado...
					  if (!receiveACK){
					    fprintf(stderr,"espero un ack que no llego en 10 intentos, posible desconexion \n");
					    estado = DESCONECTADO;
					  }
					  else{
					   
					    //incremento el numero de secuencia
						pthread_mutex_lock(&semBuffer);
						buffer->expected_seq_number = (buffer->expected_seq_number + 1) % MAX_SEQ_NUMBER;
						pthread_mutex_unlock(&semBuffer);
					    
					  }
					}//fin if cantToSend > 0
			}
					break;
			case TERMINAR_ENVIAR:{
					char datagram[MAX_IP_SIZE];
					memset(datagram, 0, MAX_IP_SIZE);
					//posiciono los punteros dentro del  datagrama
					struct iphdr *ipHeader = (struct iphdr *)datagram;
					struct rdt_header *rdtHeader = (struct rdt_header*) (datagram + sizeof(struct iphdr));
					
					//cantidad de bytes a enviar
					int cantToSend;
					//numero de secuencia a enviar y esperado por ack
					int seq_number;
					
					//pido el semaforo
					pthread_mutex_lock(&semBuffer);
					cantToSend= MAX_LARGO_BUFFER_RECEIVER - buffer->cantLibres;
					
					if (cantToSend>0){
					  if (cantToSend > MAX_RDT_SIZE){
					    cantToSend=MAX_RDT_SIZE;					    
					  }
					  //copio los bytes a enviar
					  for(int i=0;i<cantToSend;i++){
					    datagram[sizeof(struct iphdr) + sizeof(struct rdt_header) + i]= buffer->arreglo[buffer->begin];
					    buffer->begin=(buffer->begin + 1) % MAX_LARGO_BUFFER_RECEIVER ;
					    buffer->cantLibres++;
					  }
					}
					//libero el semaforo
					seq_number=buffer->expected_seq_number;
					pthread_mutex_unlock(&semBuffer);
					if(cantToSend>0){
					  //creo el datagrama a enviar				
					  ipHeader->ihl = 5;
					  ipHeader->version = 4;
					  ipHeader->tos = 0;
					  ipHeader->tot_len = htons(sizeof(struct iphdr) + sizeof(struct rdt_header) + cantToSend);	/* tamaño total del datagrama, headerIP + datos*/
					  ipHeader->frag_off = 0;		/* no fragment */
					  ipHeader->ttl = 64;			/* default value */
					  ipHeader->protocol = PROTOCOLO_RDT;	/* protocolo*/
					  ipHeader->check = 0;			/* not needed in iphdr */
					  ipHeader->saddr = myAddr.sin_addr.s_addr; /* direccion del source */
					  ipHeader->daddr = remote_addr.sin_addr.s_addr; /* direccion del destination */

					  //seteo el rdt_header
					  rdtHeader->srcPort=myAddr.sin_port;
					  rdtHeader->destPort=remote_addr.sin_port;
					  rdtHeader->flags_rdt= DATA_FLAG;
					  rdtHeader->nro_SEC_rdt=seq_number;
					  rdtHeader->nro_ACK_rdt=0;
					  
					  int envioS = -1;
					  while(envioS<=0){
					    envioS = sendto(miSocket, (char *)datagram, sizeof(struct iphdr) + sizeof(struct rdt_header) + cantToSend , 0,(struct sockaddr *)&remote_addr, (socklen_t)sizeof(remote_addr));
					  }
					  bool receiveACK=false;
					  int cantidadIntentos = 0;
					  //set de filedescriptors
					  fd_set fds;
					  int n;
					  //estructura para el timeout
					  struct timeval tv;

					  //setea el file descriptor del que voy a leer
					  FD_ZERO(&fds);
					  FD_SET(miSocket, &fds);

					  //setea el struct timeval para el timeout, habiamos quedado que en 1 segundo estaba bien
					  tv.tv_sec = 1;
					  tv.tv_usec = 0;
					  while (!receiveACK && (cantidadIntentos < MAX_INTENTOS)){
					    //la magia del select			
					    n = select(miSocket + 1, &fds, NULL, NULL, &tv);
					    //salgo por timeout
					    if (n == 0){
					      fprintf(stderr,"no llego el ACK,sali por timeout\n");
					      envioS = -1;
					      //vuelvo a enviar el paquete
					      while(envioS<=0){
						envioS = sendto(miSocket, (char *)datagram, sizeof(struct iphdr) + sizeof(struct rdt_header) + cantToSend , 0,(struct sockaddr *)&remote_addr, (socklen_t)sizeof(remote_addr));
					      }
					      tv.tv_sec = 1;
					      tv.tv_usec = 0;
					      cantidadIntentos++;
					    }
					    else{
					      if (n < 0){
						fprintf(stderr,"error al recibir para ACK\n");
					      }
					      else{		  		  		  		  	  
						//datagrama ACK a recibir
						char datagrama_ack [sizeof(struct iphdr) + sizeof(struct rdt_header)];
						memset(datagrama_ack, 0, sizeof(struct iphdr) + sizeof(struct rdt_header));
						struct sockaddr_in remote_addr2;
						socklen_t *remote_addr_size2 = (socklen_t *) sizeof (remote_addr2);
						int fromlen2 = sizeof(remote_addr2);

						//en teoria esto tendria que funcionar, puedo leer sin problemas porque el select me devuelve en que socket tengo actividad y hay uno solo asi que no habria problemas
						int tamRecibidoAck = recvfrom(miSocket, (char*) &datagrama_ack, sizeof(struct iphdr) + sizeof(struct rdt_header), 0, (struct sockaddr*) &remote_addr2, (socklen_t *)&fromlen2);
						
						
						if (tamRecibidoAck < 0){
						    fprintf(stderr,"error al recibir ACK\n");

						}
						else{				

						    char charIP[20];
						    //extraigo el headerIP
						    struct iphdr *ipr = (struct iphdr *)datagrama_ack;

						    //paso el IP del q manda a char para imprimirlo
						    IP2asc(ntohl(ipr->saddr),&charIP[0]);
						    printf("Recepcion origen IP address: %s \n", charIP);

						    //idem al anterior, solo con el IP del destinatario
						    IP2asc(ntohl(ipr->daddr),&charIP[0]);
						    printf("Destino IP address: %s: \n", charIP);

						    //imprimo el protocolo 
						    printf("protocolo: %d: ", ipr->protocol);

						    //extraigo el headerRDT
						    struct rdt_header *rdt = (struct rdt_header *)(datagrama_ack + sizeof(struct iphdr));
						    printf("srcPort %d\n", rdt->srcPort);		
						    printf("destPort %d\n", rdt->destPort);
						    printf("flags %d\n", rdt->flags_rdt);				
						    printf("nro_SEC_rdt %d\n", rdt->nro_SEC_rdt);		
						    printf("nro_ACK_rdt %d\n", rdt->nro_ACK_rdt);		

						    //verifico el protocolo
						    bool protocoloCorrecto;
						    if (ipr->protocol == PROTOCOLO_RDT)
							    protocoloCorrecto = true;
						    else
							    protocoloCorrecto = false;

						    //verifico que es ACK
						    bool esACK;
						    //verifico que sea solo ack
						    unsigned char banderas = rdt->flags_rdt;			
						    if (banderas == ACK_FLAG)
							    esACK = true;
						    else
							    esACK = false;
						    
						    //es mi IP
						    bool miIP=false;
						    if (myAddr.sin_addr.s_addr == ipr->daddr)
							miIP = true;
						    else
							miIP = false;

						    //es la IP del otro
						    bool suIP=false;
						    if (remote_addr.sin_addr.s_addr == ipr->saddr)
							suIP = true;
						    else
							suIP = false;

						    //es mi puerto
						    bool miPuerto=false;
						    if (myAddr.sin_port == rdt->destPort)
							miPuerto= true;
						    else
							miPuerto = false;

						    //es su puerto
						    bool suPuerto=false;
						    if (remote_addr.sin_port == rdt->srcPort)
							suPuerto= true;
						    else
							suPuerto = false;
						    
						    //faltaria controlar el numero de secuencia		  
						    if (protocoloCorrecto && esACK && miIP && suIP && miPuerto && suPuerto){
						      if (rdt->nro_ACK_rdt == seq_number){
							fprintf(stderr,"recibi el ACK con exito \n");
							receiveACK = true;
							cantidadIntentos = 0;
						      }
						      else{
							fprintf(stderr,"recibi el ACK con numero de secuencia distinto\n");
						      }
						    }
						    else{//el paquete no es para mi
							    fprintf(stderr,"no es el paquete que espero,espero ack \n");
							    //tv.tv_sec = 3;
							    //tv.tv_usec = 0;							    
						    }

						}//fin else hubo error en el receive
					      }//fin else n<0
					    }//fin else select == 0
					    
					  }//fin while !receiveACK
					  
					  //si no recibi el ACK en la cantidad de intentos necesaria
					  if (!receiveACK){
					    int aux = enviarFIN();
					    estado = ESPERANDO_CLOSE;
					    fprintf(stderr,"espero un ack que no llego en 10 intentos, posible desconexion \n");
					  }
					  else{
					  //incremento el numero de secuencia
					  pthread_mutex_lock(&semBuffer);
					  buffer->expected_seq_number = (buffer->expected_seq_number + 1) % MAX_SEQ_NUMBER;
					  pthread_mutex_unlock(&semBuffer);
					  }
					}//fin if cantToSend > 0
					else{
					  //terrmine de mandar todo, envio fin y cambio de estado...
					  int aux = enviarFIN();
					  estado = ESPERANDO_CLOSE;
					}
			}
					break;
			case ESPERANDO_CLOSE:{
			  
					char datagrama_fin_ack [sizeof(struct iphdr) + sizeof(struct rdt_header)];
					memset(datagrama_fin_ack, 0, sizeof(struct iphdr) + sizeof(struct rdt_header));

					struct sockaddr_in remote_addr2;
					socklen_t *remote_addr_size2 = (socklen_t *) sizeof (remote_addr2);
					int fromlen2 = sizeof(remote_addr2);
					
					//booleano para saber si recibi el fin,ack
					bool recibi_fin_ack = false;
					int cantidadIntentosFIN_ACK = 0;
					
					//bool para salir por timeout,casi siempre voy a leer algo, asi q en caso de que no llegue nada salgo por timeout
					bool timeout = false;
					//set de filedescriptors
					fd_set fds;
					int n;
					//estructura para el timeout
					struct timeval tv;

					//setea el file descriptor del que voy a leer
					FD_ZERO(&fds);
					FD_SET(miSocket, &fds);

					//setea el struct timeval para el timeout, habiamos quedado que en 1 segundo estaba bien
					tv.tv_sec = 3;
					tv.tv_usec = 0;
					
					while (!recibi_fin_ack && !timeout){


					  //la magia del select			
					  n = select(miSocket + 1, &fds, NULL, NULL, &tv);
					  //salgo por timeout	      
					  if (n == 0){
					    fprintf(stderr,"no llego el fin,ack para terminar la conexion,sali por timeout\n");
					    timeout=true;
					  }
					  else{
					    if (n < 0){
					      fprintf(stderr,"error al recibir para fin,ack para terminar la conexion\n");
					    }
					    else{
						int tamRecibidoFinAck = recvfrom(miSocket, (char*) &datagrama_fin_ack, sizeof(struct iphdr) + sizeof(struct rdt_header), 0, (struct sockaddr*) &remote_addr2, (socklen_t *)&fromlen2);
						if (tamRecibidoFinAck < 0){
						  fprintf(stderr,"error al recibir el fin,ack\n");
						}
						else{
						    char charIP[20];
						    //extraigo el headerIP
						    struct iphdr *ipr = (struct iphdr *)datagrama_fin_ack;

						    //paso el IP del q manda a char para imprimirlo
						    IP2asc(ntohl(ipr->saddr),&charIP[0]);
						    printf("Recepcion origen IP address: %s \n", charIP);

						    //idem al anterior, solo con el IP del destinatario
						    IP2asc(ntohl(ipr->daddr),&charIP[0]);
						    printf("Destino IP address: %s: \n", charIP);

						    //imprimo el protocolo 
						    printf("protocolo: %d: ", ipr->protocol);

						    //extraigo el headerRDT
						    struct rdt_header *rdt = (struct rdt_header *)(datagrama_fin_ack + sizeof(struct iphdr));
						    printf("srcPort %d\n", rdt->srcPort);		
						    printf("destPort %d\n", rdt->destPort);
						    printf("flags %d\n", rdt->flags_rdt);				
						    printf("nro_SEC_rdt %d\n", rdt->nro_SEC_rdt);		
						    printf("nro_ACK_rdt %d\n", rdt->nro_ACK_rdt);		

						    //verifico el protocolo
						    bool protocoloCorrecto;
						    if (ipr->protocol == PROTOCOLO_RDT)
							    protocoloCorrecto = true;
						    else
							    protocoloCorrecto = false;

						    //verifico que es FIN_ACK
						    bool es_FIN_ACK;
						    
						    unsigned char banderas = rdt->flags_rdt;			
						    if (banderas == (ACK_FLAG + FIN_FLAG))
							    es_FIN_ACK = true;
						    else
							    es_FIN_ACK = false;
						    
						    //es mi IP
						    bool miIP=false;
						    if (myAddr.sin_addr.s_addr == ipr->daddr)
							miIP = true;
						    else
							miIP = false;

						    //es la IP del otro
						    bool suIP=false;
						    if (remote_addr.sin_addr.s_addr == ipr->saddr)
							suIP = true;
						    else
							suIP = false;

						    //es mi puerto
						    bool miPuerto=false;
						    if (myAddr.sin_port == rdt->destPort)
							miPuerto= true;
						    else
							miPuerto = false;

						    //es su puerto
						    bool suPuerto=false;
						    if (remote_addr.sin_port == rdt->srcPort)
							suPuerto= true;
						    else
							suPuerto = false;
						    
						    //faltaria controlar el numero de secuencia
						    
						    if (protocoloCorrecto && es_FIN_ACK && miIP && suIP && miPuerto && suPuerto){	
							    fprintf(stderr,"recibi el fin,ack con exito \n");
							    recibi_fin_ack = true;
						    }
						    else{//el paquete no es para mi
							    fprintf(stderr,"no es el paquete que espero \n");
						    }//fin else comparar si es el mensaje que quiero
						}//fin else tamRecibido <0
					    }//fin else n<0
					  }//fin else n ==0

					}//fin while recibi_fin_ack
					if (recibi_fin_ack){
					  int aux = enviarACK(0);
					}
					close = true;
			}
					break;
			case ESTABLECIMIENTO_PASIVO:
					break;
			case SYN_RECIBIDO:
					break;			
			case ESTABLECIDO_PAS:{
			  
					bool esFIN =false;
					bool timeout = false;
					//set de filedescriptors
					fd_set fds;
					int n;
					//estructura para el timeout
					struct timeval tv;

					//setea el file descriptor del que voy a leer
					FD_ZERO(&fds);
					FD_SET(miSocket, &fds);

					//setea el struct timeval para el timeout, habiamos quedado que en 1 segundo estaba bien
					tv.tv_sec = 5;
					tv.tv_usec = 0;
					while(!timeout && !esFIN){
			  
					  //la magia del select			
					  n = select(miSocket + 1, &fds, NULL, NULL, &tv);
					  if (n == 0){
					    fprintf(stderr,"sali por timeout en establecido_pas\n");
					    timeout = true;
					  }else{
					    
					      char datagram[MAX_IP_SIZE];
					      memset(datagram, 0, MAX_IP_SIZE);
					      //posiciono los punteros dentro del  datagrama
					      struct iphdr *ipHeader = (struct iphdr *)datagram;
					      struct rdt_header *rdtHeader = (struct rdt_header*) (datagram + sizeof(struct iphdr));

					      struct sockaddr_in remote_addr2;
					      socklen_t *remote_addr_size2 = (socklen_t *) sizeof (remote_addr2);
					      int fromlen2 = sizeof(remote_addr2);

					      pthread_mutex_lock(&semBuffer);
					      int seq_number = buffer->expected_seq_number;
					      pthread_mutex_unlock(&semBuffer);
					      
					      //quedo trancado hasta que recibo algo de la red...
					      int tamRecibido = recvfrom(miSocket, (char*) &datagram, MAX_IP_SIZE, 0, (struct sockaddr*) &remote_addr2, (socklen_t *)&fromlen2);
					      
					      if (tamRecibido < 0){
						fprintf(stderr,"error al recibir un datagrama del cliente\n");
					      }
					      else{
						char charIP[20];
						  //extraigo el headerIP
						  struct iphdr *ipr = (struct iphdr *)datagram;

						  //paso el IP del q manda a char para imprimirlo
						  IP2asc(ntohl(ipr->saddr),&charIP[0]);
					         printf("Recepcion origen IP address: %s \n", charIP);

						  //idem al anterior, solo con el IP del destinatario
						  IP2asc(ntohl(ipr->daddr),&charIP[0]);
						 printf("Destino IP address: %s: \n", charIP);

						  //imprimo el protocolo 
						  printf("protocolo: %d: ", ipr->protocol);

						  //extraigo el headerRDT
						  struct rdt_header *rdt = (struct rdt_header *)(datagram + sizeof(struct iphdr));
						  printf("srcPort %d\n", rdt->srcPort);		
						  printf("destPort %d\n", rdt->destPort);
						  printf("flags %d\n", rdt->flags_rdt);				
						  printf("nro_SEC_rdt %d\n", rdt->nro_SEC_rdt);
						  printf("espero nro_SEC_ %d\n", seq_number);		
						  printf("nro_ACK_rdt %d\n", rdt->nro_ACK_rdt);		

						  //verifico el protocolo
						  bool protocoloCorrecto;
						  if (ipr->protocol == PROTOCOLO_RDT)
							  protocoloCorrecto = true;
						  else
							  protocoloCorrecto = false;

						  //verifico que es DATA
						  bool esDATA;
						  //verifico que sea solo ack
						  unsigned char banderas = rdt->flags_rdt;			
						  if (banderas == DATA_FLAG)
							  esDATA = true;
						  else
							  esDATA = false;
						  
						  if (banderas == FIN_FLAG)
							  esFIN = true;
						  else
							  esFIN = false;
						  
						  //es mi IP
						  bool miIP=false;
						  if (myAddr.sin_addr.s_addr == ipr->daddr)
						      miIP = true;
						  else
						      miIP = false;

						  //es la IP del otro
						  bool suIP=false;
						  if (remote_addr.sin_addr.s_addr == ipr->saddr)
						      suIP = true;
						  else
						      suIP = false;

						  //es mi puerto
						  bool miPuerto=false;
						  if (myAddr.sin_port == rdt->destPort)
						      miPuerto= true;
						  else
						      miPuerto = false;

						  //es su puerto
						  bool suPuerto=false;
						  if (remote_addr.sin_port == rdt->srcPort)
						      suPuerto= true;
						  else
						      suPuerto = false;
						  
						  //faltaria controlar el numero de secuencia		  
						  if (protocoloCorrecto && esDATA && miIP && suIP && miPuerto && suPuerto){
						    if (rdt->nro_SEC_rdt == seq_number){
						      fprintf(stderr,"recibi el datagrama con datos, con numero de secuencia esperado \n");
						      
						      //pido el semaforo
						      pthread_mutex_lock(&semBuffer);
						      
						      //controlo que tenga espacio en el buffer para los datos...
						      int sizeDatos = tamRecibido - (sizeof(struct rdt_header) + sizeof(struct iphdr));
						      if (buffer->cantLibres >= sizeDatos){
							
							int pos = sizeof(struct rdt_header) + sizeof(struct iphdr);
							//copio los datos al buffer...
							for (int i=0; i< sizeDatos; i++){
							  buffer->arreglo[buffer->end] = datagram[pos + i];
							  buffer->end = (buffer->end + 1) % MAX_LARGO_BUFFER_RECEIVER;
							  buffer->cantLibres--;
							  
							}
							
							int aux = enviarACK (seq_number);
							
							//incremento el numero de secuencia...						  
							buffer->expected_seq_number = (buffer->expected_seq_number + 1) % MAX_SEQ_NUMBER;
							pthread_mutex_unlock(&semBuffer);
							//reseteo
							tv.tv_sec = 5;
							tv.tv_usec = 0;
						      }
						      else{
							fprintf(stderr,"no tengo espacio en el buffer para almacenar los datos recibidos\n");
							pthread_mutex_unlock(&semBuffer);
							//reseteo
							tv.tv_sec = 5;
							tv.tv_usec = 0;
						      }
						    }
						    else{
						      fprintf(stderr,"recibi el datagrama con datos,con numero de secuencia distinto al esperado\n");
						      //envio con ack anterior al actual
						      int envio = enviarACK((seq_number-1)%MAX_SEQ_NUMBER);
						      //reseteo
						      tv.tv_sec = 5;
						      tv.tv_usec = 0;
						      
						    }
						  }
						  else{
							  if (protocoloCorrecto && esFIN && miIP && suIP && miPuerto && suPuerto){
							    //envio FIN,ACK y cambio de estado...
							    fprintf(stderr,"recibi fin\n");
							    esFIN = true;
							    //int aux = enviarFIN_ACK();
							    //estado = ESPERANDO_FIN;
							  }
							  else{
							    //el paquete no es para mi
							    fprintf(stderr,"el paquete no es para mi,espero data \n");
							    tv.tv_sec = 5;
							    tv.tv_usec = 0;
							  }
						  }
					    }//fin else tamRecibido < 0
					  }//fin else n==0
					}//fin while
					if (esFIN){
					  int aux = enviarFIN_ACK();
					  
					}
					fprintf(stderr,"cambieeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee \n");
					estado = ESPERANDO_FIN;
					  					
			}
					break;
			case ESPERANDO_FIN:{
				        char datagrama_ack [sizeof(struct iphdr) + sizeof(struct rdt_header)];
					memset(datagrama_ack, 0, sizeof(struct iphdr) + sizeof(struct rdt_header));

					struct sockaddr_in remote_addr2;
					socklen_t *remote_addr_size2 = (socklen_t *) sizeof (remote_addr2);
					int fromlen2 = sizeof(remote_addr2);
					
					//booleano para saber si recibi el fin,ack
					bool recibi_ack = false;
					
					//bool para salir por timeout,casi siempre voy a leer algo, asi q en caso de que no llegue nada salgo por timeout
					bool timeout = false;
					//set de filedescriptors
					fd_set fds;
					int n;
					//estructura para el timeout
					struct timeval tv;

					//setea el file descriptor del que voy a leer
					FD_ZERO(&fds);
					FD_SET(miSocket, &fds);

					//setea el struct timeval para el timeout, habiamos quedado que en 1 segundo estaba bien
					tv.tv_sec = 3;
					tv.tv_usec = 0;
					while (!recibi_ack && !timeout){
					  //la magia del select			
					  n = select(miSocket + 1, &fds, NULL, NULL, &tv);
					  //salgo por timeout	      
					  if (n == 0){
					    fprintf(stderr,"no llego el ack para terminar la conexion,sali por timeout\n");
					    timeout=true;
					  }
					  else{
					    if (n < 0){
					      fprintf(stderr,"error al recibir ack para terminar la conexion\n");
					    }
					    else{
						int tamRecibidoAck = recvfrom(miSocket, (char*) &datagrama_ack, sizeof(struct iphdr) + sizeof(struct rdt_header), 0, (struct sockaddr*) &remote_addr2, (socklen_t *)&fromlen2);
						if (tamRecibidoAck < 0){
						  fprintf(stderr,"error al recibir el fin,ack\n");
						}
						else{
						    char charIP[20];
						    //extraigo el headerIP
						    struct iphdr *ipr = (struct iphdr *)datagrama_ack;

						    //paso el IP del q manda a char para imprimirlo
						    IP2asc(ntohl(ipr->saddr),&charIP[0]);
						    printf("Recepcion origen IP address: %s \n", charIP);

						    //idem al anterior, solo con el IP del destinatario
						    IP2asc(ntohl(ipr->daddr),&charIP[0]);
						    printf("Destino IP address: %s: \n", charIP);

						    //imprimo el protocolo 
						    printf("protocolo: %d: ", ipr->protocol);

						    //extraigo el headerRDT
						    struct rdt_header *rdt = (struct rdt_header *)(datagrama_ack + sizeof(struct iphdr));
						    printf("srcPort %d\n", rdt->srcPort);		
						    printf("destPort %d\n", rdt->destPort);
						    printf("flags %d\n", rdt->flags_rdt);				
						    printf("nro_SEC_rdt %d\n", rdt->nro_SEC_rdt);		
						    printf("nro_ACK_rdt %d\n", rdt->nro_ACK_rdt);		

						    //verifico el protocolo
						    bool protocoloCorrecto;
						    if (ipr->protocol == PROTOCOLO_RDT)
							    protocoloCorrecto = true;
						    else
							    protocoloCorrecto = false;

						    //verifico que es ACK
						    bool es_ACK;
						    
						    unsigned char banderas = rdt->flags_rdt;			
						    if (banderas == (ACK_FLAG))
							    es_ACK = true;
						    else
							    es_ACK = false;
						    
						    //es mi IP
						    bool miIP=false;
						    if (myAddr.sin_addr.s_addr == ipr->daddr)
							miIP = true;
						    else
							miIP = false;

						    //es la IP del otro
						    bool suIP=false;
						    if (remote_addr.sin_addr.s_addr == ipr->saddr)
							suIP = true;
						    else
							suIP = false;

						    //es mi puerto
						    bool miPuerto=false;
						    if (myAddr.sin_port == rdt->destPort)
							miPuerto= true;
						    else
							miPuerto = false;

						    //es su puerto
						    bool suPuerto=false;
						    if (remote_addr.sin_port == rdt->srcPort)
							suPuerto= true;
						    else
							suPuerto = false;
						    
						    //faltaria controlar el numero de secuencia
						    
						    if (protocoloCorrecto && es_ACK && miIP && suIP && miPuerto && suPuerto){	
							    fprintf(stderr,"recibi el ack con exito, cierro la conexion\n");
							    recibi_ack = true;
						    }
						    else{//el paquete no es para mi
							    fprintf(stderr,"no es el paquete que espero \n");
						    }//fin else comparar si es el mensaje que quiero
						}//fin else tamRecibido <0
					    }//fin else n<0
					  }//fin else n ==0

					}//fin while recibi_ack
					
					//tengo que esperar que terminen de leer para finalizar...
					bool termine = false;
					int faltan;
					while (!termine){					  
					  pthread_mutex_lock(&semBuffer);	
					  faltan = buffer->cantLibres;					
					  pthread_mutex_unlock(&semBuffer);						  
					  if (faltan == MAX_LARGO_BUFFER_RECEIVER)
					    termine = true;
					  else
					    termine = false;
					}
					close = true;
			}
					break;
			case INICIO:
					break;
		}
	}

	estado = INICIO;
	//este pedido de semaforo no tendria que ser necesario...
	pthread_mutex_lock(&semBuffer);	
	delete buffer;					
	pthread_mutex_unlock(&semBuffer);	  
	pthread_exit(NULL);
}


int enviarSYN_ACK (int numeroDeSecuencia){
  //variable donde almaceno lo que voy a enviar, la seteo en 0
  //el tamaño es headerIP + headerRDT
  char datagrama_syn_ack [sizeof(struct iphdr) + sizeof(struct rdt_header)];
  memset(datagrama_syn_ack, 0, sizeof(struct iphdr) + sizeof(struct rdt_header));

  //posiciono los punteros dentro del  datagrama
  struct iphdr *ipHeader = (struct iphdr *)datagrama_syn_ack;  
  struct rdt_header *rdtHeader = (struct rdt_header*) (datagrama_syn_ack + sizeof(struct iphdr));

  //creo el datagrama a enviar				
  ipHeader->ihl = 5;
  ipHeader->version = 4;
  ipHeader->tos = 0;
  ipHeader->tot_len = htons(sizeof(struct iphdr) + sizeof(struct rdt_header));	/* tamaño total del datagrama, headerIP + datos*/
  ipHeader->frag_off = 0;		/* no fragment */
  ipHeader->ttl = 64;			/* default value */
  ipHeader->protocol = PROTOCOLO_RDT;	/* protocolo*/
  ipHeader->check = 0;			/* not needed in iphdr */
  ipHeader->saddr = myAddr.sin_addr.s_addr; /* direccion del source */
  ipHeader->daddr = remote_addr.sin_addr.s_addr; /* direccion del destination */

  //seteo el rdt_header
  rdtHeader->srcPort=myAddr.sin_port;
  rdtHeader->destPort=remote_addr.sin_port;
  rdtHeader->flags_rdt= SYN_FLAG + ACK_FLAG;
  rdtHeader->nro_SEC_rdt=0;
  rdtHeader->nro_ACK_rdt=0;

  //envio el syn,ack
  int envioSA = sendto(miSocket, (char *)datagrama_syn_ack, sizeof(struct iphdr) + sizeof(struct rdt_header) , 0,(struct sockaddr *)&remote_addr, (socklen_t)sizeof(remote_addr));
  return envioSA;

  
}

int enviarSYN (int numeroDeSecuencia){
  //variable donde almaceno lo que voy a enviar, la seteo en 0
  //el tamaño es headerIP + headerRDT
  char datagrama_syn [sizeof(struct iphdr) + sizeof(struct rdt_header)];
  memset(datagrama_syn, 0, sizeof(struct iphdr) + sizeof(struct rdt_header));

  //posiciono los punteros dentro del  datagrama
  struct iphdr *ipHeader = (struct iphdr *)datagrama_syn;  
  struct rdt_header *rdtHeader = (struct rdt_header*) (datagrama_syn + sizeof(struct iphdr));

  //creo el datagrama a enviar				
  ipHeader->ihl = 5;
  ipHeader->version = 4;
  ipHeader->tos = 0;
  ipHeader->tot_len = htons(sizeof(struct iphdr) + sizeof(struct rdt_header));	/* tamaño total del datagrama, headerIP + datos*/
  ipHeader->frag_off = 0;		/* no fragment */
  ipHeader->ttl = 64;			/* default value */
  ipHeader->protocol = PROTOCOLO_RDT;	/* protocolo*/
  ipHeader->check = 0;			/* not needed in iphdr */
  ipHeader->saddr = myAddr.sin_addr.s_addr; /* direccion del source */
  ipHeader->daddr = remote_addr.sin_addr.s_addr; /* direccion del destination */

  //seteo el rdt_header
  rdtHeader->srcPort=myAddr.sin_port;
  rdtHeader->destPort=remote_addr.sin_port;
  rdtHeader->flags_rdt= SYN_FLAG;
  rdtHeader->nro_SEC_rdt=0;
  rdtHeader->nro_ACK_rdt=0;

  //envio el syn
  int envioS = sendto(miSocket, (char *)datagrama_syn, sizeof(struct iphdr) + sizeof(struct rdt_header) , 0,(struct sockaddr *)&remote_addr, (socklen_t)sizeof(remote_addr));
  return envioS;
}

int crearRDT(struct in_addr localIPaddr){
	if (estado == INICIO){
		estado = DESCONECTADO;
		myAddr.sin_family=AF_INET;
	   	myAddr.sin_port=0;     // se completa en la funcion aceptarRDT
		myAddr.sin_addr.s_addr = localIPaddr.s_addr;
		memset(myAddr.sin_zero, 0, sizeof(myAddr.sin_zero));
		miSocket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);	
		buffer = new buffer_t();

		buffer->expected_seq_number = 0;
		buffer->begin = 0;
		buffer->end = 0;
		buffer->cantLibres = MAX_LARGO_BUFFER_RECEIVER;
		close=false;

		if (miSocket < 0) {
			perror("error");
			estado = INICIO;
			return(-1);
		}
		pthread_t thread;
		int nada;
		int pidHijo = pthread_create( &thread, NULL, threadAttention, (void*)nada);
		if (pidHijo <0){
			perror("error");
			return -1;	
		}
		return 0;
	}
	else{
		fprintf(stderr,"llamaron al crear en un estado incorrecto\n");
		return -1;
	}
}

int aceptarRDT(unsigned char localRDTport){
  if (estado == DESCONECTADO){
    estado = ESTABLECIMIENTO_PASIVO;
    //myAddr.sin_port = htons(localRDTport);
    myAddr.sin_port =(localRDTport);
    socklen_t my_addr_size = sizeof myAddr;
//no estoy seguro de si va el bind y el listen
//	if (bind( miSocket, (struct sockaddr*) &myAddr, my_addr_size) < 0)
//		return -1;
    //aca tengo q recibir el syn, mandar syn,ack y esperar por el ack, cuando me llegue el ack cambio el estado a establecido_pas

    //variable donde almaceno lo recibido, la seteo en 0
    char recibido [MAX_IP_SIZE];
    memset(recibido, 0, sizeof(recibido));
    //int numSecuenciaEsperado =0;

    socklen_t *remote_addr_size = (socklen_t *)sizeof(remote_addr);
    int fromlen = sizeof(remote_addr);

    bool conexion = false;
    while (conexion == false){
	fprintf(stderr,"espero el syn\n");
	bool recibi_syn = false;
	while (recibi_syn == false){
	  //recibo un datagrama
	  int received_data_size = recvfrom(miSocket,(char*) &recibido, MAX_IP_SIZE, 0, (struct sockaddr*)&remote_addr, (socklen_t *)&fromlen);
	  if (received_data_size < 0){
	    printf("error en recibir syn\n");
	    estado = DESCONECTADO;
	    return -1;
	  }
	  else{
	    //chequeo si tiene el protocolo adecuado, los IPs y los puertos
	    //por ahora lo dejo asi, faltan los otros controles

	    //variable auxiliar para imprimir IPs
	    char charIP[20];
	    //extraigo el headerIP
	    struct iphdr *ipr = (struct iphdr *)recibido;

	    //paso el IP del q manda a char para imprimirlo
	    IP2asc(ntohl(ipr->saddr),&charIP[0]);
	    printf("Recepcion origen IP address: %s \n", charIP);

	    //idem al anterior, solo con el IP del destinatario
	    IP2asc(ntohl(ipr->daddr),&charIP[0]);
	    printf("Destino IP address: %s\n", charIP);

	    //imprimo el protocolo 
	    printf("protocolo: %d\n: ", ipr->protocol);


	    //extraigo el headerRDT
	    struct rdt_header *rdt = (struct rdt_header *)(recibido + sizeof(struct iphdr));	
		//asigno el puerto a la direccion remota
		remote_addr.sin_port =rdt->srcPort;
	    printf("srcPort %d\n", rdt->srcPort);		
	    printf("destPort %d\n", rdt->destPort);
	    printf("miPort %d\n",myAddr.sin_port);		
	    printf("remPort %d\n", remote_addr.sin_port);
	    printf("flags %d\n", rdt->flags_rdt);				
	    printf("nro_SEC_rdt %d\n", rdt->nro_SEC_rdt);		
	    printf("nro_ACK_rdt %d\n", rdt->nro_ACK_rdt);		

	    //verifico el protocolo, si no es RDT no me interesa
	    bool protocoloCorrecto;
	    if (ipr->protocol == PROTOCOLO_RDT)
		protocoloCorrecto = true;
	    else
		protocoloCorrecto = false;

	    bool esSyn;
	    //verifico que sea solo syn
	    unsigned char banderas = rdt->flags_rdt;
	    fprintf(stderr,"banderas %c\n",banderas);
	    fprintf(stderr,"flag %c\n",SYN_FLAG);						
	    if (banderas == SYN_FLAG)
		esSyn = true;
	    else
		esSyn = false;

	    bool miIP=false;
	    if (myAddr.sin_addr.s_addr == ipr->daddr)
		miIP = true;
	    else
		miIP = false;
	    //tendria que verificar el IP, si es mio o no, si es el mio, guardo el IP de el otro

	    bool miPuerto=false;
	    if (myAddr.sin_port == rdt->destPort)
		miPuerto= true;
	    else
		miPuerto = false;
	    //tendria que verificar el puerto, si es mio o no, si es el mio, guardo el puerto de el otro

	    if ( (protocoloCorrecto) && (esSyn) && (miIP) && (miPuerto)){
		recibi_syn =true;
		//numSecuenciaEsperado = rdt->nro_SEC_rdt +1;
	    }
	    else{
	      fprintf(stderr,"el syn que espero no es\n");
	    }

	  }//fin de if tamaño correcto					
	}//fin while recibi_syn

	fprintf(stderr,"recibi el syn\n");
	estado = SYN_RECIBIDO;

	//llevo la cantidad de intentos que espero por un ACK, simularia un timeout, el problema es que casi siempre recibo algo
	int cantidadIntentosACK1 = 0;

	//si recibi el ACK
	bool reciboACK1 = false;

	//envio el syn,ack
	//aca no va un 0 hay q cambiarlo
	int envioSA = enviarSYN_ACK(0);
	if (envioSA < 0){
	    fprintf(stderr,"error al enviar el syn,ack \n");
	    estado = ESTABLECIMIENTO_PASIVO;
	    recibi_syn =false;
	}
	else{
		fprintf(stderr,"envie el syn,ack \n");
		//si no recibo nada en x segundos
		bool timeout = false;
		//set de filedescriptors
		fd_set fds;
		int n;
		//estructura para el timeout
		struct timeval tv;

		//setea el file descriptor del que voy a leer
		FD_ZERO(&fds);
		FD_SET(miSocket, &fds);

		//setea el struct timeval para el timeout, habiamos quedado que en 1 segundo estaba bien
		tv.tv_sec = 3;
		tv.tv_usec = 0;		
		while (!reciboACK1 && !timeout){

		  //la magia del select			
		  n = select(miSocket + 1, &fds, NULL, NULL, &tv);
		  //salgo por timeout
		  if (n == 0){
		    fprintf(stderr,"no llego el ACK para realizar la conexion,sali por timeout\n");
		    estado = ESTABLECIMIENTO_PASIVO;
		    timeout=true;
		    recibi_syn =false;
		  }
		  else{
		    if (n < 0){
		      fprintf(stderr,"error al recibir para ACK para realizar la conexion\n");
		      estado = DESCONECTADO;
		      return -1;
		    }
		    else{		  		  		  		  	  
		      //datagrama ACK a recibir
		      char datagrama_ack [sizeof(struct iphdr) + sizeof(struct rdt_header)];
		      memset(datagrama_ack, 0, sizeof(struct iphdr) + sizeof(struct rdt_header));
		      struct sockaddr_in remote_addr2;
		      socklen_t *remote_addr_size2 = (socklen_t *) sizeof (remote_addr2);
		      int fromlen2 = sizeof(remote_addr2);

		      //en teoria esto tendria que funcionar, puedo leer sin problemas porque el select me devuelve en que socket tengo actividad y hay uno solo asi que no habria problemas
		      int tamRecibidoAck = recvfrom(miSocket, (char*) &datagrama_ack, sizeof(struct iphdr) + sizeof(struct rdt_header), 0, (struct sockaddr*) &remote_addr2, (socklen_t *)&fromlen2);
		      
		      
		      if (tamRecibidoAck < 0){
			  fprintf(stderr,"error al recibir para ACK1 para realizar la conexion\n");
			  estado = DESCONECTADO;
			  return -1;
		      }
		      else{				

			  char charIP[20];
			  //extraigo el headerIP
			  struct iphdr *ipr = (struct iphdr *)datagrama_ack;

			  //paso el IP del q manda a char para imprimirlo
			  IP2asc(ntohl(ipr->saddr),&charIP[0]);
			  printf("Recepcion origen IP address: %s \n", charIP);

			  //idem al anterior, solo con el IP del destinatario
			  IP2asc(ntohl(ipr->daddr),&charIP[0]);
			  printf("Destino IP address: %s: \n", charIP);

			  //imprimo el protocolo 
			  printf("protocolo: %d: ", ipr->protocol);

			  //extraigo el headerRDT
			  struct rdt_header *rdt = (struct rdt_header *)(datagrama_ack + sizeof(struct iphdr));
			  printf("srcPort %d\n", rdt->srcPort);		
			  printf("destPort %d\n", rdt->destPort);
			  printf("flags %d\n", rdt->flags_rdt);				
			  printf("nro_SEC_rdt %d\n", rdt->nro_SEC_rdt);		
			  printf("nro_ACK_rdt %d\n", rdt->nro_ACK_rdt);		

			  //verifico el protocolo
			  bool protocoloCorrecto;
			  if (ipr->protocol == PROTOCOLO_RDT)
				  protocoloCorrecto = true;
			  else
				  protocoloCorrecto = false;

			  //verifico que es ACK
			  bool esACK;
			  //verifico que sea solo ack
			  unsigned char banderas = rdt->flags_rdt;			
			  if (banderas == ACK_FLAG)
				  esACK = true;
			  else
				  esACK = false;
			  
			  //es mi IP
			  bool miIP=false;
			  if (myAddr.sin_addr.s_addr == ipr->daddr)
			      miIP = true;
			  else
			      miIP = false;

			  //es la IP del otro
			  bool suIP=false;
			  if (remote_addr.sin_addr.s_addr == ipr->saddr)
			      suIP = true;
			  else
			      suIP = false;

			  //es mi puerto
			  bool miPuerto=false;
			  if (myAddr.sin_port == rdt->destPort)
			      miPuerto= true;
			  else
			      miPuerto = false;

			  //es su puerto
			  bool suPuerto=false;
			  if (remote_addr.sin_port == rdt->srcPort)
			      suPuerto= true;
			  else
			      suPuerto = false;
			  
			  //faltaria controlar el numero de secuencia		  
			  if (protocoloCorrecto && esACK && miIP && suIP && miPuerto && suPuerto){	
				  fprintf(stderr,"recibi el ACK1 con exito \n");
				  reciboACK1 = true;
			  }
			  else{//el paquete no es para mi
				  fprintf(stderr,"no es el paquete que espero,espero ack \n");
			  }

		      }//fin else hubo error en el receive
		    }//fin else n<0
		  }//fin else select == 0
		}//fin while recibo ACK1
		if (reciboACK1){
		  conexion = true;
		  estado = ESTABLECIDO_PAS;
		}//finreciboACK1
	}//fin else envioSA<0
    }//fin while conexion	

    printf("estado: %d\n",estado);
    return 0;
  }
  else{
	  fprintf(stderr,"llamaron al aceptar en un estado incorrecto\n");
	  return -1;
  }
}

int conectarRDT(unsigned char localRDTport,unsigned char peerRDTPport, struct in_addr peerIPaddr){
  if (estado == DESCONECTADO){
    estado = ESTABLECIMIENTO_ACTIVO;
    //myAddr.sin_port = htons(localRDTport);
    myAddr.sin_port = (localRDTport);
    socklen_t my_addr_size = sizeof myAddr;

    //guardo la direccion del otro extremo
    remote_addr.sin_family=AF_INET;
    //remote_addr.sin_port= htons(peerRDTPport);
    remote_addr.sin_port= (peerRDTPport);
    remote_addr.sin_addr.s_addr = peerIPaddr.s_addr;
    memset(remote_addr.sin_zero, 0, sizeof(remote_addr.sin_zero));

    //envio el syn, esto tendria q cambiar
    int envioS = enviarSYN(0);
    if (envioS < 0){
      fprintf(stderr,"error al enviar syn\n");
      estado = DESCONECTADO;
      return -1;
    }
    else{
      //datagrama syn,ack a recibir
      fprintf(stderr,"envie syn\n");
      char datagrama_syn_ack [sizeof(struct iphdr) + sizeof(struct rdt_header)];
      memset(datagrama_syn_ack, 0, sizeof(struct iphdr) + sizeof(struct rdt_header));

      struct sockaddr_in remote_addr2;
      socklen_t *remote_addr_size2 = (socklen_t *) sizeof (remote_addr2);
      int fromlen2 = sizeof(remote_addr2);
      
      //booleano para saber si recibi el syn,ack
      bool recibi_syn_ack = false;
      int cantidadIntentosSYN_ACK = 0;
      
      //bool para salir por timeout,casi siempre voy a leer algo, asi q en caso de que no llegue nada salgo por timeout
      bool timeout = false;
      //set de filedescriptors
      fd_set fds;
      int n;
      //estructura para el timeout
      struct timeval tv;

      //setea el file descriptor del que voy a leer
      FD_ZERO(&fds);
      FD_SET(miSocket, &fds);

      //setea el struct timeval para el timeout, habiamos quedado que en 1 segundo estaba bien
      tv.tv_sec = 3;
      tv.tv_usec = 0;

      
      while (!recibi_syn_ack  && !timeout){
	//la magia del select			
	n = select(miSocket + 1, &fds, NULL, NULL, &tv);
	//salgo por timeout	      
	if (n == 0){
	  fprintf(stderr,"no llego el syn,ack para realizar la conexion,sali por timeout\n");
	  estado = DESCONECTADO;
	  timeout=true;
	}
	else{
	  if (n < 0){
	    fprintf(stderr,"error al recibir para syn,ack para realizar la conexion\n");
	    estado = DESCONECTADO;
	    return -1;
	  }
	  else{
	      int tamRecibidoSynAck = recvfrom(miSocket, (char*) &datagrama_syn_ack, sizeof(struct iphdr) + sizeof(struct rdt_header), 0, (struct sockaddr*) &remote_addr2, (socklen_t *)&fromlen2);
	      if (tamRecibidoSynAck < 0){
		fprintf(stderr,"error al recibir el syn,ack\n");
		estado = DESCONECTADO;
		return -1;
	      }
	      else{
		  char charIP[20];
		  //extraigo el headerIP
		  struct iphdr *ipr = (struct iphdr *)datagrama_syn_ack;

		  //paso el IP del q manda a char para imprimirlo
		  IP2asc(ntohl(ipr->saddr),&charIP[0]);
		  printf("Recepcion origen IP address: %s \n", charIP);

		  //idem al anterior, solo con el IP del destinatario
		  IP2asc(ntohl(ipr->daddr),&charIP[0]);
		  printf("Destino IP address: %s: \n", charIP);

		  //imprimo el protocolo 
		  printf("protocolo: %d: ", ipr->protocol);

		  //extraigo el headerRDT
		  struct rdt_header *rdt = (struct rdt_header *)(datagrama_syn_ack + sizeof(struct iphdr));
		  printf("srcPort %d\n", rdt->srcPort);		
		  printf("destPort %d\n", rdt->destPort);
		  printf("flags %d\n", rdt->flags_rdt);				
		  printf("nro_SEC_rdt %d\n", rdt->nro_SEC_rdt);		
		  printf("nro_ACK_rdt %d\n", rdt->nro_ACK_rdt);		

		  //verifico el protocolo
		  bool protocoloCorrecto;
		  if (ipr->protocol == PROTOCOLO_RDT)
			  protocoloCorrecto = true;
		  else
			  protocoloCorrecto = false;

		  //verifico que es SYN_ACK
		  bool es_SYN_ACK;
		  //verifico que sea solo ack
		  unsigned char banderas = rdt->flags_rdt;			
		  if (banderas == (ACK_FLAG + SYN_FLAG))
			  es_SYN_ACK = true;
		  else
			  es_SYN_ACK = false;
		  
		  //es mi IP
		  bool miIP=false;
		  if (myAddr.sin_addr.s_addr == ipr->daddr)
		      miIP = true;
		  else
		      miIP = false;

		  //es la IP del otro
		  bool suIP=false;
		  if (remote_addr.sin_addr.s_addr == ipr->saddr)
		      suIP = true;
		  else
		      suIP = false;

		  //es mi puerto
		  bool miPuerto=false;
		  if (myAddr.sin_port == rdt->destPort)
		      miPuerto= true;
		  else
		      miPuerto = false;

		  //es su puerto
		  bool suPuerto=false;
		  if (remote_addr.sin_port == rdt->srcPort)
		      suPuerto= true;
		  else
		      suPuerto = false;
		  
		  //faltaria controlar el numero de secuencia
		  
		  if (protocoloCorrecto && es_SYN_ACK && miIP && suIP && miPuerto && suPuerto){	
			  fprintf(stderr,"recibi el syn,ack con exito \n");
			  recibi_syn_ack = true;
		  }
		  else{//el paquete no es para mi
			  fprintf(stderr,"no es el paquete que espero \n");
		  }//fin else comparar si es el mensaje que quiero
	      }//fin else tamRecibido <0
	  }//fin else n<0
	}//fin else n ==0

      }//fin while recibi_syn_ack
    
      if (recibi_syn_ack){
	//recibi el syn,ack tengo que enviar el ack
	//esto no es 0, hay que cambiarlo...
	fprintf(stderr,"recibi el syn,ack\n");
	
	int enviado_ack = enviarACK(0);
	if (enviado_ack <0){
	  fprintf(stderr,"error al enviar ack\n");
	  estado = DESCONECTADO;
	  return -1;
	}
	else{
	  fprintf(stderr,"envie el ack\n");
	  estado = ESTABLECIDO_ACT;
	  return 0;
	}
      }
      else{
	//no recibi el syn,ack sali por timeout
	fprintf(stderr,"no recibi syn,ack, sali por timeout o max_cantidad_intentos...\n");
	estado = DESCONECTADO;
	return -1;
      }
    }
  }//fin if estado
  else{
    fprintf(stderr,"llamaron al conectar en un estado incorrecto\n");
    return -1;
  }

}

int escribirRDT(const void *buf, size_t len){
	if (estado==ESTABLECIDO_ACT){
		pthread_mutex_lock(&semBuffer);
		int minLen = 0;
		fprintf(stderr,"cant%d.\n",buffer->cantLibres);
		if (buffer->cantLibres>0){
		  if ( buffer->cantLibres > len)
		    minLen = len;
		  else
		    minLen = buffer->cantLibres;
		  
		  char *aux = (char *)buf;
		  for(int i=0; i < minLen; i++){
		    buffer->arreglo[buffer->end] = aux[i];
		    buffer->end = ((buffer->end + 1) % MAX_LARGO_BUFFER_RECEIVER);
		    buffer->cantLibres--;
		  }		  
		  
		}else{
		  fprintf(stderr,"el buffer se encuentra lleno, no se puede escribir informacion.\n");
		  pthread_mutex_unlock(&semBuffer);
		  return -1;
		}
		pthread_mutex_unlock(&semBuffer);
		return minLen;
	}else{
	  fprintf(stderr,"estado,%d\n",estado);
		fprintf(stderr,"intento de escribir, pero no se encuentra en estado ESTABLECIDO_ACT\n");
		return -1;
	}
}


int leerRDT(void *buf, size_t len){
  if ((estado == ESTABLECIDO_PAS) || (estado == ESPERANDO_FIN)){
    pthread_mutex_lock(&semBuffer);
    if (buffer->cantLibres == MAX_LARGO_BUFFER_RECEIVER){
      pthread_mutex_unlock(&semBuffer);
      return 0;
    }else{
      int min;
      if ((MAX_LARGO_BUFFER_RECEIVER - buffer->cantLibres) > len)
	min = len;
      else
	min = (MAX_LARGO_BUFFER_RECEIVER - buffer->cantLibres);
      
      char *aux = (char*)buf;
      for(int i=0; i < min; i++){
	aux [i] = buffer->arreglo[buffer->begin];
	buffer->begin = ((buffer->begin + 1) % MAX_LARGO_BUFFER_RECEIVER);
	buffer->cantLibres = (buffer->cantLibres + 1);
      }				  
      pthread_mutex_unlock(&semBuffer);
      return min;
    }
  }
  else{
    fprintf(stderr,"intento de leer, pero no se encuentra en estado ESTABLECIDO_PAS\n");
    return -1;
  }
}

int cerrarRDT(){
  if (estado==ESTABLECIDO_ACT){
    estado=TERMINAR_ENVIAR;
    return 0;
  }else{
    fprintf(stderr,"intento de cerrar, pero no se encuentra en estado ESTABLECIDO_ACT\n");
    return -1;
  }
}

