#!/bin/bash
echo "compilando..."
g++ -c -lpthread rdt.c
g++ -c send.c
g++ -c -lpthread prueba.c
g++ -c -lpthread pruebaCliente.c
g++ send.c -o send
g++ rdt.c prueba.c -lpthread -o prueba
g++ rdt.c pruebaCliente.c -lpthread -o pruebaCliente
echo "compilado con exito"