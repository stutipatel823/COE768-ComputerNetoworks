/* peer.c */
/* peer.c uses TCP to download and UDP to register contents into index_server.c */
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFLEN 100
#define PACKET_SIZE 100
#define MAX_CON 10000

char errorMSG[BUFLEN], successMSG[BUFLEN];

struct pdu
{
    char type;
    char data[BUFLEN];
} spdu;
struct content_info
{
    char peer_name[10], content_name[10], address_port[80];
};

struct sockets
{
    int socket;
    int port;
};
struct sockets servers[MAX_CON], clients[BUFLEN];
int server_count = 0, client_count = 0;

fd_set rfds, afds;

int udp_sock;
struct sockaddr_in udp_addr;
char *host = "localhost";
int port = 3000;
int bytes_read, bytesRead;

void registerContent(int udp_sock, struct sockaddr_in udp_addr);
void searchContent(int udp_sock, struct sockaddr_in udp_addr);
void contentList(int udp_sock, struct sockaddr_in udp_addr);
int downloadRequest();
void contentDataTransfer(char filename[BUFLEN]);
int create_tcp_server();
int create_tcp_client(int tcp_port);
void contentDeRegister(int udp_sock, struct sockaddr_in udp_addr);
void quitCommand(int udp_sock, struct sockaddr_in udp_addr);

int main(int argc, char *argv[])
{
    switch (argc)
    {
    case 1:
        break;
    case 2:
        host = argv[1];
        break;
    case 3:
        host = argv[1];
        port = atoi(argv[2]);
        break;
    default:
        fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
        exit(1);
    }

    /* Map host name to IP address, allowing for dotted decimal */
    struct hostent *phe; /* pointer to host information entry */
    if (!(phe = gethostbyname(host)))
    {
        fprintf(stderr, "Can't get host entry\n");
        exit(1);
    }

    // UDP setup
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(port);
    memcpy(&udp_addr.sin_addr, phe->h_addr, phe->h_length);

    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("[-]UDP Socket Error");
        exit(1);
    }

    if (connect(udp_sock, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0)
    {
        fprintf(stderr, "Can't connect to %s %s\n", host, "Time");
        exit(1);
    }

    FD_ZERO(&afds);
    FD_SET(STDIN_FILENO, &afds); // Add stdin to the set
    int exit_flag = 0;
    while (1) // Main loop
    {
        printf("Press:\nR: to registerContent\nS: to searchContent\nO: to listContents\nD: to downloadRequest\nT: to contentDeRegister\nQ: to Quit\n");

        memcpy(&rfds, &afds, sizeof(rfds));
        int max_fd = 0;
        for (int i = 0; i < server_count; i++)
        {
            if (servers[i].socket > max_fd)
            {
                max_fd = servers[i].socket;
            }
        }
        for (int i = 0; i < client_count; i++)
        {
            if (clients[i].socket > max_fd)
            {
                max_fd = clients[i].socket;
            }
        }
        max_fd = (max_fd > STDIN_FILENO) ? max_fd : STDIN_FILENO; // Adjust max_fd
        if (select(max_fd + 1, &rfds, NULL, NULL, NULL) < 0)
        {
            perror("Select error");
            exit(EXIT_FAILURE);
        }
        // Check if stdin has data
        if (FD_ISSET(STDIN_FILENO, &rfds))
        {
            char choice;
            // printf("Press:\nR: to registerContent\nS: to searchContent\nO: to listContents\nD: to downloadRequest\nT: to contentDeRegister\nQ: to Quit\n");
            scanf(" %c", &choice);
            switch (choice)
            {
            case 'R':
            case 'r':
                registerContent(udp_sock, udp_addr);
                break;
            case 'S':
            case 's':
                searchContent(udp_sock, udp_addr);
                break;
            case 'O':
            case 'o':
                contentList(udp_sock, udp_addr);
                break;
            case 'D':
            case 'd':
                // if (downloadRequest() == 0)
                // {
                //     close(udp_sock);
                //     return 0; // Exit the program
                // }
                downloadRequest();
                break;
            case 'T':
            case 't':
                contentDeRegister(udp_sock, udp_addr);
                break;
            case 'Q':
            case 'q':
                quitCommand(udp_sock, udp_addr);
                close(udp_sock);
                exit(EXIT_SUCCESS);
                break;
            default:
                printf("Invalid Choice\n");
                break;
            }
        }
        for (int i = 0; i < server_count; i++)
        {
            if (FD_ISSET(servers[i].socket, &rfds))
            { // Handle New Client Connections
                struct sockaddr_in tcp_client_addr;
                socklen_t tcp_client_addr_len = sizeof(tcp_client_addr);
                int tcp_client_sock = accept(servers[i].socket, (struct sockaddr *)&tcp_client_addr, &tcp_client_addr_len);
                if (tcp_client_sock < 0)
                {
                    perror("accept error");
                }
                FD_SET(tcp_client_sock, &afds);
                printf("[+] Client connected from IP %s and port %d\n", inet_ntoa(tcp_client_addr.sin_addr), ntohs(tcp_client_addr.sin_port));
                // Download Request and Content Transmission
                read(tcp_client_sock, &spdu, sizeof(spdu));
                printf("Server: Type: %c, Data: %s\n", spdu.type, spdu.data);
                if (spdu.type == 'D')
                {
                    FILE *file = fopen(spdu.data, "r");
                    if (!file)
                    {
                        spdu.type = 'E';
                        snprintf(spdu.data, BUFLEN, "File not found");
                        write(tcp_client_sock, &spdu, sizeof(spdu));
                        perror("File not found");
                        close(tcp_client_sock);
                        continue;
                    }

                    // Send the entire spdu structure with type 'C' to the client_sock
                    spdu.type = 'C';
                    write(tcp_client_sock, &spdu, sizeof(spdu));

                    // int bytesRead;
                    while ((bytesRead = fread(spdu.data, 1, PACKET_SIZE, file)) > 0)
                    {
                        // Send only spdu.data to the client_sock
                        write(tcp_client_sock, spdu.data, bytesRead);
                    }

                    fclose(file);
                }
                close(tcp_client_sock);
            } // END OF TCP SERVER
        }
    }
    return 0;
}

