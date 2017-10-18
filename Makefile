CC=g++

default: sendfile recvfile

all: sendfile recvfile

server.o: src/server.cpp src/segment.h
	$(CC) -c src/server.cpp

client.o: src/client.cpp src/segment.h
	$(CC) -c src/client.cpp

sendfile: client.o
	$(CC) -o sendfile client.o

recvfile: server.o
	$(CC) -o recvfile server.o

clean:
	rm - f *.0