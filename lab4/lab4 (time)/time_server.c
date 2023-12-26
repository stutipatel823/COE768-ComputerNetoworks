/* time_server.c - main */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char *argv[])
{
	int s; /* socket descriptor*/
	int port = 3000;
	struct sockaddr_in sin;	 /* an Internet endpoint address*/
	struct sockaddr_in fsin; /* the from address of a client	*/
	char buf[100];			 /* "input" buffer; any size > 0	*/
	socklen_t alen;			 /* from-address length*/

	switch (argc)
	{
	case 1:
		break;
	case 2:
		port = atoi(argv[1]);
		break;
	default:
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	/* Allocate a socket */
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		fprintf(stderr, "can't creat socket\n");

	/* Bind the socket */
	if (bind(s, (struct sockaddr *)&sin, (socklen_t)sizeof(sin)) < 0)
		fprintf(stderr, "can't bind to %d port\n", port);

	/*five clients in total*/
	listen(s, 5);

	alen = sizeof(fsin);
	while (1)
	{
		if (recvfrom(s, buf, sizeof(buf), 0,
					 (struct sockaddr *)&fsin, &alen) < 0)
			fprintf(stderr, "recvfrom error\n");
		// printf("Received message: %s\n", buf); // Print the received message

		time_t now; /* current time	*/
		char *pts;
		(void)time(&now);
		pts = ctime(&now);

		(void)sendto(s, pts, strlen(pts), 0,
					 (struct sockaddr *)&fsin, sizeof(fsin)); // void is not necessary
	}
}