int downloadRequest()
{
    spdu.type = 'D';
    char filename[BUFLEN];
    int tcp_server_port;

    printf("Enter TCP server's port number: ");
    scanf("%d", &tcp_server_port);

    printf("Enter content name: ");
    scanf("%s", filename);
    filename[BUFLEN - 1] = '\0'; // Ensure null termination

    snprintf(spdu.data, BUFLEN, "%s", filename);
    printf("|%c|%s|\n", spdu.type, spdu.data);

    // Create a TCP client to retrieve data from the owner of that content(TCP server)
    int tcp_client_sock = create_tcp_client(tcp_server_port);
    write(tcp_client_sock, &spdu, sizeof(spdu)); // Asking TCP server for the content

    char newfilename[BUFLEN];
    // snprintf(newfilename, BUFLEN, "new_%s", filename);
    snprintf(newfilename, BUFLEN, "%s", filename);
    FILE *newfile = fopen(newfilename, "w");
    if (!newfile)
    {
        perror("Error opening new file");
        close(tcp_client_sock);
        return -1;
    }

    // Read the entire spdu structure from the server
    read(tcp_client_sock, &spdu, sizeof(spdu));
    if (spdu.type == 'E')
    {
        printf("|%c|%s|\n", spdu.type, spdu.data);
        fclose(newfile);
        close(tcp_client_sock);
        return -1;
    }

    // int bytesRead;
    while ((bytesRead = read(tcp_client_sock, spdu.data, PACKET_SIZE)) > 0)
    {
        printf("|%c|%s|\n", spdu.type, spdu.data);
        fwrite(spdu.data, 1, bytesRead, newfile);
    }

    fclose(newfile);
    close(tcp_client_sock);
    // Re-registration process
    char new_peer_name[10], new_content_name[10];

    printf("Enter new peer name for re-registration (up to 10 characters): ");
    scanf("%9s", new_peer_name); // Read new peer name

    // Use the existing registerContent function to re-register the content
    // You may need to modify registerContent to accept parameters
    // For simplicity, here we're calling it directly with global variables
    snprintf(spdu.data, sizeof(spdu.data), "%-10s%-10s%d", new_peer_name, filename, create_tcp_server());
    spdu.type = 'R';

    // Send data over UDP
    if (write(udp_sock, &spdu, sizeof(spdu)) < 0)
    {
        perror("[-]UDP Send Error in re-registration");
    }
    else
    {
        memset(spdu.data, 0, sizeof(spdu.data));
        read(udp_sock, &spdu, sizeof(spdu));
        while (spdu.type == 'E') //Keep asking user to enter a different name until the content is registered with a unique name to get rid of name conflict
        {
            printf("|%c|%s|\n", spdu.type, spdu.data);
            printf("Enter new peer name for re-registration (up to 10 characters): ");
            scanf("%9s", new_peer_name); // Read new peer name

            // Assuming filename and create_tcp_server() are defined somewhere in your code
            snprintf(spdu.data, sizeof(spdu.data), "%-10s%-10s%d", new_peer_name, filename, create_tcp_server());
            spdu.type = 'R';

            write(udp_sock, &spdu, sizeof(spdu));

            memset(spdu.data, 0, sizeof(spdu.data)); // Clear data for the next read
            read(udp_sock, &spdu, sizeof(spdu));
        }
        printf("|%c|%s|\n", spdu.type, spdu.data);
    }

    return 0;
}

