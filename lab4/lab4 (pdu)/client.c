/* client.c */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFLEN 100
struct pdu
{
    char type;
    char data[BUFLEN];
} spdu;

int main(int argc, char **argv)
{
    char *host = "localhost"; // Default host
    int port = 3000;          // Default port
    char now[100];            /* 32-bit integer to hold time*/
    struct sockaddr_in sin;   /* an Internet endpoint address*/
    int s;                    /* socket descriptor */

    switch (argc)
    {
    case 1:
        break;
    case 2:
        host = argv[1];
    case 3:
        host = argv[1];
        port = atoi(argv[2]);
        break;
    default:
        fprintf(stderr, "usage: UDPtime [host [port]]\n");
        exit(1);
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    /* Map host name to IP address, allowing for dotted decimal */
    struct hostent *phe; /* pointer to host information entry	*/
    if (!(phe = gethostbyname(host)))
    {
        fprintf(stderr, "Can't get host entry \n");
        exit(1);
    }
    memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);

    /* Allocate a socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        fprintf(stderr, "Can't create socket \n");

    /* Connect the socket */
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        fprintf(stderr, "Can't connect to %s %s \n", host, "Time");

    /*Type C: Read/Send Filename*/
    spdu.type = 'C';
    printf("Enter Filename: ");
    scanf("%s", spdu.data);
    write(s, &spdu, sizeof(spdu));
    FILE *newfile = fopen("new_file_from_server", "w");
    while (1)
    {
        int bytes_read = read(s, &spdu, sizeof(spdu));
        if (bytes_read < 0)
        {
            perror("Error");
            exit(1);
        }
        else
        {
            switch (spdu.type)
            {
            case 'E':
                printf("Error from server: %s\n", spdu.data);
                fclose(newfile);
                exit(1); // Exit on error
                break;
            case 'D':
                fwrite(spdu.data, 1, bytes_read - sizeof(char), newfile); // subtracting 1 byte becuz spdu has a total of 101 bytes (sizeof(data)+sizeof(type))

                break;
            case 'F':
                fwrite(spdu.data, 1, bytes_read - sizeof(char), newfile); // subtracting 1 byte becuz spdu has a total of 101 bytes (sizeof(data)+sizeof(type))
                fclose(newfile);
                printf("File Successfully Downloaded.\n");
                close(s);
                exit(0);
                break;
            }
        }
    }
    // close(s);
    // exit(0);
}
