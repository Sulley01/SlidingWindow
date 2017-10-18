#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctime>
#include "segment.h"
#include <fstream>
using namespace std;

int8_t CheckSumSegment(segment segdata){
	int8_t checksum = 0;
	int8_t* segtemp = (int8_t*)&segdata;
	int8_t i;
	for(i=0; i<8; i++){
		checksum += sizeof(segtemp[i]);
	}
	return checksum;
}

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

	time_t logtimenow;
	struct tm * now;
	ofstream ofs;
	ofs.open("logfile.txt",std::ofstream::out | std::ofstream::app);
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
    logtimenow=time(0);
	now = localtime(&logtimenow);
	ofs<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday << " - ";
	ofs <<now->tm_hour << ":";
   	ofs <<now->tm_min << ":";
   	ofs <<now->tm_sec << " ";
   	ofs <<"RECEIVER :";
   	ofs <<" UDPServer Waiting for client on port ";
   	ofs<<port<<endl;
    printf("\nUDPServer Waiting for client on port %d\n", port);
    fflush(stdout);

	// Variables
	char buff[buffersize];
	char bufftemp[9];
	int i = 0;
	segment seg;
	acks ackseg;
	char ackstr[7];
    FILE *f;
	f = fopen(filename, "w+");

	// Start
    while (1) {
    	// Read buffer
		bytes_read = recvfrom(sock, bufftemp, 9, 0, (struct sockaddr *)&client_addr, (socklen_t*)&addr_len);
		if (bytes_read < 0) {
			perror("Buffer");
			exit(1);
		}
		bufftemp[bytes_read] = '\0';

		// EOF
		if (bufftemp[0] == '\00') {
			for (int k=6;k<i;k=k+9) {
				fputc(buff[k], f);
			}
			exit(1);
		}

		// Create segment from buffer
		if (*bufftemp == '\01' && *(bufftemp+5) == '\02' && *(bufftemp+7) == '\03') {
			seg.soh = *bufftemp;
			seg.sequencenumber = *(bufftemp+1);
			seg.stx = *(bufftemp+5);
			seg.data = *(bufftemp+6);
			seg.etx = *(bufftemp+7);
			seg.checksum = *(bufftemp+8);

			// Test segment
			logtimenow=time(0);
			now = localtime(&logtimenow);
			ofs<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday << " - ";
			ofs <<now->tm_hour << ":";
		   	ofs <<now->tm_min << ":";
		   	ofs <<now->tm_sec << " ";
		   	ofs <<"RECEIVER :";
		   	ofs <<" RECEIVED SEGMENT :";
		   	ofs<<seg.sequencenumber<<endl;
			printf("\n*RECEIVED SEGMENT %d*\n",seg.sequencenumber);
			printSegment(seg);
			fflush(stdout);

			// Check segment checksum
			if (seg.checksum == CheckSumSegment(seg)) {
				// Input segment to buffer
				for (int j=0;j<9;j++) {
					buff[i] = bufftemp[j];
					i++;
					if (i == buffersize) {
						// Write segment to file
						for (int k=6;k<buffersize;k=k+9) {
							fputc(buff[k], f);
						}
						i = 0;
					}
				}

				// Make ACK
				ackseg.ack = '\06';
				ackseg.nextsequencenumber = seg.sequencenumber + windowsize;
				ackseg.advertisedwindowsize = windowsize;
				ackseg.checksum = 6;

				// Send string from ACK
				ACKToString(&ackseg, ackstr);
				sendto(sock, ackstr, 7, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));

				// Test ACK
				logtimenow=time(0);
				now = localtime(&logtimenow);
				ofs<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday << " - ";
				ofs <<now->tm_hour << ":";
			   	ofs <<now->tm_min << ":";
			   	ofs <<now->tm_sec << " ";
			   	ofs <<"RECEIVER :";
			   	ofs <<" SENT ACKS :";
			   	ofs<<seg.sequencenumber<<endl;
				printf("\n*SENT ACKS %d*\n",seg.sequencenumber);
				printACK(ackseg);
				fflush(stdout);
			}
			else {
				// Input segment to buffer
				for (int j=0;j<9;j++) {
					buff[i] = bufftemp[j];
					i++;
					if (i == buffersize) {
						// Write segment to file
						for (int k=6;k<buffersize;k=k+9) {
							fputc(buff[k], f);
						}
						i = 0;
					}
				}

				// Make broken ACK
				ackseg.ack = '\00';
				ackseg.nextsequencenumber = seg.sequencenumber + windowsize;
				ackseg.advertisedwindowsize = windowsize;
				ackseg.checksum = 6;

				// Send string from ACK
				ACKToString(&ackseg, ackstr);
				sendto(sock, ackstr, 7, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));

				// Test ACK
				logtimenow=time(0);
				now = localtime(&logtimenow);
				ofs<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday << " - ";
				ofs <<now->tm_hour << ":";
			   	ofs <<now->tm_min << ":";
			   	ofs <<now->tm_sec << " ";
			   	ofs <<"RECEIVER :";
			   	ofs <<" SENT BROKEN ACKS :";
			   	ofs<<seg.sequencenumber<<endl;
				printf("\n*SENT BROKEN ACKS %d*\n",seg.sequencenumber);
				printACK(ackseg);
				fflush(stdout);
			}
		}
    }
    fclose(f);
    ofs.close();
    close(sock);

    return 0;
}