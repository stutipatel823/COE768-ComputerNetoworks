#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h> // Add this header for inet_addr (ip=127.0.0.1)

#define SERVER_TCP_PORT 3000 /* well-known port */
#define BUFLEN 257

int main(int argc, char **argv)
{
	int n;
	int sd, port;
	struct hostent *hp;
	struct sockaddr_in server;
	char *host, rbuf[BUFLEN], sbuf[BUFLEN - 1];

	switch (argc)
	{
	case 2:
		host = argv[1];
		port = SERVER_TCP_PORT;
		break;
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
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

	if ((hp = gethostbyname(host)))
		bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	else if (!inet_aton(host, (struct in_addr *)&server.sin_addr))
	{
		fprintf(stderr, "Can't get server's address\n");
		exit(1);
	}

	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		fprintf(stderr, "Can't connect\n");
		exit(1);
	}

	printf("Enter the filename to download: ");
	fgets(sbuf, sizeof(sbuf), stdin); // reads line text from stdin; reads what user wrote in terminal
	sbuf[strcspn(sbuf, "\n")] = '\0'; // Remove newline character
	write(sd, sbuf, strlen(sbuf));	  // Send filename to server

	FILE *file = fopen("downloaded_file", "ab"); // open a file called "downloaded_file" in append binary mode
	if (!file)
	{
		fprintf(stderr, "Failed to open the file for writing.\n");
		close(sd);
		return (1); // Return an error code
	}

	/*while ((n = read(sd, rbuf, BUFLEN)) > 0)
	{
		fwrite(rbuf, 1, n, file);
	}*/
	while ((n = read(sd, rbuf, BUFLEN)) > 0) // if read sucssessfully return 1
	{
		if (strncmp(rbuf, "ERROR:", 6) == 0) // comparing: if it reads first 6 characters as "ERROR:"
		{
			fprintf(stderr, "%s\n", rbuf); // Print the error message to the terminal
			break;						   // Exit the loop
		}
		fwrite(rbuf, 1, n, file); // if no error is detected then write the contents of file from server into new file
	}

	fclose(file); // Close the file once all data has been received
	close(sd);
	return (0);
}
