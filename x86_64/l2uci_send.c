#include "l2uci.h"

void print_hex(uint8_t* hex, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", hex[i]);

        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }

    if (len % 16 != 0) {
        printf("\n");
    }
}


void ascii_normalize_mac( char *mac )
{
    char *write = mac;
    for( char *read = mac; *read; read++ )
    {
        if ( *read != ':' )
        {
            *write = tolower( *read );
            write++;
        }
    }
    *write = '\0';
}

void ascii_mac_to_hex( const char *ascii_mac, uint8_t *hex_mac )
{
    for( int i = 0; i < HEX_MAC_LEN; i++ )
    {
        sscanf( ascii_mac + (i * 2), "%2hhx", &hex_mac[ i ] );
    }
}

opt_t ascii_opt_to_hex( const char* str )
{
    for( size_t i = 0; i < sizeof( opt_lookup ) / sizeof( opt_lookup[ 0 ] ); i++ )
    {
        if( strcmp( str, opt_lookup[ i ].str ) == 0 )
        {
            return opt_lookup[ i ].opt;
        }
    }
    return INVALID;
}

/*
General GET/SET/SHOW packet
 SEQ  OPT  VARLEN  VAR 
[   ][   ][      ][   ]
*/
size_t construct_packet(uint8_t* packet, uint8_t opt_code, const char* var) {
    uint8_t* ptr = packet;
    uint32_t net_byte_var_len = htonl(strlen(var) + 1);
    size_t runtime_packet_len = 0;

    // Calculate the runtime packet length based on the option code
    switch (opt_code) {
        case DISCOVER:
            runtime_packet_len = sizeof(uint8_t) * 2; // SEQ + opt
            break;
        case SHOW:
        case GET:
            runtime_packet_len = sizeof(uint8_t) * 2 +     // SEQ + opt
                                 sizeof(uint32_t) +         // Variable length (4 bytes)
                                 strlen(var) + 1;           // Variable string + null terminator
            break;
        case SET:
            runtime_packet_len = sizeof(uint8_t) * 2 +     // SEQ + opt
                                 sizeof(uint32_t) +         // Variable length (4 bytes)
                                 strlen(var) + 1;          // Variable string + null terminator
            break;
        case COMMIT:
            runtime_packet_len = sizeof(uint8_t) * 2;     // SEQ + ID
            break;
        default:
            printf("Invalid option code\n");
            return 0; // Invalid code, return zero length
    }

    if (runtime_packet_len <= 0 || runtime_packet_len >= PACKET_LEN) {
        printf("Error while constructing packet\n");
        return runtime_packet_len;
    }

    *(ptr++) = 0;               // Set sequence number (currently hardcoded to 0)
    *(ptr++) = opt_code;       // Set option code

    // Common packet construction for SHOW, GET, and SET
    if (opt_code == SHOW || opt_code == GET || opt_code == SET) {
        *(ptr++) = (net_byte_var_len >> 0) & 0xff;   // Copy variable length in network byte order
        *(ptr++) = (net_byte_var_len >> 8) & 0xff;
        *(ptr++) = (net_byte_var_len >> 16) & 0xff;
        *(ptr++) = (net_byte_var_len >> 24) & 0xff;

        memcpy(ptr, var, strlen(var) + 1);   // Copy variable string with null terminator
        ptr += strlen(var) + 1;
    }

    print_hex(packet, ptr - packet);   // Print the constructed packet in hex format

    return ptr - packet;                // Return the total packet length constructed
}


int send_packet( uint8_t* hex_mac, char* ifname, uint8_t* packet, size_t packet_len )
{
    int    raw_socket;
    struct ifreq ifr;
    struct sockaddr_ll socket_address;
    unsigned char buffer[ETH_FRAME_LEN];
    struct ethhdr *eh = (struct ethhdr *)buffer;
    
    // Create raw socket
    raw_socket = socket(AF_PACKET, SOCK_RAW, htons(L2UCI_PROTOCOL));
    if (raw_socket == -1) {
        perror("Socket creation failed");
        return -1;
    }

    // Get interface index
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);
    if (ioctl(raw_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        close(raw_socket);
        return -1;
    }

    // Prepare sockaddr_ll
    memset(&socket_address, 0, sizeof(struct sockaddr_ll));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_protocol = htons(L2UCI_PROTOCOL);
    socket_address.sll_ifindex = ifr.ifr_ifindex;
    socket_address.sll_halen = ETH_ALEN;
    memcpy(socket_address.sll_addr, hex_mac, ETH_ALEN);

    // Prepare Ethernet header
    memcpy(eh->h_dest, hex_mac, ETH_ALEN);
    if (ioctl(raw_socket, SIOCGIFHWADDR, &ifr) < 0) {
        perror("SIOCGIFHWADDR");
        close(raw_socket);
        return -1;
    }
    memcpy(eh->h_source, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    eh->h_proto = htons(L2UCI_PROTOCOL);  // You can change this for different protocols

    // Prepare packet data (after Ethernet header)
    char *data = buffer + sizeof(struct ethhdr);
    memcpy( data, packet, packet_len );
    int data_len = packet_len;

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

int main( int argc, char *argv[] )
{
    uint8_t packet[ PACKET_LEN ] = { 0 };
    int opt;
    uint8_t  l2opt                            =  0x04;
    uint8_t  ascii_mac[ ASCII_MAC_LEN_C + 1 ] = { 0 };
    uint8_t  hex_mac[ HEX_MAC_LEN ]           = { 0 };
    uint8_t  variable[ VAR_LEN ]              = { 0 };
    uint8_t ifname[ IFNAME_LEN + 1 ]          = { 0 };
    uint8_t iflen = 0;

    while( ( opt = getopt( argc, argv, "m:o:v:i:" ) ) != -1 )
    {
        switch (opt)
        {
            case 'm':
                snprintf( ascii_mac, ASCII_MAC_LEN_C + 1, "%s", optarg );
                size_t mac_len = strlen( ascii_mac );
                if( mac_len != ASCII_MAC_LEN && mac_len != ASCII_MAC_LEN_C )
                {
                    printf( "Error: MAC length isn't correct\n" );
                    printf( "mac_len: %u\n", mac_len );
                    return 1;
                }
                ascii_normalize_mac( ascii_mac );
                ascii_mac_to_hex( ascii_mac, hex_mac );
                break;

            case 'o':
                l2opt = ascii_opt_to_hex( optarg );

                if( l2opt == INVALID )
                {
                    printf("Error: you didn't pass a valid opt code\n");
                    return 1;
                }
                break;

            case 'v':
                memcpy( variable, optarg, strlen( optarg ) );
                printf( "variable: %s\n", variable );
                break;

            case 'i':
                if( strlen( optarg ) > IFNAME_LEN )
                {
                    printf("Error: interface name is too long");
                    return 1;
                }

                memcpy( ifname, optarg, strlen( optarg ) + 1 );
                iflen = strlen( ifname );
                break;

            default:
                printf("Usage: %s [-m mac] [-o operation] [-v variable] [-i interface]\n", argv[0]);
                return 1;
        }
    }

    size_t packet_size = construct_packet( packet, l2opt, variable );

    if(packet_size == 0){
        printf("Error: issue while constructing packet\n");
        return 1;
    }

    int status = send_packet( hex_mac, ifname, packet, packet_size );

    if(status < 0){
        printf("Error: issue sending packet\n");
        return 1;
    }

    return 0;
}