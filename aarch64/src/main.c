#include "l2uci.h"

#define BUFFER_SIZE 9000

int l2uci_packet_handler(unsigned char* payload, size_t size)
{
    uint8_t seq_num = payload[ SEQ_NUM_OFFSET ];
    uint8_t opt     = payload[ SEQ_NUM_OFFSET + 1 ];

    struct uci_context *ctx = uci_alloc_context();
    struct uci_ptr ptr = NULL;

    if (uci_lookup_ptr(ctx, &ptr, payload + (SEQ_NUM_OFFSET + 6), true) != UCI_OK)
    {
        printf("parameter doesn't exist\n");
        return -1;
    }

    switch(opt)
    {
        case DISCOVER:
            break;
        case SHOW:
            struct uci_element *e;
            uci_foreach_element(&ptr.p->sections, e) {
                struct uci_section *s = uci_to_section(e);
                struct uci_element *o;
                uci_foreach_element(&s->options, o) {
                    struct uci_option *opt = uci_to_option(o);
                    printf("%s.%s.%s=%s\n", ptr.p->e.name, s->e.name, o->name, opt->v.string);
                }
            }
            break;
        case GET:
            break;
        case SET:
            break;
        case COMMIT:
            break;
        default:
            printf("Error: Invalid packet opt code\n");
            break;
    }

    return 1;
}

int l2uci_listener()
{
    int sock_r;
    struct sockaddr saddr;
    int saddr_len = sizeof(saddr);
    unsigned char  buffer[ BUFFER_SIZE ] = { 0 };

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

        struct ethhdr *eth = (struct ethhdr *)(buffer);

        // Process the Ethernet header
        if (ntohs(eth->h_proto) == L2UCI_PROTOCOL)
        {
            printf("Received L2UCI protocol packet\n");

            // struct ethhdr *eth = (struct ethhdr *)(buffer);
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

            l2uci_packet_handler(buffer + sizeof(struct ethhdr), buflen - sizeof(struct ethhdr));

            printf("\n\n");
            break;
        }
    }
    close(sock_r);
    return 0;
}

int main() {
    l2uci_listener();
    return 0;
}
