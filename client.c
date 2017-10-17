#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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
	if (argc < 6) {
		perror("Execution Format : ./sendfile <filename> <windowsize> <buffersize> <destination_ip> <destionation_port>");
		exit(1);
	}

	// Input execution arguments to variables
	char* filename = argv[1];
	int windowsize = atoi(argv[2]);
	int buffersize = atoi(argv[3]);
	char* destination_ip = argv[4];
	int destination_port = atoi(argv[5]);

	// Variables to be used
    int sock,bytes_recv,sin_size;
	struct sockaddr_in server_addr;
	struct hostent *host;
	char send_data[buffersize],recv_data[buffersize];

	// Create UDP socket
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}

	// Configure socket
	host = (struct hostent *) gethostbyname((char *)destination_ip);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(destination_port);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(server_addr.sin_zero),8);
	sin_size = sizeof(struct sockaddr);

	// Variables
	char buff[buffersize];
	segment seg;
	acks ackseg;
	char segstr[9];
	FILE *f; 
	f = fopen(filename, "r");
	int i = 0;
	int window_left = 1;
	int window_right = window_left + windowsize - 1;
	int curr_window = 1;
	int ack_target = 1;

	// Write buffer from file
	fgets(buff, buffersize, f);
	if (f == NULL) {
		perror("File");
		exit(1);
	}
	else {
		// Start
		while (i < strlen(buff)) {
			// Send message
			while (i < strlen(buff) && curr_window >= window_left && curr_window <= window_right) {
				// Create segment from buffer
				seg.soh = '\01';
				seg.sequencenumber = i+1;
				seg.stx = '\02';
				seg.data = buff[i];
				seg.etx = '\03';
				seg.checksum = 'c';

				// Make string from segment
				segmentToString(&seg, segstr);

				// Send segment
				sendto(sock, segstr, 9, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));

				// Test segment
				printf("\n*SENT SEGMENT %d*\n",seg.sequencenumber);
				printSegment(seg);
				fflush(stdout);

				i++;
				curr_window++;
			}

			// Receive ACK
			int countack = 0;
			int checkack[4];
			int valintcheck[4];
			for (int j=0;j<4;j++) {
				checkack[j] = i+j-4;
				valintcheck[j] = 0;
			}
			int k = 0;
			int iter = 0;
			do {
				// Receive ACK
				char ackstr[7];
				if (bytes_recv = recvfrom(sock, ackstr, 7, 0, (struct sockaddr *)&server_addr, (socklen_t*)&sin_size) >= 0) {
					// Test ACK
					stringToACK(ackstr, &ackseg);
					checkack[k] = ackseg.nextsequencenumber-windowsize;
					valintcheck[k]++;
					printf("\n*RECEIVED ARKS %d*\n",ackseg.nextsequencenumber-windowsize);
					printACK(ackseg);
					fflush(stdout);
					countack++;
					k++;
				}
				iter++;
			} while (countack < windowsize && iter < 8);
			if (countack < windowsize) {
				// Retransmit
				for (int l=0;l<4;l++) {
					if (valintcheck[l] == 0) {
						// Create segment from buffer
						seg.soh = '\01';
						seg.sequencenumber = checkack[l];
						seg.stx = '\02';
						seg.data = buff[checkack[l]];
						seg.etx = '\03';
						seg.checksum = 'c';

						// Make string from segment
						segmentToString(&seg, segstr);

						// Send segment
						sendto(sock, segstr, 9, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));

						// Test segment
						printf("\n*SENT SEGMENT %d*\n",seg.sequencenumber);
						printSegment(seg);
						fflush(stdout);
					}
				}
			}
			else {
				// Expand window
				window_left += windowsize;
				window_right += windowsize;
				printf("\n*WINDOW EXPANDED*\n");
			}
		}
	}
	fclose(f);

	close(sock);
	return 0;
}