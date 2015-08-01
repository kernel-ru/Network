#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <errno.h>

#include <sys/select.h>

#include <main.h>

#define BUF_SIZE 2000

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
			nr = read(tun_fd, buffer, BUF_SIZE);
			if (nr == -1)
				goto ERR;
#ifdef DEBUG
			fprintf(stderr, "read from tun %zu bytes\n", nr);
#endif
			len = htonl(nr);
			if (write_all(net_fd, &len, sizeof(len), &nw) == ERROR)
				if (nw == 0)
					goto ERR;
#ifdef DEBUG
			fprintf(stderr, "write to network %zu bytes\n", nw);
#endif
			if (write_all(net_fd, buffer, nr, &nw) == ERROR)
				if (nw == 0)
					goto ERR;
#ifdef DEBUG
			fprintf(stderr, "write to network remain %zu bytes\n======\n", nw);
#endif
		}
		if (FD_ISSET(net_fd, &rfds) == true) {
			if (read_all(net_fd, &len, sizeof(len), &nr) == ERROR)
				if (nr == 0)
					goto ERR;
#ifdef DEBUG
			fprintf(stderr, "read from network %zu bytes\n", nr);
#endif
			len = ntohl(len);
			if (read_all(net_fd, buffer, len, &nr) == ERROR)
				if (nr == 0)
					goto ERR;
#ifdef DEBUG
			fprintf(stderr, "read from network remain %zu bytes\n", nr);
#endif
			if (write_all(tun_fd, buffer, nr, &nw) == ERROR)
				if (nw == 0)
					goto ERR;
#ifdef DEBUG
			fprintf(stderr, "write to tun %zu bytes\n======\n", nw);
#endif
		}
	} /*for (;;)*/

	free(buffer);
	return SUCCESS;
ERR:
	free(buffer);
	return ERROR;
}

RC_t write_all(int fd, void *buf, ssize_t n, ssize_t *tot_write)
{
	ssize_t tot, c;
	for (tot = 0; tot < n;) {
		c = write(fd, buf + tot, n - tot);
		tot += c;
		if ((c == -1 || c == 0) && tot != n) {
			perror("write write_all");
			*tot_write = tot;
			return ERROR;
		}
	}
	*tot_write = tot;
	return SUCCESS;
}
RC_t read_all(int fd, void *buf, ssize_t n, ssize_t *tot_read)
{
	ssize_t tot, c;
	for (tot = 0; tot < n;) {
		c = read(fd, buf + tot, n - tot);
		tot += c;
		if ((c == -1 || c == 0) && tot != n) {
			perror("read read_all");
			*tot_read = tot;
			return ERROR;
		}
	}
	*tot_read = tot;
	return SUCCESS;
}
