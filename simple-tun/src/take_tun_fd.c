#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/ioctl.h>

#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_tun.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <main.h>

RC_t take_tun_fd(CMD_t *args, NetFD_t **fds)
{
	int fd, err;
	struct ifreq ifr;
	if ( (*fds = calloc(1, sizeof(NetFD_t))) == NULL) {
		perror("calloc fds");
		return ERROR;
	}
	if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
		free(*fds);
		perror("open /dev/net/tun");
		return ERROR;
	}
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN;
	strcpy(ifr.ifr_name, args->tun_name);
	if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
		free(*fds), close(fd);
		perror("ioctl TUNSETIFF");
		return ERROR;
	}
#ifdef DEBUG
	fprintf(stderr, "tun name is: \"%s\"\n", ifr.ifr_name);
#endif
	(*fds)->tun_fd = fd;
	return SUCCESS;
}
