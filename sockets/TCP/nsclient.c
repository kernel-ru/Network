#include <error.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#define fatal(msg) {perror(msg);}

int nsclient(char *host)
{
	int sock;
	struct sockaddr_in servadd;
	struct hostent *hp;
	char nodename[200], servname[200];

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		fatal("socket");

	bzero(&servadd, sizeof(servadd));

	if ((hp = gethostbyname(host)) == NULL)
		fatal("gethostbyname");
   
	servadd.sin_port = htons(12489);
	servadd.sin_family = AF_INET;
	bcopy(hp->h_addr, (struct sockaddr *)&servadd.sin_addr, hp->h_length);

	int status;
	if ((status = connect(sock, (struct sockaddr *)&servadd, sizeof(servadd))) != 0)
		if(errno != ECONNREFUSED)
			fatal("connect");
   
	getnameinfo((struct sockaddr *)&servadd, sizeof(servadd),
		nodename, sizeof(nodename), servname, sizeof(servname), 0);

	close(sock);
	return status;
}

void usage()
{
	printf("USAGE:\t scan <IP_address>\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	if(argv[1] == NULL)
        	usage();

	if (nsclient(argv[1]) == 0)
		printf("nsclient: running\n");
	else
		printf("nsclient: no_answer\n");

	return 0;
}


