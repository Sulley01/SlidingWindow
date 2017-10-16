#ifndef SEGMENT_H
#define SEGMENT_H

typedef struct {
    char soh;
    int sequencenumber;
    char stx;
    char data;
    char etx;
    char checksum;
} segment;

typedef struct {
    char ack;
    int nextsequencenumber;
    char advertisedwindowsize;
    char checksum;
} acks;

#endif