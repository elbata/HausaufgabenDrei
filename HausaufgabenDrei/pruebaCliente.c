
#include "rdt.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <iostream>

using namespace std;

#define MAX_INTENTOS	10

int main( int argc, const char* argv[] )
{
    int IdRDT,result,cantEnvio;
    int count,cantInt,puerto, puertoOrig,puertoDest, tam_archivo;
    unsigned char puertoO,puertoD;

    
    struct in_addr dirOrig;
    struct in_addr dirDest;
    long lSize;


    /*if (argc != 6){
        printf( "Error, ejecutar %s <puerto_local> <direccionIP_local> <puerto_remoto>  <direccionIP_remoto> <nombre_archivo>\n",argv[0]);
        exit(-1);
    }*/

    // cargo variables de la conexion
   // puerto=atoi( argv[1] );
    puertoOrig=(1);
    puertoO = (unsigned char) puertoOrig;
    puertoDest=(8);
    puertoD = (unsigned char) puertoDest;


    // configuro direccion del equipo local
    inet_aton("127.0.0.2", &dirOrig);
    IdRDT = crearRDT(dirOrig);
    if ( IdRDT  == -1 ){
        printf( "Error al crear el Socket\n");
        exit(-1);
    }
    printf( "Conexion creada\n");

    // Configuro direccion remota y conecto
    inet_aton("127.0.0.1", &dirDest);
    result = conectarRDT(puertoO,puertoD,dirDest);
    if ( result == -1 ) {
        printf( "Error al conectar Socket\n");
        exit(-1);
    }
    printf( "Equipo conectado\n");
    
    char arreglo_a_enviar[2048];
    for (int i=0; i<2048; i++)
      arreglo_a_enviar[i]=i%256;
    printf("Envio Archivo de largo %ld \n",lSize);
    int enviado  = 0;
    int buffSize = 0;
    cantInt      = 0;
    lSize =2048;
    while (enviado < lSize) {
	buffSize = MAX_READ_SIZE;
	if ( (lSize - enviado) < MAX_READ_SIZE ){
		buffSize = lSize - enviado;
	}
	result = escribirRDT(&arreglo_a_enviar[enviado], buffSize);
        if ( result == -1 ){
            printf("Error al enviar datos\n");
            sleep(3);
	    result = 0;
        } 
	if(result >= 0){
            usleep(500);
            printf("envie %d bytes de datos\n",result);
	    enviado = enviado + result;
        }
	if ( cantInt > 50 ){ 
        	enviado = lSize;
	}
 	sleep(5);
	cantInt++;
    }
    while(1){}
    printf("Archivo enviado\n");
}
