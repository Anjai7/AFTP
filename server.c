// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "protocol.h"

#define SERVER_PORT 8080
#define MAX_CLIENTS 1
#define MAX_BUFFERED_CHUNKS 1024
#define RESEND_TIMEOUT 5

ChunkEntry chunk_buffer[MAX_BUFFERED_CHUNKS];

typedef struct {
    DataPacket chunk;
    time_t timestamp;
    int acknowledged;
} ChunkEntry;

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    char buffer[1024];
    char client_msg[1024];
    int last_acknowledged = -1;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);

        if (strncmp(buffer, "GET ", 4) == 0) {
            char filename[256] = {0};
            sscanf(buffer + 4, "%255s", filename);

            FILE *fp = fopen(filename, "rb");
            if (!fp) {
                perror("File open failed");
                continue;
            }

            struct stat st;
            stat(filename, &st);
            int filesize = st.st_size;
            int total_chunks = (filesize + CHUNK_SIZE - 1) / CHUNK_SIZE;

            MetaPacket meta = {0};
            strncpy(meta.filename, filename, sizeof(meta.filename));
            meta.filesize = filesize;
            meta.chunk_size = CHUNK_SIZE;
            meta.total_chunks = total_chunks;

            sendto(sockfd, &meta, sizeof(meta), 0, (struct sockaddr *)&client_addr, addr_len);

            fseek(fp, 0, SEEK_SET);
            for (int i = 0; i < total_chunks; i++) {
                DataPacket dp;
                dp.chunk_id = i;
                dp.chunk_size = fread(dp.data, 1, CHUNK_SIZE, fp);
                sendto(sockfd, &dp, sizeof(int) * 2 + dp.chunk_size, 0,
                       (struct sockaddr *)&client_addr, addr_len);
                chunk_buffer[i % MAX_BUFFERED_CHUNKS].chunk = dp;
                chunk_buffer[i % MAX_BUFFERED_CHUNKS].timestamp = time(NULL);
                chunk_buffer[i % MAX_BUFFERED_CHUNKS].acknowledged = 0;
            }

            fclose(fp);
        } else if (strncmp(buffer, "ACK", 3) == 0) {
            AckPacket ack;
            memcpy(&ack, buffer + 3, sizeof(ack));
            for (int i = ack.start_chunk_id; i < ack.start_chunk_id + ack.count; i++) {
                chunk_buffer[i % MAX_BUFFERED_CHUNKS].acknowledged = 1;
            }
        } else if (strncmp(buffer, "REQ", 3) == 0) {
            ResendRequest req;
            memcpy(&req, buffer + 3, sizeof(req));
            int id = req.chunk_id;
            if (!chunk_buffer[id % MAX_BUFFERED_CHUNKS].acknowledged) {
                DataPacket dp = chunk_buffer[id % MAX_BUFFERED_CHUNKS].chunk;
                sendto(sockfd, &dp, sizeof(int) * 2 + dp.chunk_size, 0,
                       (struct sockaddr *)&client_addr, addr_len);
                chunk_buffer[id % MAX_BUFFERED_CHUNKS].timestamp = time(NULL);
            }
        }

        // Timeout-based cleanup
        time_t now = time(NULL);
        for (int i = 0; i < MAX_BUFFERED_CHUNKS; i++) {
            if (!chunk_buffer[i].acknowledged &&
                now - chunk_buffer[i].timestamp > RESEND_TIMEOUT) {
                chunk_buffer[i].acknowledged = 1;  // Force deletion
                }
        }
    }

    close(sockfd);
    return 0;
}
