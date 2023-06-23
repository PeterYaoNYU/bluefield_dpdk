#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define UDP_PORT 12345 // Replace with the desired port number

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[1024];

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(UDP_PORT);

    // Bind the socket to the IP address and port
    if (bind(sockfd, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Listening for UDP packets on port %d...\n", UDP_PORT);

    while (1) {
        // Receive a packet
        int len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &addr_len);
        if (len < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }

        printf("Received packet from %s:%d: %s\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
    }

    close(sockfd);
    return 0;
}
