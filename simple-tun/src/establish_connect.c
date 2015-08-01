#include <stdio.h>

#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <main.h>

RC_t cli_tun(CMD_t *args, NetFD_t *fds);
RC_t serv_tun(CMD_t *args, NetFD_t *fds);

RC_t establish_connect(CMD_t *args, NetFD_t *fds)
{
	if (args->role == SERVER)
		return serv_tun(args, fds);
	else
		return cli_tun(args, fds);
}

RC_t cli_tun(CMD_t *args, NetFD_t *fds)
{
	struct sockaddr_in remote;
	int sock_fd;
	if ( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket client");
		return ERROR;
	}
	remote.sin_family = AF_INET;
	remote.sin_addr = args->ip_addr.remote_ip;
	remote.sin_port = htons(args->port);
	if (connect(sock_fd, (struct sockaddr *)&remote, sizeof(struct sockaddr_in)) == -1) {
		perror("connect client");
		return ERROR;
	}
#ifdef DEBUG
	fprintf(stderr, "connect to server ip: %s\n", inet_ntoa(remote.sin_addr));
#endif
	fds->net_fd = sock_fd;
	return SUCCESS;
}

RC_t serv_tun(CMD_t *args, NetFD_t *fds)
{
	struct sockaddr_in local, guest;
	int sock_fd;
	socklen_t socklen;
	if ( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket server");
		return ERROR;
	}
	memset(&local, 0, sizeof(struct sockaddr_in));
	local.sin_family = AF_INET;
	local.sin_addr = args->ip_addr.local_ip;
	local.sin_port = htons(args->port);
	if (bind(sock_fd, (struct sockaddr *)&local, sizeof(struct sockaddr_in)) == -1) {
		perror("bind server");
		return ERROR;
	}
	if (listen(sock_fd, 0) == -1) {
		perror("listen server");
		return ERROR;
	}
	socklen = sizeof(struct sockaddr_in);
	if ( (fds->net_fd = accept(sock_fd, (struct sockaddr *)&guest, &socklen)) == -1) {
		perror("accept server");
		return ERROR;
	}

	return SUCCESS;
}
