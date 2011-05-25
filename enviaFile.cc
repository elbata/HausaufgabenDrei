
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


    if (argc != 6){
        printf( "Error, ejecutar %s <puerto_local> <direccionIP_local> <puerto_remoto>  <direccionIP_remoto> <nombre_archivo>\n",argv[0]);
        exit(-1);
    }

    // cargo variables de la conexion
    puerto=atoi( argv[1] );
    puertoOrig=(atoi( argv[1] ) % 256);
    puertoO = (unsigned char) puertoOrig;
    puertoDest=(atoi( argv[3] ) % 256);
    puertoD = (unsigned char) puertoDest;


    // configuro direccion del equipo local
    inet_aton(argv[2], &dirOrig);
    IdRDT = crearRDT(dirOrig);
    if ( IdRDT  == -1 ){
        printf( "Error al crear el Socket\n");
        exit(-1);
    }
    printf( "Conexion creada\n");

    // Configuro direccion remota y conecto
    inet_aton(argv[4], &dirDest);
    result = conectarRDT(puertoO,puertoD,dirDest);
    if ( result == -1 ) {
        printf( "Error al conectar Socket\n");
        exit(-1);
    }
    printf( "Equipo conectado\n");
	
    // abro archivo a transmitir
    FILE* archivocliente = fopen(argv[5], "rb");
    if ( archivocliente == NULL ) {
    	printf("Error al abrir el archivo\n");
	exit(-1);
    }
    fseek (archivocliente , 0 , SEEK_END);
    lSize = ftell (archivocliente);
    printf( "Preparando para envio de archivo de largo %ld\n",lSize);
    rewind (archivocliente);
    char buf[lSize];
    //copio el archivo en el buffer
    result = fread (buf,1,lSize,archivocliente);
    if (result != lSize) {
    	printf("Error al leer el archivo\n");
        exit(-1);
    }
    fclose(archivocliente);

    // comienzo proceso de envio
    int enviado  = 0;
    int buffSize = 0;
    cantInt      = 0;
    
    char alpedo;
    scanf("%c",&alpedo);
    printf("Envio Archivo de largo %ld \n",lSize);
    while (enviado < lSize) {
	buffSize = MAX_READ_SIZE;
	if ( (lSize - enviado) < MAX_READ_SIZE ){
		buffSize = lSize - enviado;
	}
	result = escribirRDT(&buf[enviado], buffSize);
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
	/*if ( cantInt > 50 ){ 
        	enviado = lSize;
	}*/
 	sleep(5);
	cantInt++;
    }
    printf("Archivo enviado\n");

 printf("ENVIEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE %d\n",enviado);
    // cierro conexion
    cerrarRDT();
    printf("Cerrando sesion RDT\n");
//    sleep(5);
    printf("Envio Finalizado\n");

    
}


