/* Reset Failure Command Program */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <endian.h>

#include "acuData.h"
#include "acuCommands.h"

/* Calculate checksum as per ICD requirements */
short calculate_checksum(const char *data, size_t length) {
    int checksum = 0;

    for (size_t i = 0; i < length; i++) {
        checksum += (int8_t)data[i];  // Treat data as signed 8-bit integers
    }

    return (short)(checksum & 0xFFFF);  // Return lower 16 bits
}

int main(void) {
    int sockfd = 0, n = 0;
    char recvBuff[256];
    char sendBuff[256];
    struct sockaddr_in serv_addr;
    acuCmd acuCommand;

    /* Construct the Failure Reset Command */
    acuCommand.stx = 0x02;                       // Start byte
    acuCommand.id = 0x58;                        // Command ID
    acuCommand.datalength = htole16(0x07);       // Length in little-endian
    acuCommand.checksum = htole16(calculate_checksum(
        (char *)&acuCommand.id, 
        offsetof(acuCmd, checksum) - offsetof(acuCmd, id)
    ));                                          // Checksum in little-endian
    acuCommand.etx = 0x03;                       // End byte



    memset(recvBuff, 0, sizeof(recvBuff));
    memset(sendBuff, 0, sizeof(sendBuff));

    /* Create Socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error: Could not create socket");
        return 1;
    }

    /* Configure Server Address */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9010); // Port
    serv_addr.sin_addr.s_addr = inet_addr("192.168.1.103"); // ACU IP address

    /* Connect to ACU */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error: Connect failed");
        return 1;
    }

    /* Send Command */
    memcpy(sendBuff, (char *)&acuCommand, sizeof(acuCommand));
    n = send(sockfd, sendBuff, sizeof(acuCommand), 0);
    if (n < 0) {
        perror("Error: Writing to ACU");
        close(sockfd);
        return 1;
    }
    printf("Wrote %d bytes to ACU\n", n);

    /* Debug: Display Sent Bytes */
    printf("Sent command bytes: ");
    for (int i = 0; i < sizeof(acuCommand); i++) {
        printf("0x%02X ", ((unsigned char *)&acuCommand)[i]);
    }
    printf("\n");

    /* Receive Response */
    n = recv(sockfd, recvBuff, sizeof(recvBuff), 0);
    if (n < 0) {
        perror("Error: Reading from ACU");
        close(sockfd);
        return 1;
    } else if (n == 0) {
        printf("Connection closed by ACU.\n");
        close(sockfd);
        return 1;
    }

    /* Debug: Display Received Bytes */
    printf("Raw response (%d bytes): ", n);
    for (int i = 0; i < n; i++) {
        printf("0x%02X ", (unsigned char)recvBuff[i]);
    }
    printf("\n");

    /* Process Response */
    if (recvBuff[0] == 0x06) {
        printf("Failure reset acknowledged.\n");
    } else if (recvBuff[0] == 0x15) {
        printf("ACU refused the command. Reason: ");
        switch (recvBuff[1]) {
            case 0x43: printf("Checksum error.\n"); break;
            case 0x45: printf("ETX not received at expected position.\n"); break;
            case 0x49: printf("Unknown identifier.\n"); break;
            case 0x4C: printf("Wrong length (incorrect number of bytes received).\n"); break;
            case 0x6C: printf("Specified length does not match identifier.\n"); break;
            case 0x4D: printf("Command ignored in present operating mode.\n"); break;
            case 0x6F: printf("Other reasons.\n"); break;
            case 0x52: printf("Device not in Remote mode.\n"); break;
            case 0x72: printf("Value out of range.\n"); break;
            case 0x53: printf("Missing STX.\n"); break;
            default:   printf("Unknown error code 0x%02X.\n", recvBuff[1]);
        }
    } else {
        printf("Unexpected response: 0x%02X\n", recvBuff[0]);
    }

    close(sockfd);
    return 0;
}
