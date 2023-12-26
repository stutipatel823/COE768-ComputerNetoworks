/* server.c */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

#define BUFLEN 100

struct pdu
{
    char type;
    char data[BUFLEN];
} spdu;

int main(int argc, char *argv[])
{
    int s; /* socket descriptor*/
    int port = 3000;
    struct sockaddr_in sin;  /* an Internet endpoint address*/
    struct sockaddr_in fsin; /* the from address of a client	*/
    char buf[BUFLEN];        /* "input" buffer; any size > 0	*/
    socklen_t alen;          /* from-address length*/

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
        fprintf(stderr, "can't create socket\n");

    /* Bind the socket */
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        fprintf(stderr, "can't bind to %d port\n", port);

    alen = sizeof(fsin);
    while (1)
    {
        recvfrom(s, &spdu, sizeof(spdu), 0, (struct sockaddr *)&fsin, &alen);

        printf("Received Type: %c, Data: %s\n", spdu.type, spdu.data);
        if (spdu.type == 'C')
        {
            char filename[BUFLEN];
            strcpy(filename, spdu.data);
            FILE *file = fopen(filename, "r");
            if (!file)
            {
                spdu.type = 'E';
                strcpy(spdu.data, "File not found");
                perror("File not found");
                sendto(s, &spdu, sizeof(spdu), 0, (struct sockaddr *)&fsin, sizeof(fsin));
            }
            else
            {
                int bytes_read;
                while ((bytes_read = fread(spdu.data, 1, BUFLEN, file)) > 0)
                {
                    if (feof(file))
                    {
                        spdu.type = 'F';                                                                        // Last batch
                        sendto(s, &spdu, bytes_read + sizeof(char), 0, (struct sockaddr *)&fsin, sizeof(fsin)); // adding 1 more byte becuz spdu has a total of 101 bytes (sizeof(data)+sizeof(type))
                    }
                    else
                    {
                        spdu.type = 'D';                                                                        // Data batch
                        sendto(s, &spdu, bytes_read + sizeof(char), 0, (struct sockaddr *)&fsin, sizeof(fsin)); // adding 1 more byte becuz spdu has a total of 101 bytes (sizeof(data)+sizeof(type))
                    }
                }
                fclose(file);
            }
        }
    }
    return 0;
}
