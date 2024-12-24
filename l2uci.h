#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>

#define INTERFACE "enp14s0"
#define ASCII_MAC_LEN 12
#define ASCII_MAC_LEN_C 17
#define HEX_MAC_LEN 6
#define OPT_LEN 8
#define IFNAME_LEN 255
#define DEST_MAC "\x74\x56\x3c\x66\x80\x50"
#define VAR_LEN 500
#define PACKET_LEN 30000

#define L2WRT_PROTOCOL 0x326c

typedef enum l2uci_opt
{
    DISCOVER = 0x00,
    SHOW     = 0x01,
    GET      = 0x02,
    SET      = 0x03,
    COMMIT   = 0x04,
    INVALID  = 0xff
} opt_t;

static const struct {
    const char *str;
    opt_t opt;
} opt_lookup[] = {
    {"DISCOVER", DISCOVER},
    {"SHOW", SHOW},
    {"GET", GET},
    {"SET", SET},
    {"COMMIT", COMMIT},
    {"INVALID", INVALID},
};
