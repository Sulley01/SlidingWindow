#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int stringToInt(char* s) {
	int x = 0;
	while (*s != 0) {
		x = x*10 + (int)(*s++)-'0';
	}
	return x;
}

int main(int argc, char** argv) {
	if (argc < 5) {
		perror("Execution Format : ./recvfile <filename> <windowsize> <buffersize> <port>");
		exit(1);
	}

	char* filename = argv[1];
	int windowsize = stringToInt(argv[2]);
	int buffersize = stringToInt(argv[3]);
	int port = stringToInt(argv[4]);

	printf("\nServer running\n");
	int sock;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket error");
        exit(-1);
    }

    struct sockaddr_in ser;
    ser.sin_family = AF_INET;
    ser.sin_port = htons(port);
    ser.sin_addr.s_addr=INADDR_ANY;
    bzero(&(ser.sin_zero),8);
    if (bind(sock,(struct sockaddr *)&ser,sizeof(struct sockaddr)) == -1) {
        perror("Bind error");
        exit(-1);
    }
    if (listen(sock,2) == -1) {
        perror("Listen error");
        exit(-1);
    }
    printf("Waiting for connection\n");

    int size = sizeof(struct sockaddr);
    struct sockaddr_in cli;
    int connect = accept(sock,(struct sockaddr *)&cli,&size);
    if (connect == -1) {
        perror("Connection failed");
        exit(-1);
    }
    printf("Connected successfully\n");

    int val,count,i;
    recv(connect,&val,sizeof(val),0);
    count = val;
    char senddata[256],recvdata[256];
    while(1) {
        i = recv(connect,&recvdata,sizeof(recvdata),0);
        recvdata[i]='\0';
        if (count != val) {
            strcpy(senddata,"Packet missing : ");
            send(connect,&count,sizeof(count),0);
            send(connect,senddata,strlen(senddata),0);
        }
        else {
            printf("Packet Number : %d\n",val);
            printf("Data : %s\n",recvdata);
            count++;
            strcpy(senddata,"Send next :");
            send(connect,&count,sizeof(count),0);
            send(connect,senddata,strlen(senddata),0);
        }
        printf("The expected packet now is: %d\n",count);
        recv(connect,&val,sizeof(val),0);
    }

    close(connect);
    close(sock);
	return 0;
}