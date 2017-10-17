#ifndef SEGMENT_H
#define SEGMENT_H

typedef struct {
    char soh;
    int sequencenumber;
    char stx;
    char data;
    char etx;
    char checksum;
} __attribute__((packed)) segment;

typedef struct {
    char ack;
    int nextsequencenumber;
    char advertisedwindowsize;
    char checksum;
} __attribute__((packed)) acks;

#endif