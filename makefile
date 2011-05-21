all: enviaFile recibeFile receive send


recibeFile: recibeFile.cc rdt.o
	g++ -pthread -o recibeFile recibeFile.cc rdt.o 

enviaFile: enviaFile.cc rdt.o
	g++ -pthread -o enviaFile enviaFile.cc rdt.o 


send: send.c
	g++ -o send send.c

receive: receive.c
	g++ -o receive receive.c

clean:
	rm *.o
	rm recibeFile
	rm enviaFile
