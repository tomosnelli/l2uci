#include "l2uci.h"

#define INTERFACE "enp14s0"
#define ASCII_MAC_LEN 12
#define ASCII_MAC_LEN_C 17
#define HEX_MAC_LEN 6
#define OPT_LEN 8
#define IFNAME_LEN 12
#define DEST_MAC "\x74\x56\x3c\x66\x80\x50"
#define VAR_LEN 30000

void ascii_normalize_mac(char *mac) {
    char *write = mac;
    for (char *read = mac; *read; read++) {
        if (*read != ':') {
            *write = tolower(*read);
            write++;
        }
    }
    *write = '\0';
}

void ascii_mac_to_hex(const char *ascii_mac, uint8_t *hex_mac) {
    for (int i = 0; i < HEX_MAC_LEN; i++)
    {
        sscanf(ascii_mac + (i * 2), "%2hhx", &hex_mac[i]);
    }
}

opt_t ascii_opt_to_hex( const char* str )
{
    for (size_t i = 0; i < sizeof(opt_lookup) / sizeof(opt_lookup[0]); i++)
    {
        if (strcmp(str, opt_lookup[i].str) == 0)
        {
            return opt_lookup[i].opt;
        }
    }
    return INVALID;
}

int send_packet()
{
    int raw_socket;
    struct ifreq ifr;
    struct sockaddr_ll socket_address;
    unsigned char buffer[ETH_FRAME_LEN];
    struct ethhdr *eh = (struct ethhdr *)buffer;
    
    // Create raw socket
    raw_socket = socket(AF_PACKET, SOCK_RAW, htons(L2WRT_PROTOCOL));
    if (raw_socket == -1) {
        perror("Socket creation failed");
        return -1;
    }

    // Get interface index
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, INTERFACE, IFNAMSIZ-1);
    if (ioctl(raw_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        close(raw_socket);
        return -1;
    }

    // Prepare sockaddr_ll
    memset(&socket_address, 0, sizeof(struct sockaddr_ll));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_protocol = htons(L2WRT_PROTOCOL);
    socket_address.sll_ifindex = ifr.ifr_ifindex;
    socket_address.sll_halen = ETH_ALEN;
    memcpy(socket_address.sll_addr, DEST_MAC, ETH_ALEN);

    // Prepare Ethernet header
    memcpy(eh->h_dest, DEST_MAC, ETH_ALEN);
    if (ioctl(raw_socket, SIOCGIFHWADDR, &ifr) < 0) {
        perror("SIOCGIFHWADDR");
        close(raw_socket);
        return -1;
    }
    memcpy(eh->h_source, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    eh->h_proto = htons(ETH_P_IP);  // You can change this for different protocols

    // Prepare packet data (after Ethernet header)
    char *data = buffer + sizeof(struct ethhdr);
    strcpy(data, "Hello, Layer 2!");
    int data_len = strlen(data);

    // Send packet
    int send_result = sendto(raw_socket, buffer, data_len + sizeof(struct ethhdr), 0, 
                             (struct sockaddr*)&socket_address, sizeof(socket_address));
    if (send_result < 0) {
        perror("sendto failed");
        close(raw_socket);
        return -1;
    }

    printf("Packet sent successfully\n");
    close(raw_socket);
    return 0;
}

int main(int argc, char *argv[])
{
    int opt;
    uint8_t ascii_mac[ ASCII_MAC_LEN_C + 1 ]   = { 0 };
    uint8_t hex_mac[ HEX_MAC_LEN + 1 ] = { 0 };
    uint8_t l2opt    = 0x04;
    uint8_t* variable[ VAR_LEN ] = { 0 };

    while ((opt = getopt(argc, argv, "m:o:v:i:")) != -1)
    {
        switch (opt)
        {
            // mac
            case 'm':
                snprintf( ascii_mac, ASCII_MAC_LEN_C + 1, "%s", optarg );
                size_t mac_len = strlen( ascii_mac );
                if( mac_len != ASCII_MAC_LEN && mac_len != ASCII_MAC_LEN_C )
                {
                    printf("Error: MAC length isn't correct\n");
                    printf("mac_len: %u\n", mac_len);
                    return 1;
                }
                ascii_normalize_mac( ascii_mac );
                break;
            // opt code
            case 'o':
                l2opt = ascii_opt_to_hex( optarg );

                if( l2opt == INVALID )
                {
                    printf("Error: you didn't pass a valid opt code\n");
                    return 1;
                }

                break;
            // variable
            case 'v':
                printf("variable: %s\n", optarg);
                snprintf( variable, VAR_LEN, "%s", optarg );
                break;
            // interface
            case 'i':
                printf("interface: %s\n", optarg);
                break;
            default:
                printf("Usage: %s [-m mac] [-o operation] [-v variable] [-i interface]\n", argv[0]);
                return 1;
        }
    }

    return 0;
}