int create_tcp_server()
{
    struct sockaddr_in tcp_addr;
    int tcp_server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_server_sock < 0)
    {
        perror("TCP Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(0); // Let OS assign a port
    tcp_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(tcp_server_sock, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0)
    {
        perror("TCP Bind failed");
        close(tcp_server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(tcp_server_sock, MAX_CON) < 0)
    {
        perror("TCP Listen failed");
        close(tcp_server_sock);
        exit(EXIT_FAILURE);
    }

    socklen_t addr_len = sizeof(tcp_addr);
    if (getsockname(tcp_server_sock, (struct sockaddr *)&tcp_addr, &addr_len) < 0)
    {
        perror("TCP Getsockname failed");
        close(tcp_server_sock);
        exit(EXIT_FAILURE);
    }
    int tcp_server_port = ntohs(tcp_addr.sin_port);
    // printf("[SERVER]TCP server set up on port %d.\n", tcp_server_port);
    // Add tcp server to servers list
    servers[server_count].port = tcp_server_port;
    servers[server_count].socket = tcp_server_sock;
    server_count++;
    FD_SET(tcp_server_sock, &afds);
    return tcp_server_port;
}

int create_tcp_client(int tcp_port)
{
    struct sockaddr_in tcp_client_addr;
    socklen_t tcp_client_addr_len = sizeof(tcp_client_addr);
    int tcp_client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_client_sock < 0)
    {
        perror("TCP Client Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&tcp_client_addr, 0, sizeof(tcp_client_addr));
    tcp_client_addr.sin_family = AF_INET;
    tcp_client_addr.sin_port = htons(tcp_port);
    // tcp_client_addr.sin_addr.s_addr = htonl(INADDR_ANY); // You might want to specify the actual IP address or use INADDR_ANY
    // printf("%d\n", tcp_port);
    if (connect(tcp_client_sock, (struct sockaddr *)&tcp_client_addr, sizeof(tcp_client_addr)) < 0)
    {
        perror("TCP Client Connect failed");
        exit(EXIT_FAILURE);
        return -1;
    }
    int tcp_client_port = ntohs(tcp_client_addr.sin_port);
    // printf("[CLIENT]TCP client set up on port %d.\n", tcp_client_port);
    clients[client_count].port = tcp_client_port;
    clients[client_count].socket = tcp_client_sock;
    client_count++;
    FD_SET(tcp_client_sock, &afds);
    return tcp_client_sock;
}

void contentList(int udp_sock, struct sockaddr_in udp_addr)
{
    spdu.type = 'O';
    write(udp_sock, &spdu.type, 1);

    // Receive the size of the content list first
    int list_size;
    if (read(udp_sock, &list_size, sizeof(list_size)) <= 0)
    {
        perror("[-]Error in receiving list size.");
        return;
    }
    struct content_info received_content;
    for (int i = 0; i < list_size; i++)
    {
        read(udp_sock, &received_content, sizeof(received_content));
        printf("%d: %s %s %s\n", i, received_content.peer_name, received_content.content_name, received_content.address_port);
    }
}

void searchContent(int udp_sock, struct sockaddr_in udp_addr)
{
    // Set up the search query
    spdu.type = 'S';
    char peer_name[10], content_name[10];
    printf("Enter peer name (up to 10 characters): ");
    scanf("%9s", peer_name); // Read peer name
    printf("Enter content name (up to 10 characters): ");
    scanf("%9s", content_name); // Read content name

    // Format the pdu for sending
    snprintf(spdu.data, sizeof(spdu.data), "%-10s%-10s", peer_name, content_name);
    // printf("Search Query: |%c|%s|\n", spdu.type, spdu.data);

    if (write(udp_sock, &spdu, sizeof(spdu)) < 0)
    {
        perror("[-]UDP Send Error");
    }
    memset(spdu.data, 0, sizeof(spdu.data));
    read(udp_sock, &spdu, sizeof(spdu));
    printf("|%c|%s|\n", spdu.type, spdu.data);
}

void registerContent(int udp_sock, struct sockaddr_in udp_addr)
{
    char peer_name[10], content_name[10]; // Exactly 10 bytes for names
    printf("Enter peer name (up to 10 characters): ");
    scanf("%9s", peer_name);

    printf("Enter content name (up to 10 characters): ");
    scanf("%9s", content_name);

    // Create a TCP server
    int tcp_server_port = create_tcp_server();

    // Format and store in pdu
    snprintf(spdu.data, sizeof(spdu.data), "%-10s%-10s%d", peer_name, content_name, tcp_server_port);
    spdu.type = 'R';

    // Send data over UDP
    if (write(udp_sock, &spdu, sizeof(spdu)) < 0)
    {
        perror("[-]UDP Send Error");
    }
    else
    {
        memset(spdu.data, 0, sizeof(spdu.data));
        read(udp_sock, &spdu, sizeof(spdu));
        printf("|%c|%s|\n", spdu.type, spdu.data);
    }
}

void contentDeRegister(int udp_sock, struct sockaddr_in udp_addr)
{
    spdu.type = 'T';
    char peer_name[10]; // Exactly 10 bytes for names
    printf("Enter peer name (up to 10 characters): ");
    scanf("%9s", peer_name);
    snprintf(spdu.data, sizeof(spdu.data), "%-10s", peer_name);
    write(udp_sock, &spdu, sizeof(spdu));
    // Receive the de-registration message
    ssize_t received_bytes = recvfrom(udp_sock, &spdu, sizeof(spdu), 0, NULL, NULL);
    if (received_bytes < 0)
    {
        perror("[-]UDP Receive Error");
        return;
    }
    printf("[+]|%c|%s|\n sent to TCP Server", spdu.type, spdu.data);
}

void quitCommand(int udp_sock, struct sockaddr_in udp_addr)
{
    spdu.type = 'Q';
    write(udp_sock, &spdu.type, 1);
    read(udp_sock, &spdu, sizeof(spdu));
    if (spdu.type == 'E')
    {
        printf("Deregistering Content First...\n");
        contentDeRegister(udp_sock, udp_addr);
    }
    printf("Quitting...\n");
}
