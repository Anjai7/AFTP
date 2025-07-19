// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include "protocol.h"

#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"
#define ACK_BATCH_SIZE 10
#define RESEND_TIMEOUT 1

int received_chunks[65536] = {0};
int last_sequential_chunk = -1;
time_t chunk_timestamps[65536] = {0};

void send_ack(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, int start, int count) {
    AckPacket ack = { .start_chunk_id = start, .count = count };
    char buffer[sizeof("ACK") + sizeof(ack)];
    memcpy(buffer, "ACK", 3);
    memcpy(buffer + 3, &ack, sizeof(ack));
    sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)server_addr, addr_len);
    printf("Sent ACK for chunks %d to %d\n", start, start + count - 1);
}

void send_resend_request(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, int chunk_id, int is_timeout) {
    ResendRequest req = { .chunk_id = chunk_id, .is_timeout = is_timeout };
    char buffer[sizeof("REQ") + sizeof(req)];
    memcpy(buffer, "REQ", 3);
    memcpy(buffer + 3, &req, sizeof(req));
    sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)server_addr, addr_len);
    printf("Sent REQ for chunk %d (%s)\n", chunk_id, is_timeout ? "timeout" : "gap");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename_to_get>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *requested_file = argv[1];

    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    char buffer[2048];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Send GET request
    char get_request[512] = {0};
    snprintf(get_request, sizeof(get_request), "GET %s", requested_file);
    sendto(sockfd, get_request, strlen(get_request), 0, (struct sockaddr *)&server_addr, addr_len);

    // Receive meta
    MetaPacket meta;
    recvfrom(sockfd, &meta, sizeof(meta), 0, (struct sockaddr *)&server_addr, &addr_len);
    printf("Receiving file: %s (%d bytes, %d chunks)\n", meta.filename, meta.filesize, meta.total_chunks);

    FILE *fp = fopen(meta.filename, "wb");
    if (!fp) {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }
    ftruncate(fileno(fp), meta.filesize);

    int ack_counter = 0;

    while (1) {
        ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr *)&server_addr, &addr_len);
        if (n > 0) {
            DataPacket dp;
            memcpy(&dp, buffer, sizeof(int) * 2);
            memcpy(dp.data, buffer + sizeof(int) * 2, dp.chunk_size);

            if (!received_chunks[dp.chunk_id]) {
                fseek(fp, dp.chunk_id * CHUNK_SIZE, SEEK_SET);
                fwrite(dp.data, 1, dp.chunk_size, fp);
                received_chunks[dp.chunk_id] = 1;
                chunk_timestamps[dp.chunk_id] = time(NULL);
                printf("Received chunk %d (%d bytes)\n", dp.chunk_id, dp.chunk_size);
                ack_counter++;
            }

            // Triggered resend
            if (dp.chunk_id > 0 && !received_chunks[dp.chunk_id - 1]) {
                send_resend_request(sockfd, &server_addr, addr_len, dp.chunk_id - 1, 0);
            }

            // ACK every 10 chunks
            if (ack_counter >= ACK_BATCH_SIZE) {
                int start = last_sequential_chunk + 1;
                int count = 0;
                for (int i = start; i < meta.total_chunks && received_chunks[i]; i++) {
                    count++;
                }
                if (count > 0) {
                    send_ack(sockfd, &server_addr, addr_len, start, count);
                    last_sequential_chunk = start + count - 1;
                    ack_counter = 0;
                }
            }
        }

        // Timeout-based resend
        time_t now = time(NULL);
        for (int i = last_sequential_chunk + 1; i < meta.total_chunks; i++) {
            if (!received_chunks[i] && (now - chunk_timestamps[i]) >= RESEND_TIMEOUT) {
                send_resend_request(sockfd, &server_addr, addr_len, i, 1);
                chunk_timestamps[i] = now + 9999; // Prevent spamming
            }
        }

        // Exit condition
        int complete = 1;
        for (int i = 0; i < meta.total_chunks; i++) {
            if (!received_chunks[i]) {
                complete = 0;
                break;
            }
        }
        if (complete) {
            printf("File received completely.\n");
            break;
        }
        usleep(10000); // 10ms sleep
    }

    fclose(fp);
    close(sockfd);
    return 0;
}
