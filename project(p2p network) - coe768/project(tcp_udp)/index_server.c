/* index_server.c - main */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

#define BUFLEN 100
struct sockaddr_in index_addr, peer_addr; /* the from address of a client */
socklen_t addr_len;                       /* from-address length		*/
int sockfd;
int port = 3000;
char errorMSG[BUFLEN], successMSG[BUFLEN];
struct pdu
{
    char type;
    char data[BUFLEN];
} spdu;

// Content information structure
struct content_info
{
    char peer_name[10], content_name[10], address_port[80];
};
struct content_info content_list[BUFLEN]; // Array of content_info

// Function declarations
void registerContent(struct pdu *p);
int searchContent(struct pdu *p);
void listRegisteredContent();
int findDuplicateContent(struct pdu *p);
void deRegisterContent(struct pdu *p);
void error(struct pdu *p, char error[]);
void acknowledge(struct pdu *p, char success[]);
void quitCommand(struct pdu *p);
int main(int argc, char *argv[])
{

    char buf[100]; /* "input" buffer; any size > 0 */

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

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("[-] Socket creation error");
        exit(1);
    }
    // memset(&peer_addr, 0, sizeof(peer_addr));
    memset(&index_addr, 0, sizeof(index_addr));
    index_addr.sin_family = AF_INET;
    index_addr.sin_addr.s_addr = INADDR_ANY;
    index_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&index_addr, sizeof(index_addr)) < 0)
    {
        perror("[-] Bind error");
        exit(1);
    }
    // listen(sockfd, 5);
    // addr_len = sizeof(peer_addr);

    while (1)
    {
        addr_len = sizeof(peer_addr);

        if (recvfrom(sockfd, &spdu, sizeof(spdu), 0, (struct sockaddr *)&peer_addr, &addr_len) < 0)
        {
            perror("[-]UDP Receive Error");
        }
        else
        {
            switch (spdu.type)
            {
            case 'R':
                printf("[+]Received Data: Type - %c, Data - %s\n", spdu.type, spdu.data);
                registerContent(&spdu);
                break;
            case 'S':
                printf("[+]Received Data Type: %c.", spdu.type);
                searchContent(&spdu);
                break;
            case 'O':
                listRegisteredContent();
                break;
            case 'T':
                deRegisterContent(&spdu);
                break;
            case 'Q':
                quitCommand(&spdu);
            default:
                printf("[-]Received Unknown Data Type: %c\n", spdu.type);
                // Handle other unknown types
                break;
            }
        }
    }
} // end of main

int content_count = 0;
void registerContent(struct pdu *p)
{
    if (content_count >= BUFLEN)
    {
        printf("Content list is full.\n");
        return;
    }

    if (findDuplicateContent(p) == -1)
    {
        memset(&content_list[content_count], 0, sizeof(struct content_info));
        strncpy(content_list[content_count].peer_name, p->data, 9);
        strncpy(content_list[content_count].content_name, p->data + 10, 9);
        strncpy(content_list[content_count].address_port, p->data + 20, 79);
        content_count++;
        strcpy(successMSG, "Content Registration Successful.");
        acknowledge(p, successMSG);
        // printf("Content added: [%s, %s, %s]\n", content_list[content_count - 1].peer_name, content_list[content_count - 1].content_name, content_list[content_count - 1].address_port);
    }
    else // duplicate found: print error message
    {
        strcpy(errorMSG, "Content of the same name already exits. Register With A Different Name");
        error(p, errorMSG);
    }
}
int findDuplicateContent(struct pdu *p)
{
    // Ensure that the strings are null-terminated
    p->data[9] = '\0';  // Null-terminate the peer_name
    p->data[19] = '\0'; // Null-terminate the content_name

    // Search for matching content info
    for (int i = 0; i < content_count; i++)
    {
        if (strncmp(content_list[i].peer_name, p->data, 10) == 0 &&
            strncmp(content_list[i].content_name, p->data + 10, 10) == 0)
        {
            return i;
            break;
        }
    }
    return -1;
}

void listRegisteredContent()
{
    // Sending the size of the list for the peer to keep track of when the list ends
    int list_size = content_count;
    sendto(sockfd, &list_size, sizeof(list_size), 0, (struct sockaddr *)&peer_addr, addr_len);
    // Sending the content_list
    for (int i = 0; i < content_count; i++)
    {
        sendto(sockfd, &content_list[i], sizeof(struct content_info), 0, (struct sockaddr *)&peer_addr, addr_len);
        printf("%d: %s %s %s\n", i, content_list[i].peer_name, content_list[i].content_name, content_list[i].address_port);
    }
}
int searchContent(struct pdu *p)
{
    // Add this line at the beginning of the searchContent function
    printf("Sending Search Query: |%c|%s|\n", p->type, p->data);

    int i = findDuplicateContent(p); // set the return value given by find duplicate to i.

    if (i == -1)
    {
        // If no matching content is found
        strcpy(errorMSG, "No such content or file");
        error(p, errorMSG);
        return -1; // Return -1 to indicate an error when no matching content is found
    }

    // If matching content is found
    p->type = 'S';
    snprintf(p->data, BUFLEN, "%s %s", content_list[i].peer_name, content_list[i].address_port);
    ssize_t sent = sendto(sockfd, p, sizeof(struct pdu), 0, (struct sockaddr *)&peer_addr, addr_len);
    printf("[+]Content sent: [%s, %s, %s]\n", content_list[i].peer_name, content_list[i].content_name, content_list[i].address_port);
    // printf("PDU data: %s\n", p->data);
    return 0; // Return here to exit the function after successful content send
}
void deRegisterContent(struct pdu *p)
{
    // Find and remove all entries with the given peer_name
    p->data[9] = '\0'; // Null-terminate the peer_name
    int removedCount = 0;

    for (int i = 0; i < content_count;)
    {
        if (strncmp(content_list[i].peer_name, p->data, 10) == 0)
        {
            // Remove the entry by shifting elements
            for (int j = i; j < content_count - 1; j++)
            {
                memcpy(&content_list[j], &content_list[j + 1], sizeof(struct content_info));
            }

            // Decrease the content count
            content_count--;
            removedCount++;
        }
        else
        {
            // Move to the next entry if no match
            i++;
        }
    }

    if (removedCount > 0)
    {
        p->type = 'T';
        snprintf(p->data, sizeof(p->data), "Content Deregistered");
        sendto(sockfd, p, sizeof(struct pdu), 0, (struct sockaddr *)&peer_addr, addr_len);
        printf("Removed %d content(s) for peer_name: [%s]\n", removedCount, p->data);
    }
    else
    {
        // No content found for deregistration
        printf("Content not found for deregistration.\n");
        strcpy(errorMSG, "Content not found by this user (peer_name)");
        error(p, errorMSG);
    }
}


