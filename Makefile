CC=g++

default: sendfile recvfile

all: sendfile recvfile

server.o: server.cpp segment.h
$(CC) -c server.cpp

client.o: client.cpp segment.h
$(CC) -c client.cpp

sendfile: client.o
$(CC) -o sendfile client.o

recvfile: server.o
$(CC) -o recvfile server.o

clean:
	rm - f *.0