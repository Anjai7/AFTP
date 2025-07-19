#ifndef PROTOCOL_H
#define PROTOCOL_H

#define CHUNK_SIZE 1024
#define MAX_RESEND_IDS 1     // 1 chunk per resend request

// Meta info for a file
typedef struct {
    char filename[256];
    int filesize;
    int chunk_size;
    int total_chunks;
} MetaPacket;

// File data chunk
typedef struct {
    int chunk_id;
    int chunk_size;
    char data[CHUNK_SIZE];
} DataPacket;

// Ack for a group of chunks
typedef struct {
    int start_chunk_id;   // e.g., 30
    int count;            // e.g., 10  → chunks 30–39
} AckPacket;

// Resend request for a missing chunk
typedef struct {
    int chunk_id;
    int is_timeout;       // 0 = triggered, 1 = timeout
} ResendRequest;

// Resume request after failure
typedef struct {
    int last_received_chunk_id;
} ResumeRequest;

#endif
