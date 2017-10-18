#include <sys/types.h>
#include <iostream>
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
#include <fstream> 
#include <ctime>
#include "segment.h"
using namespace std;

int8_t CheckSumACK(acks ackdata){
	int8_t checksum = 0;
	int8_t* acktemp = (int8_t*)&ackdata;
	int8_t i;
	for(i=0; i<6; i++){
		checksum += sizeof(acktemp[i]);
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
	// Validate execution format
	time_t logtimenow;
	struct tm * now;
	ofstream ofs;
  	ofs.open ("logfile.txt", std::ofstream::out | std::ofstream::app);

	if (argc < 6) {
		perror("Execution Format : ./sendfile <filename> <windowsize> <buffersize> <destination_ip> <destination_port>");
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
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    	perror("Timeout");
	}

	// Variables
	char buff[buffersize];
	segment seg;
	acks ackseg;
	char segstr[9];
	FILE *f; 
	f = fopen(filename, "r");
	int window_left = 1;
	int window_right = window_left + windowsize - 1;
	int curr_window = 1;
	int ack_target = 1;
	int bufflen = 0;

	// Start
	while (fgets(buff, buffersize, f) != NULL) {
		int i = 0;
		buff[strlen(buff)] = '\0';
		// Send message
		while (i < strlen(buff)) {
			while (i < strlen(buff) && curr_window >= window_left && curr_window <= window_right) {
				// Create segment from buffer
				seg.soh = '\01';
				seg.sequencenumber = i+1+bufflen;
				seg.stx = '\02';
				seg.data = buff[i];
				seg.etx = '\03';
				seg.checksum = 8;

				// Make string from segment
				segmentToString(&seg, segstr);

				// Send segment
				sendto(sock, segstr, 9, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));

				// Test segment
				logtimenow=time(0);
				now = localtime(&logtimenow);
				ofs<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday << " - ";
				ofs <<now->tm_hour << ":";
			   	ofs <<now->tm_min << ":";
			   	ofs <<now->tm_sec << " ";
			   	ofs <<"SENDER : ";
			   	ofs <<"SENDER SENT SEGMENT ";
			   	ofs<<seg.sequencenumber<<endl;
				printf("\n*SENT SEGMENT %d*\n",seg.sequencenumber);
				printSegment(seg);
				fflush(stdout);

				i++;
				curr_window++;
			}

			// Initialize receive ACK
			int countwindow = i;
			while (countwindow-4 > 0) {
				countwindow -= 4;
			}
			int countack = 0;
			int checkack[countwindow];
			int valintcheck[countwindow];
			for (int j=0;j<countwindow;j++) {
				checkack[j] = i+j-countwindow+1;
				valintcheck[j] = 0;
			}
			int k = 0;
			int iter = 0;
			do {
				// Receive ACK
				char ackstr[7];
				bytes_recv = recvfrom(sock, ackstr, 7, 0, (struct sockaddr *)&server_addr, (socklen_t*)&sin_size);

				// Test ACK
				stringToACK(ackstr, &ackseg);
				if (ackseg.ack == '\06' && ackseg.checksum == CheckSumACK(ackseg)) {
					valintcheck[k]++;
					countack++;
					logtimenow=time(0);
					now = localtime(&logtimenow);
					ofs<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday << " - ";
					ofs <<now->tm_hour << ":";
				   	ofs <<now->tm_min << ":";
				   	ofs <<now->tm_sec << " ";
				   	ofs <<"SENDER : ";
				   	ofs <<"RECEIVED ACKS ";
				   	ofs <<ackseg.nextsequencenumber-windowsize<<endl;
					printf("\n*RECEIVED ACKS %d*\n",ackseg.nextsequencenumber-windowsize);
				}
				else {
					logtimenow=time(0);
					now = localtime(&logtimenow);
					ofs<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday << " - ";
					ofs <<now->tm_hour << ":";
				   	ofs <<now->tm_min << ":";
				   	ofs <<now->tm_sec << " ";
				   	ofs <<"SENDER : ";
				   	ofs <<"RECEIVED BROKEN ACKS ";
				   	ofs <<ackseg.nextsequencenumber-windowsize<<endl;
					printf("\n*RECEIVED BROKEN ACKS %d*\n",ackseg.nextsequencenumber-windowsize);
				}
				printACK(ackseg);
				fflush(stdout);
				k++;
				iter++;
			}
	 		while (k < countwindow && iter < 8);

			while (countack < countwindow) {
				// Retransmit
				for (int l=0;l<countwindow;l++) {
					if (valintcheck[l] == 0) {
						// Create segment from buffer
						seg.soh = '\01';
						seg.sequencenumber = checkack[l];
						seg.stx = '\02';
						seg.data = buff[checkack[l]];
						seg.etx = '\03';
						seg.checksum = 8;

						// Make string from segment
						segmentToString(&seg, segstr);

						// Send segment
						sendto(sock, segstr, 9, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));

						// Test segment
						logtimenow=time(0);
						now = localtime(&logtimenow);
						ofs<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday << " - ";
						ofs <<now->tm_hour << ":";
					   	ofs <<now->tm_min << ":";
					   	ofs <<now->tm_sec << " ";
					   	ofs <<"SENDER : ";
					   	ofs <<"SENT SEGMENT ";
					   	ofs <<seg.sequencenumber<<endl;
						printf("\n*SENT SEGMENT %d*\n",seg.sequencenumber);
						printSegment(seg);
						fflush(stdout);

						// Receive ACK
						char ackstr[7];
						bytes_recv = recvfrom(sock, ackstr, 7, 0, (struct sockaddr *)&server_addr, (socklen_t*)&sin_size);

						// Test ACK
						stringToACK(ackstr, &ackseg);
						if (ackseg.ack == '\06' && ackseg.checksum == CheckSumACK(ackseg)) {
							valintcheck[k]++;
							countack++;
							logtimenow=time(0);
							now = localtime(&logtimenow);
							ofs<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday << " - ";
							ofs <<now->tm_hour << ":";
						   	ofs <<now->tm_min << ":";
						   	ofs <<now->tm_sec << " ";
						   	ofs <<"SENDER : ";
						   	ofs <<"RECEIVED ACKS ";
						   	ofs <<ackseg.nextsequencenumber-windowsize<<endl;
							printf("\n*RECEIVED ACKS %d*\n",ackseg.nextsequencenumber-windowsize);
						}
						else {
							logtimenow=time(0);
							now = localtime(&logtimenow);
							ofs<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday << " - ";
							ofs <<now->tm_hour << ":";
						   	ofs <<now->tm_min << ":";
						   	ofs <<now->tm_sec << " ";
						   	ofs <<"SENDER : ";
						   	ofs <<"RECEIVED BROKEN ACKS ";
						   	ofs <<ackseg.nextsequencenumber-windowsize<<endl;
							printf("\n*RECEIVED BROKEN ACKS %d*\n",ackseg.nextsequencenumber-windowsize);
						}
						printACK(ackseg);
						fflush(stdout);
					}
				}
			}
			// Expand window
			if (countack == windowsize) {
				window_left += windowsize;
				window_right += windowsize;
				logtimenow=time(0);
				now = localtime(&logtimenow);
				ofs<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday << " - ";
				ofs <<now->tm_hour << ":";
			   	ofs <<now->tm_min << ":";
			   	ofs <<now->tm_sec << " ";
			   	ofs <<"SENDER : ";
			   	ofs <<"WINDOW EXPANDED ";
			   	ofs <<"WINDOW LEFT "<<window_left<<" ";
			   	ofs <<"WINDOW RIGHT "<<window_right<<endl;
				printf("\n*WINDOW EXPANDED*\n");
			}
		}
		bufflen += strlen(buff);
	}
	fclose(f);

	// Signal finish to server
	// Create segment from buffer
	seg.soh = '\00';
	seg.sequencenumber = 0;
	seg.stx = '\02';
	seg.data = '.';
	seg.etx = '\03';
	seg.checksum = 8;

	// Make string from segment
	segmentToString(&seg, segstr);

	// Send segment
	sendto(sock, segstr, 9, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
	ofs<<"FILE SUCCESSFULLY SEND"<<endl;
	ofs.close();
	close(sock);
	return 0;
}