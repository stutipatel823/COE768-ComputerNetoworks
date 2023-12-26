#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include <arpa/inet.h> // Add this header for inet_addr (ip=127.0.0.1)

#define SERVER_TCP_PORT 3000 /* well-known port */
#define BUFLEN 257			 /* buffer length + 1 for ERROR_CHAR */
#define PACKET_SIZE 100

int echod(int);
void reaper(int);

int main(int argc, char **argv)
{
	int sd, new_sd, client_len, port;
	struct sockaddr_in server, client;

	switch (argc)
	{
	case 1:
		port = SERVER_TCP_PORT;
		break;
	case 2:
		port = atoi(argv[1]);
		break;
	default:
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr, "Can't create a socket\n");
		exit(1);
	}

	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr("127.0.0.1"); // REMOVE LATER

	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	listen(sd, 5);
	(void)signal(SIGCHLD, reaper);

	while (1)
	{
		client_len = sizeof(client);
		new_sd = accept(sd, (struct sockaddr *)&client, (socklen_t *)&client_len);
		if (new_sd < 0)
		{
			fprintf(stderr, "Can't accept client \n");
			exit(1);
		}
		switch (fork())
		{
		case 0: /* child */
			close(sd);
			exit(echod(new_sd));
		default: /* parent */
			close(new_sd);
			break;
		case -1:
			fprintf(stderr, "fork: error\n");
		}
	}
}

int echod(int sd)
{
	char buf[BUFLEN]; // strings from the file
	int n;			  // value returned by the read

	if ((n = read(sd, buf, BUFLEN - 1)) <= 0) // reads the file name from the connected client's terminal
	{
		printf("Client requested file: %s\n", buf);
		return 0;
	}

	// buf[n] = '\0'; // Null terminate the filename string

	FILE *file = fopen(buf, "r"); // r=read, b=binary -> read in binary mode
	if (!file)					  // if file is not found there should be an error printed on the client terminal
	{
		write(sd, "ERROR: File not found.", strlen("ERROR: File not found."));
		printf("ERROR: File not found!\n");
		return 0;
	}

	char file_buf[PACKET_SIZE];											  // Adjusted size to PACKET_SIZE
	int bytes_read;														  // stores bytes read from the file
	while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), file)) > 0) // read data into file_buf, each data item should be read in 1 byte
	{
		write(sd, file_buf, bytes_read); // if that is greater than 0, contents should be written into file_buf
	}
	fclose(file);

	return (0);
}

void reaper(int sig)
{
	int status;
	while (wait3(&status, WNOHANG, (struct rusage *)0) >= 0)
		;
}
