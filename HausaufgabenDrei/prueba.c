#include "rdt.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <netdb.h>
#include <stddef.h>
#include <iostream>

#define PUERTO 8
int main( int argc, const char* argv[] )
{
    int IdRDT,result,i;
    int cantInt,puerto, puertoOrig,puertoDest, tam_archivo;
    unsigned char puertoO,puertoD;    
    struct in_addr direccion;

  //  FILE* archivo;

/*    if (argc != 4){
        printf( "Error, ejecutar %s <puerto_orig> <direccionIP_local> <nom_archivo_salida>\n",argv[0]);
        exit(-1);
    }*/

    // cargo variables para conexion
    //puertoOrig=(atoi( argv[1] ) % 256);
    puertoO = (unsigned char) 8;
    unsigned char buf[MAX_READ_SIZE];
    inet_aton("127.0.0.1", &direccion);
    IdRDT = crearRDT(direccion);
        printf( "puerto %d\n",puertoO);
        printf( "puerto %d\n",htons(puertoO));
    if ( IdRDT  == -1 ){
        printf( "Error al crear el Socket\n");
        exit(-1);
    }
    printf( "Esperando Conexion\n");

    result = aceptarRDT(puertoO);
    if ( result == -1 ) {
        printf( "Error al aceptar conexion\n");
        exit(-1);
    }

 printf( "no aparezco nunca\n");
 while(1){};
return 0;

}
