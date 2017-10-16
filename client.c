#include <sys/types.h>
#include <netinet/in.h>
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
		perror("Execution Format : ./sendfile <filename> <windowsize> <buffersize> <destination_ip> <destionation_port>");
		exit(1);
	}

	char* filename = argv[1];
	int windowsize = stringToInt(argv[2]);
	int buffersize = stringToInt(argv[3]);
	int destination_ip = stringToInt(argv[4]);
	int destination_port = stringToInt(argv[5]);

    int sock;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket error");
        exit(1);
    }

	struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(destination_port);
    server_addr.sin_addr.s_addr= htonl(INADDR_ANY);
    bzero(&(server_addr.sin_zero),8);
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("Connect error");
        exit(1);
    }

    int val,i,count;
    char recvdata[256],senddata[256];
    while(1) {
    	printf("\nEnter packet number : ");
    	scanf("%d",&val);
    	send(sock,&val,sizeof(val),0);
    	printf("Enter data : ");
        scanf("%s",senddata);
        send(sock,senddata,strlen(senddata),0);
        recv(sock,&count,sizeof(count),0);
        i = recv(sock,recvdata,256,0);
        recvdata[i]='\0';
        printf("%s %d",recvdata,count);
    }

    close(sock);
	return 0;
}