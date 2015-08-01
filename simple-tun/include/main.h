#ifndef _MAIN_H
#define _MAIN_H

#include <net/if.h>
#include <netdb.h>

#define PROGRAM_NAME "icmp-tunnel"
#define DEFAULT_PORT_NUM 12345
#define DEFAULT_TUN_NAME "icmp-tun"
#define MAX_PRIVELEGED_PORT 1024

typedef enum {
	PARSING_ERR	= -2,
	ERROR		= -1,
	SUCCESS		= 0,
	HELP_RET	= 1,
} RC_t;

typedef enum {
	SERVER	= 0,
	CLIENT	= 1,
} Role_t;

typedef struct {
	Role_t role;
	unsigned short port;
	char tun_name[IFNAMSIZ + 1];
	union {
		struct in_addr remote_ip;
		struct in_addr local_ip;
	} ip_addr;
} CMD_t;

typedef struct {
	int tun_fd;
	int net_fd;
} NetFD_t;

RC_t do_communication(NetFD_t *fds);

RC_t establish_connect(CMD_t *args, NetFD_t *fds);

RC_t take_tun_fd(CMD_t *args, NetFD_t **fds);

#endif /*_MAIN_H*/