void quitCommand(struct pdu *p)
{
    // Check if command_list is empty
    if (content_count == 0)
    {
        p->type = 'A';
        sendto(sockfd, &(p->type), sizeof(p->type), 0, (struct sockaddr *)&peer_addr, addr_len);
    }
    else
    {
        p->type = 'E';
        sendto(sockfd, &(p->type), sizeof(p->type), 0, (struct sockaddr *)&peer_addr, addr_len);
    }
}
void error(struct pdu *p, char errorMSG[])
{
    p->type = 'E';
    snprintf(p->data, BUFLEN, "%s %s", "Error: ", errorMSG);
    sendto(sockfd, p, sizeof(struct pdu), 0, (struct sockaddr *)&peer_addr, addr_len);
    printf("[+]Sending Error: %c|%s|\n", p->type, p->data);
}
void acknowledge(struct pdu *p, char successMSG[])
{
    p->type = 'A';
    snprintf(p->data, BUFLEN, "%s", successMSG);
    sendto(sockfd, p, sizeof(struct pdu), 0, (struct sockaddr *)&peer_addr, addr_len);
    printf("[+]Sending Acknowledgement: %c|%s|\n", p->type, p->data);

}

// DO NOT DELETE
//  int searchContent(struct pdu *p)
//  {

//     // Add this line at the beginning of searchContent function
//     printf("Sending Search Query: |%c|%s|\n", spdu.type, spdu.data);

//     int i = findDuplicateContent(p); // set the return value given by find duplicate to i.

//     if (i != -1)
//     {
//         p->type = 'S';
//         snprintf(p->data, BUFLEN, "%s %s", content_list[i].peer_name, content_list[i].address_port);
//         ssize_t sent = sendto(sockfd, p, sizeof(struct pdu), 0, (struct sockaddr *)&peer_addr, addr_len);
//         printf("Content sent: [%s, %s, %s]\n", content_list[i].peer_name, content_list[i].content_name, content_list[i].address_port);
//         printf("PDU data: %s\n", p->data);
//     }
//     // If no matching content is found
//     strcpy(errorMSG, "No such content or file");
//     error(p, errorMSG);

//     // printf("No matching content found for [%s, %s]\n", p->data, p->data + 10);
//     return (0);
// }

// void deRegisterAllContent(struct pdu *p)
// {
//     // Reset content count to zero
//     content_count = 0;

//     // Clear the content_list array
//     memset(content_list, 0, sizeof(content_list));

//     // Create a new pdu to send the de-registration message
//     p->type = 'T';
//     snprintf(p->data, sizeof(p->data), "Content deregistered");

//     // Send the de-registration message
//     sendto(sockfd, p, sizeof(struct pdu), 0, (struct sockaddr *)&peer_addr, addr_len);

//     printf("Content list has been emptied.\n");
// }
// void deRegisterContent(struct pdu *p)
// {
//     // Find the index of the content to be deregistered
//     p->data[9] = '\0'; // Null-terminate the peer_name
//     int foundIndex = -1;
//     for (int i = 0; i < content_count; i++)
//     {
//         if (strncmp(content_list[i].peer_name, p->data, 10) == 0)
//         {
//             foundIndex = i;
//             break;
//         }
//     }
//     // If the content is found, remove it by shifting elements
//     if (foundIndex != -1)
//     {
//         for (int i = foundIndex; i < content_count - 1; i++)
//         {
//             memcpy(&content_list[i], &content_list[i + 1], sizeof(struct content_info));
//         }
//         // Decrease the content count
//         content_count--;
//         p->type = 'T';
//         snprintf(p->data, sizeof(p->data), "Content Deregistered");
//         sendto(sockfd, p, sizeof(struct pdu), 0, (struct sockaddr *)&peer_addr, addr_len);
//         printf("Content removed: [%s, %s, %s]\n", content_list[foundIndex].peer_name, content_list[foundIndex].content_name, content_list[foundIndex].address_port);
//     }
//     else
//     {
//         // Content not found
//         printf("Content not found for deregistration.\n");
//         strcpy(errorMSG, "Content not found by this user (peer_name)");
//         error(p, errorMSG);
//     }
// }
