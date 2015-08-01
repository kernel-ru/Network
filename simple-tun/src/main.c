#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include <getopt.h>

#include <limits.h>

#include <arpa/inet.h>

#include <string.h>

#include <errno.h>

#include <main.h>

RC_t parsing_cmd(int argc, char **argv, CMD_t **args);
void free_stuff(CMD_t **args, NetFD_t **fds);
void usage(FILE *stream);

int main(int argc, char **argv)
{
	CMD_t *args = NULL;
	NetFD_t *fds = NULL;
#ifdef DEBUG
	fprintf(stderr, PROGRAM_NAME "\n");
	fprintf(stderr, "start...\n");
	fprintf(stderr, "parsing command line...\n");
#endif
	RC_t ret = parsing_cmd(argc, argv, &args);
	if (ret < SUCCESS)
		exit(EXIT_FAILURE);
	else if (ret == HELP_RET)
		exit(EXIT_SUCCESS);

#ifdef DEBUG
	fprintf(stderr, "take_tun_fd...\n");
#endif
	if (take_tun_fd(args, &fds) < SUCCESS) {
		free_stuff(&args, NULL);
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	fprintf(stderr, "establish_connect...\n");
#endif
	if (establish_connect(args, fds) < SUCCESS) {
		free_stuff(&args, &fds);
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	fprintf(stderr, "do_communication...\n");
	fprintf(stderr, "Now you can setup ip address for tun interface and tune routing tables\n");
#endif
	if (do_communication(fds) < SUCCESS) {
		free_stuff(&args, &fds);
		exit(EXIT_FAILURE);
	}

	close(fds->tun_fd);
	free_stuff(&args, &fds);
	fprintf(stderr, "good luck\n");
	exit(EXIT_SUCCESS);
}

RC_t parsing_cmd(int argc, char **argv, CMD_t **args)
{
	bool client_fl = false, server_fl = false, port_fl = false, tun_name_fl = false;
	int c;
	long tmp_port;
	char *endptr = NULL;
	CMD_t *tmp_args = NULL;
	static struct option long_opts[] = {
		{"server",	optional_argument, NULL, 's'},
		{"client",	required_argument, NULL, 'c'},
		{"tun-name",	required_argument, NULL, 'n'},
		{"port",	required_argument, NULL, 'p'},
		{"help",	no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};
	tmp_args = calloc(1, sizeof(CMD_t));
	if (tmp_args == NULL) {
		perror("calloc tmp_args");
		return ERROR;
	}
	for(;;) {
		c = getopt_long(argc, argv, "s::c:n:p:h", long_opts, NULL);
		switch (c) {
			case -1:
				goto END_OPTS;
				break;
			case 'h':
				usage(stdout), free_stuff(&tmp_args, NULL);
				return HELP_RET;
				break;
			case 'c':
				if (server_fl == true) {
					fprintf(stderr, "can't both options \"server\" and \"client\"\n");
					usage(stderr), free_stuff(&tmp_args, NULL);
					return PARSING_ERR;
				}
				if ( (inet_aton(optarg, &tmp_args->ip_addr.local_ip)) == 0) {
					fprintf(stderr, "client: invalid ip address: \"%s\"\n", optarg);
					free_stuff(&tmp_args, NULL);
					return PARSING_ERR;
				}
				tmp_args->role = CLIENT;
				client_fl = true;
				break;
			case 's':
				if (client_fl == true) {
					fprintf(stderr, "can't both options \"server\" and \"client\"\n");
					usage(stderr), free_stuff(&tmp_args, NULL);
					return PARSING_ERR;
				}
				if (optarg != NULL) {
					if ( (inet_aton(optarg, &tmp_args->ip_addr.local_ip)) == 0) {
						fprintf(stderr, "server: invalid ip address: \"%s\"\n", optarg);
						free_stuff(&tmp_args, NULL);
						return PARSING_ERR;
					}
				} else {
					/*set local_ip as 0x0, INADDR_ANY define
					 * as 0x00000000
					 */
					memset(&tmp_args->ip_addr.local_ip, 0, sizeof(struct in_addr));
				}
				tmp_args->role = SERVER;
				server_fl = true;
				break;
			case 'n':
				if (strlen(optarg) > IFNAMSIZ) {
					fprintf(stderr, "dev name too large\n");
					usage(stderr), free_stuff(&tmp_args, NULL);
					return PARSING_ERR;
				}
				strcpy(tmp_args->tun_name, optarg);
				tun_name_fl = true;
				break;
			case 'p':
				errno = 0;
				tmp_port = strtol(optarg, &endptr, 10);
				if ((errno == ERANGE && (tmp_port == LONG_MAX || tmp_port == LONG_MIN)) ||
						(errno != 0 && tmp_port == 0)) {
					perror("strtol port");
					usage(stderr), free_stuff(&tmp_args, NULL);
					return PARSING_ERR;
				}
				if (endptr == optarg) {
					fprintf(stderr, "No digit in port number input: \"%s\"\n", optarg);
					usage(stderr), free_stuff(&tmp_args, NULL);
					return PARSING_ERR;
				}
				if (*endptr != '\0') {
					fprintf(stderr, "Invalid value in string: \"%s\" with value: \"%s\"\n",
							optarg, endptr);
					usage(stderr), free_stuff(&tmp_args, NULL);
					return PARSING_ERR;
				}
				if (tmp_port > USHRT_MAX || tmp_port <= MAX_PRIVELEGED_PORT) {
					fprintf(stderr, "port can't be more %d and less %d\n",
							USHRT_MAX, MAX_PRIVELEGED_PORT);
					usage(stderr), free_stuff(&tmp_args, NULL);
					return PARSING_ERR;
				}
				tmp_args->port = (unsigned short)tmp_port;
				port_fl = true;
				break;
			case '?':
				fprintf(stderr, "bad option\n");
				usage(stderr), free_stuff(&tmp_args, NULL);
				return PARSING_ERR;
				break;
			case ':':
				fprintf(stderr, "missing argument\n");
				usage(stderr), free_stuff(&tmp_args, NULL);
				return PARSING_ERR;
			default:
				fprintf(stderr, "getopt bad code \"0X%x\"\n", c);
				usage(stderr), free_stuff(&tmp_args, NULL);
				return PARSING_ERR;
				break;
		}
	}
END_OPTS:
	if (server_fl == false && client_fl == false) {
		fprintf(stderr, "there should be one option \"client\" or \"server\"\n");
		usage(stderr), free_stuff(&tmp_args, NULL);
		return PARSING_ERR;
	}
	if (port_fl == false) {
		tmp_args->port = DEFAULT_PORT_NUM;
	}
	if (tun_name_fl == false) {
		strcpy(tmp_args->tun_name, DEFAULT_TUN_NAME);
	}

#ifdef DEBUG
	if (tmp_args->role == SERVER) {
		fprintf(stderr, "role is \"server\"\n");
		fprintf(stderr, "listen ip address: %s\n", inet_ntoa(tmp_args->ip_addr.local_ip));
	} else {
		fprintf(stderr, "role is \"client\"\n");
		fprintf(stderr, "connect to ip address: \"%s\"\n", inet_ntoa(tmp_args->ip_addr.remote_ip));
	}
	fprintf(stderr, "tun name is: %s\n", tmp_args->tun_name);
	fprintf(stderr, "port number is: %d\n", tmp_args->port);
#endif
	*args = tmp_args;
	return SUCCESS;
}

void free_stuff(CMD_t **args, NetFD_t **fds)
{
	if (args) {
		if (*args) {
			free(*args);
			*args = NULL;
		}
	}
	if (fds) {
		if (*fds) {
			free(*fds);
			*fds = NULL;
		}
	}
}

void usage(FILE *stream)
{
	fprintf(stream, PROGRAM_NAME ": --server || --client [--tun-num=\"tun-name\"] [--port] [--help]\n");
}
