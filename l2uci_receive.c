#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1518

int main() {
    int sock_r;
    struct sockaddr saddr;
    int saddr_len = sizeof(saddr);
    unsigned char *buffer = (unsigned char *)malloc(BUFFER_SIZE);

    // Create a raw socket
    sock_r = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_r < 0) {
        perror("Socket creation error");
        return -1;
    }

    while (1) {
        // Receive a packet
        int buflen = recvfrom(sock_r, buffer, BUFFER_SIZE, 0, &saddr, (socklen_t *)&saddr_len);
        if (buflen < 0) {
            perror("Packet reception error");
            close(sock_r);
            return -1;
        }

        // Process the Ethernet header
        struct ethhdr *eth = (struct ethhdr *)(buffer);
        printf("Ethernet Header:\n");
        printf("Source MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
               eth->h_source[0], eth->h_source[1], eth->h_source[2],
               eth->h_source[3], eth->h_source[4], eth->h_source[5]);
        printf("Destination MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
               eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
               eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
        printf("Protocol: %04X\n", ntohs(eth->h_proto));

        printf("Payload (%d bytes):\n", buflen - sizeof(struct ethhdr));
        for (int i = sizeof(struct ethhdr); i < buflen; i++) {
            printf("%02X ", buffer[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\n\n");
    }

    close(sock_r);
    free(buffer);
    return 0;
}
