#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <errno.h>

#include <sys/select.h>

#include <main.h>

#define BUF_SIZE 1500

RC_t read_all(int fd, void *buf, ssize_t n, ssize_t *tot_read);
RC_t write_all(int fd, void *buf, ssize_t n, ssize_t *tot_read);

RC_t do_communication(NetFD_t *fds)
{
	int net_fd = fds->net_fd, tun_fd = fds->tun_fd, ret;
	int maxfd = net_fd > tun_fd ? net_fd : tun_fd;
	ssize_t nr, nw, len;
	fd_set rfds;
	char *buffer = malloc(BUF_SIZE);
	if (buffer == NULL) {
		perror("buffer");
		return ERROR;
	}

	for (;;) {
		FD_ZERO(&rfds);
		FD_SET(net_fd, &rfds);
		FD_SET(tun_fd, &rfds);
		ret = select(maxfd + 1, &rfds, NULL, NULL, NULL);
		if (ret == -1 && errno == EINTR) {
			continue;
		}
		if (ret == -1) {
			perror("select");
			return ERROR;
		}

		if (FD_ISSET(tun_fd, &rfds) == true) {
			if (read_all(tun_fd, buffer, BUF_SIZE, &nr) == ERROR)
				if (nr == 0)
					goto ERR;
			len = htonl(nr);
			if (write_all(net_fd, &len, sizeof(len), &nw) == ERROR)
				if (nw == 0)
					goto ERR;
			if (write_all(net_fd, buffer, nr, &nw) == ERROR)
				if (nw == 0)
					goto ERR;
		}
		if (FD_ISSET(net_fd, &rfds) == true) {
			/*can be read from the network 0.0.0.0:12345*/
			if (read_all(net_fd, &len, sizeof(len), &nr) == ERROR)
				if (nr == 0)
					goto ERR;
			len = ntohl(len);
			if (read_all(net_fd, buffer, len, &nr) == ERROR)
				if (nr == 0)
					goto ERR;

			if (write_all(tun_fd, buffer, nr, &nw) == ERROR)
				if (nw == 0)
					goto ERR;
		}
	} /*for (;;)*/

	return SUCCESS;
ERR:
	free(buffer);
	return ERROR;
}

RC_t write_all(int fd, void *buf, ssize_t n, ssize_t *tot_write)
{
	ssize_t tot, c;
	for (tot = 0; tot < n; tot += c) {
		c = write(fd, buf, n - tot);
		if ((c == -1 || c == 0) && tot != n) {
			perror("write write_all");
			*tot_write = tot;
			return ERROR;
		}
	}
	return SUCCESS;
}
RC_t read_all(int fd, void *buf, ssize_t n, ssize_t *tot_read)
{
	ssize_t tot, c;
	for (tot = 0; tot < n; tot += c) {
		c = read(fd, buf, n - tot);
		if ((c == -1 || c == 0) && tot != n) {
			perror("read read_all");
			*tot_read = tot;
			return ERROR;
		}
	}
	return SUCCESS;
}
