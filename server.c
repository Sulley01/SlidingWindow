#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "segment.h"

void stringToSegment(char* s, segment* seg) {
	seg->soh = *s;
	seg->sequencenumber = *(s + 1);
	seg->stx = *(s + 5);
    seg->data = *(s + 6);
    seg->etx = *(s + 7);
	seg->checksum = *(s + 8);
}

void segmentToString(segment* seg, char* s) {
	*s = seg->soh;
	*(s+1) = seg->sequencenumber;
	*(s+5) = seg->stx;
	*(s+6) = seg->data;
	*(s+7) = seg->etx;
	*(s+8) = seg->checksum;
	*(s+9) = '\0';
}

void stringToACK(char* s, acks* ackseg) {
	ackseg->ack = *s;
	ackseg->nextsequencenumber = *(s + 1);
	ackseg->advertisedwindowsize = *(s + 5);
    ackseg->checksum = *(s + 6);
}

void ACKToString(acks* seg, char* s) {
	*s = seg->ack;
	*(s+1) = seg->nextsequencenumber;
	*(s+5) = seg->advertisedwindowsize;
	*(s+6) = seg->checksum;
	*(s+7) = '\0';
}

void printSegment(segment seg) {
	printf("SOH : 0x%02x\n", seg.soh);
	printf("SeqNum : 0x%02x (%d in decimal)\n", seg.sequencenumber, seg.sequencenumber);
	printf("STX : 0x%02x\n", seg.stx);
	printf("Data : 0x%02x\n", seg.data & 0xff);
	printf("ETX : 0x%02x\n", seg.etx);
	printf("Checksum : 0x%02x\n", seg.checksum & 0xff);
}

void printACK(acks ackseg) {
	printf("ACK : 0x%02x\n", ackseg.ack);
	printf("NextSeqNum : 0x%02x (%d in decimal)\n", ackseg.nextsequencenumber, ackseg.nextsequencenumber);
	printf("ADV Window Size: 0x%02x\n", ackseg.advertisedwindowsize);
	printf("Checksum : 0x%02x\n", ackseg.checksum & 0xff);
}

int main(int argc, char** argv) {
	// Validate execution format
	if (argc < 5) {
		perror("Execution Format : ./recvfile <filename> <windowsize> <buffersize> <port>");
		exit(1);
	}

	// Input execution arguments to variables
	char* filename = argv[1];
	int windowsize = atoi(argv[2]);
	int buffersize = atoi(argv[3]);
	int port = atoi(argv[4]);

	// Variables to be used
	int sock;
    int addr_len, bytes_read;
    char recv_data[buffersize],send_data[buffersize];
    struct sockaddr_in server_addr , client_addr;

    // Create UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Socket");
        exit(1);
    }

    // Configure socket
    memset(&server_addr, 0, sizeof server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(sock,(struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("Bind");
        exit(1);
    }

    addr_len = sizeof(struct sockaddr);

    // Wait for client
    printf("\nUDPServer Waiting for client on port %d\n", port);
    fflush(stdout);

	// Variables
	char buff[buffersize];
	segment seg;
	acks ackseg;
	char ackstr[7];
    FILE *f;
	f = fopen(filename, "w+");

	// Start
    while (1) {
    	// Read buffer
		bytes_read = recvfrom(sock, buff, 9, 0, (struct sockaddr *)&client_addr, (socklen_t*)&addr_len);
		if (bytes_read < 0) {
			perror("Buffer");
			exit(1);
		}
		buff[bytes_read] = '\0';

		// Create segment from buffer
		if (*buff == '\01' && *(buff+5) == '\02' && *(buff+7) == '\03') {
			seg.soh = *buff;
			seg.sequencenumber = *(buff+1);
			seg.stx = *(buff+5);
			seg.data = *(buff+6);
			seg.etx = *(buff+7);
			seg.checksum = *(buff+8);

			// Test segment
			printf("\n*RECEIVED SEGMENT %d*\n",seg.sequencenumber);
			printSegment(seg);
			fflush(stdout);

			// Write segment to file
			fputc(seg.data, f);

			//if (checksum) {
				// Make ACK
				ackseg.ack = '\06';
				ackseg.nextsequencenumber = seg.sequencenumber + windowsize;
				ackseg.advertisedwindowsize = windowsize;
				ackseg.checksum = 'c';

				// Make string from ACK
				ACKToString(&ackseg, ackstr);

				// Send ACK
				sendto(sock, ackstr, 7, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));

				// Test ACK
				printf("\n*SENT ACKS %d*\n",seg.sequencenumber);
				printACK(ackseg);
				fflush(stdout);
			//}
		}

		if (buff[6] == '.') {
			break;
		}
    }
    fclose(f);

    close(sock);
    return 0;
}