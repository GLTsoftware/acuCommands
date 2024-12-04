#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stddef.h>
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
    double hours, minutes, seconds;
    struct sockaddr_in serv_addr;
    ioStatus ioStatusResp;
    acuCmd acuCommand;
    short azmotor1temp, azmotor2temp, elmotor1temp, elmotor2temp;
    short az1motorcurrent, az2motorcurrent, el1motorcurrent, el2motorcurrent;
    float az1motorcurrentF, az2motorcurrentF, el1motorcurrentF, el2motorcurrentF;

    /* Construct the I/O Status Command */
    acuCommand.stx = 0x02;                       // Start byte
    acuCommand.id = 0x51;                        // 'Q' command for I/O status
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
    serv_addr.sin_port = htons(9110);
    serv_addr.sin_addr.s_addr = inet_addr("192.168.1.103");

    /* Connect to ACU */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error: Connect failed");
        close(sockfd);
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

    /* Receive ACK */
    n = recv(sockfd, recvBuff, sizeof(recvBuff), 0);
    if (n < 0) {
        perror("Error: Reading from ACU");
        close(sockfd);
        return 1;
    }

    printf("Received:  0x%x 0x%x from ACU\n", recvBuff[0], recvBuff[1]);

    if (recvBuff[0] == 0x06) {  // ACK received
        /* Receive I/O Status Data */
        n = recv(sockfd, (char *)&ioStatusResp, sizeof(ioStatusResp), 0);
        if (n < 0) {
            perror("Error: Reading I/O status data from ACU");
        } else {
            printf("Read %d bytes from ACU\n", n);
            printf("Time: (day, hh:mm:ss.sss): %d ",
                   ioStatusResp.dayOfYear);
            hours = ioStatusResp.timeOfDay / 3600000.0;
            int hh = (int)hours;
            minutes = (hours - hh) * 60.0;
            int mm = (int)minutes;
            seconds = (minutes - mm) * 60.0;
            printf("%02d:%02d:%06.3f\n", hh, mm, seconds);

            printf("AzPos2 (deg): %.6f\n",
                   (double)ioStatusResp.azPosEnc2 / 1.0e6);
            printf("ElPos2 (deg): %.6f\n",
                   (double)ioStatusResp.elPosEnc2 / 1.0e6);

  if (ioStatusResp.ServoPLCgeneralStatus[0] & 1)
                printf("E-stop 1 - Drive cabinet.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[0] & 2)
                printf("E-stop 2 - ACU.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[0] & 4)
                printf("E-stop 3 - Inside pedestal.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[0] & 8)
                printf("E-stop 4 - Outside pedestal.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[0] & 16)
                printf("E-stop 5 - Az drive.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[0] & 32)
                printf("E-stop 6 - Level 2 container central.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[0] & 64)
                printf("E-stop 7 - El cabin.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[0] & 128)
                printf("E-stop 8 - PCU.\n");

  if (ioStatusResp.ServoPLCgeneralStatus[1] & 1)
                printf("E-stop device.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[1] & 2)
                printf("Receiver cabin door.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[1] & 4)
                printf("Bride L2 central to receiver cabin.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[1] & 8)
                printf("L2 central container cabin access.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[1] & 16)
                printf("L2 central container left door.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[1] & 32)
                printf("L2 central container right door.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[1] & 64)  {
                printf("L2 central container front door.\n");
                printf("(receiver transfer door).\n");
                }
  if (ioStatusResp.ServoPLCgeneralStatus[1] & 128)
                printf("Access stair.\n");

  if (ioStatusResp.ServoPLCgeneralStatus[2] & 1)
                printf("SAFE.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[2] & 8)
                printf("Lightning protection surge arresters.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[2] & 16)
                printf("Power failure.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[2] & 32)
                printf("24 V power failure.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[2] & 64)
                printf("Breaker failure.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[2] & 128)
                printf("Cabinet overtemperature.\n");

  if (ioStatusResp.ServoPLCgeneralStatus[3] & 1)
                printf("ACU I/F error.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[3] & 2)
                printf("Profibus error.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[3] & 4)
                printf("Tiltmeter read error.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[3] & 16)
                printf("Fan failure.\n");

  if (ioStatusResp.ServoPLCgeneralStatus[4] & 4)
                printf("Shutter moving.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[4] & 8)
                printf("Shutter timeout.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[4] & 16)
                printf("Shutter failure.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[4] & 32)
                printf("Shutter open.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[4] & 64)
                printf("Shutter closed.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[4] & 128)
                printf("Shutter hand crank.\n");

  if (ioStatusResp.ServoPLCgeneralStatus[5] & 2)
                printf("Key switch bypass emergency limits.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[5] & 4)
                printf("PCU operation.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[5] & 8)
                printf("reserved.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[5] & 128)
                printf("E-stop customer.\n");

  if (ioStatusResp.ServoPLCgeneralStatus[6] & 1)
                printf("E-stop level 1 container left.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[6] & 2)
                printf("E-stop level 2 container left.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[6] & 4)
                printf("E-stop level 2 container right\n");
  if (ioStatusResp.ServoPLCgeneralStatus[6] & 8)
                printf("E-stop at access stairs.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[6] & 16)
                printf("E-stop operations building.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[6] & 128)
                printf("Collision switch at reflector.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[7] & 1)
                printf("Door L1 left container.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[7] & 2)
                printf("Door L1 right container.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[7] & 4)
                printf("Door L2 left container.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[7] & 8)
                printf("Door L2 right container.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[7] & 16)
                printf("Collision switch L1 corner 1.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[7] & 32)
                printf("Collision switch L1 corner 2.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[7] & 64)
                printf("Collision switch L1 corner 3.\n");
  if (ioStatusResp.ServoPLCgeneralStatus[7] & 128)
                printf("Collision switch L1 corner 4.\n");

  if (ioStatusResp.ServoPLCazimuthStatus[0] & 1)
                printf("Az Computer disabled.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[0] & 2)
                printf("Az Axis disabled.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[0] & 4)
                printf("Az Prelimit cw.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[0] & 8)
                printf("Az Prelimit ccw.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[0] & 16)
                printf("Az Limit cw.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[0] & 32)
                printf("Az Limit ccw.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[0] & 64)
                printf("Az Emergency limit.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[0] & 128)
                printf("Az 2nd emergency limit.\n");

  if (ioStatusResp.ServoPLCazimuthStatus[1] & 1)
                printf("Az Servo failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[1] & 2)
                printf("Az Motion error.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[1] & 4)
                printf("Az Brake 1 failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[1] & 8)
                printf("Az Brake 2 failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[1] & 16)
                printf("Brake 3 failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[1] & 32)
                printf("Brake 4 failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[1] & 64)
                printf("Brake warning.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[1] & 128)
                printf("Az Breaker failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[2] & 1)
                printf("Az Amplifier 1 failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[2] & 2)
                printf("Az Amplifier 2 failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[2] & 4)
                printf("Amplifier 3 failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[2] & 8)
                printf("Amplifier 4 failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[2] & 16)
                printf("Az Motor 1 overtemperature.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[2] & 32)
                printf("Az Motor 2 overtemperature.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[2] & 64)
                printf("Motor 3 overtemperature.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[2] & 128)
                printf("Motor 4 overtemperature.\n");

  if (ioStatusResp.ServoPLCazimuthStatus[3] & 1)
                printf("Az Aux 1 mode selected.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[3] & 2)
                printf("Az Aux 2 mode selected.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[3] & 4)
                printf("Az Axis in stop.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[3] & 8)
                printf("DC bus overvoltage.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[3] & 16)
                printf("Az Hand crank inserted.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[3] & 32)
                printf("Az Foot switch activated.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[3] & 64)
                printf("Az Overspeed.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[3] & 128)
                printf("Az brakes released.\n");

  if (ioStatusResp.ServoPLCazimuthStatus[4] & 1)
                printf("Az Gearbox 1 low oil level.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[4] & 2)
                printf("Az Gearbox 2 low oil level.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[4] & 4)
                printf("Gearbox 3 low oil level.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[4] & 8)
                printf("Gearbox 4 low oil level.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[4] & 16)
                printf("Az Gearbox 1 heater failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[4] & 32)
                printf("Az Gearbox 2 heater failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[4] & 64)
                printf("Gearbox 3 heater failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[4] & 128)
                printf("Gearbox 4 heater failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[5] & 1)
                printf("Az Stow pin 1 inserted.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[5] & 2)
                printf("Stow pin 2 inserted.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[5] & 4)
                printf("Az Stow pin 1 retracted.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[5] & 8)
                printf("Stow pin 2 retracted.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[5] & 16)
                printf("Az Stow pin 1 fail.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[5] & 32)
                printf(" Stow pin 2 fail.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[5] & 64)
                printf(" Az Stow position 1.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[5] & 128)
                printf(" Az Stow position 2.\n");

  if (ioStatusResp.ServoPLCazimuthStatus[6] & 1)
                printf(" Az Stow pin 1 timeout.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[6] & 2)
                printf(" Stow pin 2 timeout.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[6] & 16)
                printf(" Az amplifier power cycle interlock.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[6] & 32)
                printf(" Az stop at LCP.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[6] & 64)
                printf(" Az Power On.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[6] & 128)
                printf(" Az Cam limit switch failure.\n");

  if (ioStatusResp.ServoPLCazimuthStatus[7] & 1)
                printf(" DC bus 1 failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[7] & 2)
                printf(" DC bus 2 failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[7] & 16)
                printf(" Motor fan 1 failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[7] & 32)
                printf(" Motor fan 2 failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[7] & 64)
                printf(" Motor fan 3 failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[7] & 128)
                printf(" Motor fan 4 failure.\n");

  if (ioStatusResp.ServoPLCazimuthStatus[8] & 2)
                printf(" Buffer interlock.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[8] & 4)
                printf(" Buffer pos cw/neutral.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[8] & 8)
                printf(" Buffer pos ccw/neutral.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[8] & 16)
                printf(" Az Secondary encoder failure.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[8] & 32)
                printf(" Az position < 0 deg.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[8] & 64)
                printf(" 360 deg switch.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[8] & 128)
                printf(" 720 deg switch.\n");

  if (ioStatusResp.ServoPLCazimuthStatus[9] & 1)
                printf(" Az Motor heating on.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[9] & 16)
                printf(" Line filter 1 overtemp.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[9] & 32)
                printf(" Line filter 2 overtemp.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[9] & 64)
                printf(" Line filter 3 overtemp.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[9] & 128)
                printf(" Line filter 4 overtemp.\n");

  if (ioStatusResp.ServoPLCazimuthStatus[10] & 1)
                printf(" Az Regen. resistor 1 overtemp..\n");
  if (ioStatusResp.ServoPLCazimuthStatus[10] & 2)
                printf(" Az Regen. resistor 1 overtemp..\n");
  if (ioStatusResp.ServoPLCazimuthStatus[10] & 4)
                printf(" Regen. resistor 1 overtemp..\n");
  if (ioStatusResp.ServoPLCazimuthStatus[10] & 8)
                printf(" Regen. resistor 1 overtemp..\n");
  if (ioStatusResp.ServoPLCazimuthStatus[10] & 16)
                printf(" Jack 1 overtemperature.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[10] & 32)
                printf(" Jack 2 overtemperature.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[10] & 64)
                printf(" Low level elevation.\n");
  if (ioStatusResp.ServoPLCazimuthStatus[10] & 128)
                printf(" Jack screw fault.\n");  if (ioStatusResp.ServoPLCelevationStatus[0] & 1)
                printf(" El Computer disabled.\n");
  if (ioStatusResp.ServoPLCelevationStatus[0] & 2)
                printf(" El Axis disabled.\n");
  if (ioStatusResp.ServoPLCelevationStatus[0] & 4)
                printf(" El Prelimit up.\n");
  if (ioStatusResp.ServoPLCelevationStatus[0] & 8)
                printf(" El Prelimit down.\n");
  if (ioStatusResp.ServoPLCelevationStatus[0] & 16)
                printf(" El Limit up.\n");
  if (ioStatusResp.ServoPLCelevationStatus[0] & 32)
                printf(" El Limit down.\n");
  if (ioStatusResp.ServoPLCelevationStatus[0] & 64)
                printf(" El Emergency limit.\n");
  if (ioStatusResp.ServoPLCelevationStatus[0] & 128)
                printf(" El 2nd emergency limit.\n");

  if (ioStatusResp.ServoPLCelevationStatus[1] & 1)
                printf(" El Servo failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[1] & 2)
                printf(" El Motion error.\n");
  if (ioStatusResp.ServoPLCelevationStatus[1] & 4)
                printf(" El Brake 1 failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[1] & 8)
                printf(" El Brake 2 failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[1] & 16)
                printf(" El Brake 3 failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[1] & 32)
                printf(" El Brake 4 failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[1] & 64)
                printf(" Brake warning.\n");
  if (ioStatusResp.ServoPLCelevationStatus[1] & 128)
                printf(" El Breaker failure.\n");

  if (ioStatusResp.ServoPLCelevationStatus[2] & 1)
                printf(" El Amplifier 1 failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[2] & 2)
                printf(" El Amplifier 2 failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[2] & 4)
                printf(" El Amplifier 3 failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[2] & 8)
                printf(" El Amplifier 4 failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[2] & 16)
                printf(" El Motor 1 overtemperature.\n");
  if (ioStatusResp.ServoPLCelevationStatus[2] & 32)
                printf(" El Motor 2 overtemperature.\n");
  if (ioStatusResp.ServoPLCelevationStatus[2] & 64)
                printf(" Motor 3 overtemperature.\n");
  if (ioStatusResp.ServoPLCelevationStatus[2] & 128)
                printf(" Motor 4 overtemperature.\n");

  if (ioStatusResp.ServoPLCelevationStatus[3] & 1)
                printf(" El Aux 1 mode selected.\n");
  if (ioStatusResp.ServoPLCelevationStatus[3] & 2)
                printf(" El Aux 2 mode selected.\n");
  if (ioStatusResp.ServoPLCelevationStatus[3] & 4)
                printf(" El Axis in stop.\n");
  if (ioStatusResp.ServoPLCelevationStatus[3] & 8)
                printf(" DC bus overvoltage.\n");
  if (ioStatusResp.ServoPLCelevationStatus[3] & 16)
                printf(" El Hand crank inserted.\n");
  if (ioStatusResp.ServoPLCelevationStatus[3] & 32)
                printf(" El Foot switch activated.\n");
  if (ioStatusResp.ServoPLCelevationStatus[3] & 64)
                printf(" El Overspeed.\n");
  if (ioStatusResp.ServoPLCelevationStatus[3] & 128)
                printf(" El brakes released.\n");

  if (ioStatusResp.ServoPLCelevationStatus[4] & 1)
                printf(" El Gearbox 1 low oil level.\n");
  if (ioStatusResp.ServoPLCelevationStatus[4] & 2)
                printf(" El Gearbox 2 low oil level.\n");
  if (ioStatusResp.ServoPLCelevationStatus[4] & 4)
                printf(" El Gearbox 3 low oil level.\n");
  if (ioStatusResp.ServoPLCelevationStatus[4] & 8)
                printf(" El Gearbox 4 low oil level.\n");
  if (ioStatusResp.ServoPLCelevationStatus[4] & 16)
                printf(" El Gearbox 1 heater failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[4] & 32)
                printf(" El Gearbox 2 heater failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[4] & 64)
                printf(" El Gearbox 3 heater failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[4] & 128)
                printf(" El Gearbox 4 heater failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[5] & 1)
                printf(" El Stow pin 1 inserted.\n");
  if (ioStatusResp.ServoPLCelevationStatus[5] & 2)
                printf(" Stow pin 2 inserted.\n");
  if (ioStatusResp.ServoPLCelevationStatus[5] & 4)
                printf(" El Stow pin 1 retracted.\n");
  if (ioStatusResp.ServoPLCelevationStatus[5] & 8)
                printf(" Stow pin 2 retracted.\n");
  if (ioStatusResp.ServoPLCelevationStatus[5] & 16)
                printf(" El Stow pin 1 fail.\n");
  if (ioStatusResp.ServoPLCelevationStatus[5] & 32)
                printf(" Stow pin 2 fail.\n");
  if (ioStatusResp.ServoPLCelevationStatus[5] & 64)
                printf(" El Stow position 1.\n");
  if (ioStatusResp.ServoPLCelevationStatus[5] & 128)
                printf("  Stow position 2.\n");

  if (ioStatusResp.ServoPLCelevationStatus[6] & 1)
                printf(" El Stow pin 1 timeout.\n");
  if (ioStatusResp.ServoPLCelevationStatus[6] & 2)
                printf(" Stow pin 2 timeout.\n");
  if (ioStatusResp.ServoPLCelevationStatus[6] & 16)
                printf(" El amplifier power cycle interlock.\n");
  if (ioStatusResp.ServoPLCelevationStatus[6] & 32)
                printf(" El stop at LCP.\n");
  if (ioStatusResp.ServoPLCelevationStatus[6] & 64)
                printf(" El Power On.\n");
  if (ioStatusResp.ServoPLCelevationStatus[6] & 128)
                printf(" El Cam limit switch failure.\n");

  if (ioStatusResp.ServoPLCelevationStatus[7] & 1)
                printf(" El DC bus 1 failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[7] & 2)
                printf(" El DC bus 2 failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[7] & 16)
                printf(" Motor fan 1 failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[7] & 32)
                printf(" Motor fan 2 failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[7] & 64)
                printf(" Motor fan 3 failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[7] & 128)
                printf(" Motor fan 4 failure.\n");

  if (ioStatusResp.ServoPLCelevationStatus[8] & 2)
                printf(" Buffer interlock.\n");
  if (ioStatusResp.ServoPLCelevationStatus[8] & 4)
                printf(" Buffer pos cw/neutral.\n");
  if (ioStatusResp.ServoPLCelevationStatus[8] & 8)
                printf(" Buffer pos ccw/neutral.\n");
  if (ioStatusResp.ServoPLCelevationStatus[8] & 16)
                printf(" El Secondary encoder failure.\n");
  if (ioStatusResp.ServoPLCelevationStatus[8] & 32)
                printf(" Position < 0 deg.\n");
  if (ioStatusResp.ServoPLCelevationStatus[8] & 64)
                printf(" 360 deg switch.\n");
  if (ioStatusResp.ServoPLCelevationStatus[8] & 128)
                printf(" 720 deg switch.\n");

  if (ioStatusResp.ServoPLCelevationStatus[9] & 1)
                printf(" El Motor heating on.\n");
  if (ioStatusResp.ServoPLCelevationStatus[9] & 16)
                printf(" Line filter 1 overtemp.\n");
  if (ioStatusResp.ServoPLCelevationStatus[9] & 32)
                printf(" Line filter 2 overtemp.\n");
  if (ioStatusResp.ServoPLCelevationStatus[9] & 64)
                printf(" Line filter 3 overtemp.\n");
  if (ioStatusResp.ServoPLCelevationStatus[9] & 128)
                printf(" Line filter 4 overtemp.\n");

  if (ioStatusResp.ServoPLCelevationStatus[10] & 1)
                printf(" El Regen. resistor 1 overtemp..\n");
  if (ioStatusResp.ServoPLCelevationStatus[10] & 2)
                printf(" El Regen. resistor 1 overtemp..\n");
  if (ioStatusResp.ServoPLCelevationStatus[10] & 4)
                printf(" El Regen. resistor 1 overtemp..\n");
  if (ioStatusResp.ServoPLCelevationStatus[10] & 8)
                printf(" El Regen. resistor 1 overtemp..\n");
  if (ioStatusResp.ServoPLCelevationStatus[10] & 16)
                printf(" Jack 1 overtemperature.\n");
  if (ioStatusResp.ServoPLCelevationStatus[10] & 32)
                printf(" Jack 2 overtemperature.\n");
  if (ioStatusResp.ServoPLCelevationStatus[10] & 64)
                printf(" Low level elevation.\n");
  if (ioStatusResp.ServoPLCelevationStatus[10] & 128)
                printf(" Jack screw fault.\n");
  if (ioStatusResp.ServoAmpAz1status[1] & 1)
                printf(" Az Amp 1 overvoltage.\n");
  if (ioStatusResp.ServoAmpAz1status[1] & 2)
                printf(" Az Amp 1 low voltage\n");
  if (ioStatusResp.ServoAmpAz1status[1] & 4)
                printf(" Az Amp 1 current limit\n");
  if (ioStatusResp.ServoAmpAz1status[1] & 8)
                printf(" Az Amp 1 motor overload.\n");
  if (ioStatusResp.ServoAmpAz1status[1] & 16)
                printf(" Az Amp 1 warning.\n");
  if (ioStatusResp.ServoAmpAz1status[1] & 32)
                printf(" Az Amp 1 error msg available.\n");
  if (ioStatusResp.ServoAmpAz1status[1] & 64)
                printf(" Az Amp 1 DIGIN2: mode-  manual master\n");
  if (ioStatusResp.ServoAmpAz1status[1] & 128)
                printf(" Az Amp 1 DIGIN2: mode- computer\n");

  if (ioStatusResp.ServoAmpAz2status[1] & 1)
                printf(" Az Amp 2 overvoltage.\n");
  if (ioStatusResp.ServoAmpAz2status[1] & 2)
                printf(" Az Amp 2 low voltage\n");
  if (ioStatusResp.ServoAmpAz2status[1] & 4)
                printf(" Az Amp 2 current limit\n");
  if (ioStatusResp.ServoAmpAz2status[1] & 8)
                printf(" Az Amp 2 motor overload.\n");
  if (ioStatusResp.ServoAmpAz2status[1] & 16)
                printf(" Az Amp 2 warning.\n");
  if (ioStatusResp.ServoAmpAz2status[1] & 32)
                printf(" Az Amp 2 error msg available.\n");
  if (ioStatusResp.ServoAmpAz2status[1] & 64)
                printf(" Az Amp 2 DIGIN2: mode-  manual master\n");
  if (ioStatusResp.ServoAmpAz2status[1] & 128)
                printf(" Az Amp 2 DIGIN2: mode- computer\n");

  if (ioStatusResp.ServoAmpEl1status[1] & 1)
                printf(" El Amp 1 overvoltage.\n");
  if (ioStatusResp.ServoAmpEl1status[1] & 2)
                printf(" El Amp 1 low voltage\n");
  if (ioStatusResp.ServoAmpEl1status[1] & 4)
                printf(" El Amp 1 current limit\n");
  if (ioStatusResp.ServoAmpEl1status[1] & 8)
                printf(" El Amp 1 motor overload.\n");
  if (ioStatusResp.ServoAmpEl1status[1] & 16)
                printf(" El Amp 1 warning.\n");
  if (ioStatusResp.ServoAmpEl1status[1] & 32)
                printf(" El Amp 1 error msg available.\n");
  if (ioStatusResp.ServoAmpEl1status[1] & 64)
                printf(" El Amp 1 DIGIN2: mode-  manual master\n");
  if (ioStatusResp.ServoAmpEl1status[1] & 128)
                printf(" El Amp 1 DIGIN2: mode- computer\n");

  if (ioStatusResp.ServoAmpEl2status[1] & 1)
                printf(" El Amp 2 overvoltage.\n");
  if (ioStatusResp.ServoAmpEl2status[1] & 2)
                printf(" El Amp 2 low voltage\n");
  if (ioStatusResp.ServoAmpEl2status[1] & 4)
                printf(" El Amp 2 current limit\n");
  if (ioStatusResp.ServoAmpEl2status[1] & 8)
                printf(" El Amp 2 motor overload.\n");
  if (ioStatusResp.ServoAmpEl2status[1] & 16)
                printf(" El Amp 2 warning.\n");
  if (ioStatusResp.ServoAmpEl2status[1] & 32)
                printf(" El Amp 2 error msg available.\n");
  if (ioStatusResp.ServoAmpEl2status[1] & 64)
                printf(" El Amp 2 DIGIN2: mode-  manual master\n");
  if (ioStatusResp.ServoAmpEl2status[1] & 128)
                printf(" El Amp 2 DIGIN2: mode- computer\n");
  if (ioStatusResp.ServoAmpEl3status[1] & 1)
                printf(" El Amp 3 overvoltage.\n");
  if (ioStatusResp.ServoAmpEl3status[1] & 2)
                printf(" El Amp 3 low voltage\n");
  if (ioStatusResp.ServoAmpEl3status[1] & 4)
                printf(" El Amp 3 current limit\n");
  if (ioStatusResp.ServoAmpEl3status[1] & 8)
                printf(" El Amp 3 motor overload.\n");
  if (ioStatusResp.ServoAmpEl3status[1] & 16)
                printf(" El Amp 3 warning.\n");
  if (ioStatusResp.ServoAmpEl3status[1] & 32)
                printf(" El Amp 3 error msg available.\n");
  if (ioStatusResp.ServoAmpEl3status[1] & 64)
                printf(" El Amp 3 DIGIN2: mode-  manual master\n");
  if (ioStatusResp.ServoAmpEl3status[1] & 128)
                printf(" El Amp 3 DIGIN2: mode- computer\n");

  if (ioStatusResp.ServoAmpEl4status[1] & 1)
                printf(" El Amp 4 overvoltage.\n");
  if (ioStatusResp.ServoAmpEl4status[1] & 2)
                printf(" El Amp 4 low voltage\n");
  if (ioStatusResp.ServoAmpEl4status[1] & 4)
                printf(" El Amp 4 current limit\n");
  if (ioStatusResp.ServoAmpEl4status[1] & 8)
                printf(" El Amp 4 motor overload.\n");
  if (ioStatusResp.ServoAmpEl4status[1] & 16)
                printf(" El Amp 4 warning.\n");
  if (ioStatusResp.ServoAmpEl4status[1] & 32)
                printf(" El Amp 4 error msg available.\n");
  if (ioStatusResp.ServoAmpEl4status[1] & 64)
                printf(" El Amp 4 DIGIN2: mode-  manual master\n");
  if (ioStatusResp.ServoAmpEl4status[1] & 128)
                printf(" El Amp 4 DIGIN2: mode- computer\n");

            azmotor1temp = ioStatusResp.ServoAmpAz1status[0] - 100;
            azmotor2temp = ioStatusResp.ServoAmpAz2status[0] - 100;
            elmotor1temp = ioStatusResp.ServoAmpEl1status[0] - 100;
            elmotor2temp = ioStatusResp.ServoAmpEl2status[0] - 100;
            elmotor3temp = ioStatusResp.ServoAmpEl3status[0] - 100;
            elmotor4temp = ioStatusResp.ServoAmpEl4status[0] - 100;

            az1motorcurrent = ioStatusResp.Az1motorCurrent;
            az2motorcurrent = ioStatusResp.Az2motorCurrent;
            el1motorcurrent = ioStatusResp.El1motorCurrent;
            el2motorcurrent = ioStatusResp.El2motorCurrent;
            el3motorcurrent = ioStatusResp.El3motorCurrent;
            el4motorcurrent = ioStatusResp.El4motorCurrent;

            az1motorcurrentF = az1motorcurrent / 10.0;
            az2motorcurrentF = az2motorcurrent / 10.0;
            el1motorcurrentF = el1motorcurrent / 10.0;
            el2motorcurrentF = el2motorcurrent / 10.0;
            el3motorcurrentF = el3motorcurrent / 10.0;
            el4motorcurrentF = el4motorcurrent / 10.0;

            printf("Az motor temps (C): %d %d\n", azmotor1temp, azmotor2temp);
            printf("El motor temps (C): %d %d %d %d\n", elmotor1temp, elmotor2temp, elmotor3temp,elmotor4temp);


            printf("Motor currents: %.1f %.1f %.1f %.1f\n",
                   az1motorcurrentF, az2motorcurrentF,
                   el1motorcurrentF, el2motorcurrentF,
                   el3motorcurrentF, el4motorcurrentF);

            printf("Servo PLC software version: %c%c%c\n",
                   ioStatusResp.ServoPLCSoftwareVer[0],
                   ioStatusResp.ServoPLCSoftwareVer[1],
                   ioStatusResp.ServoPLCSoftwareVer[2]);

            printf("ACU software version: %c%c%c%c\n",
                   ioStatusResp.ACUsoftwareVer[0],
                   ioStatusResp.ACUsoftwareVer[1],
                   ioStatusResp.ACUsoftwareVer[2],
                   ioStatusResp.ACUsoftwareVer[3]);

            printf("ACU LUI software version: %c%c%c%c\n",
                   ioStatusResp.ACUluiSoftwareVer[0],
                   ioStatusResp.ACUluiSoftwareVer[1],
                   ioStatusResp.ACUluiSoftwareVer[2],
                   ioStatusResp.ACUluiSoftwareVer[3]);
        }
    } else {
        printf("ACU refuses the command: Reason code 0x%x\n", recvBuff[1]);
  if (recvBuff[1]==0x43) printf("Checksum error.\n");
  if (recvBuff[1]==0x45) printf("ETX not received at expected position.\n");
  if (recvBuff[1]==0x49) printf("Unknown identifier.\n");
  if (recvBuff[1]==0x4C) printf("Wrong length (incorrect no. of bytes rcvd.\n");
  if (recvBuff[1]==0x6C) printf("Specified length does not match identifier.\n");
  if (recvBuff[1]==0x4D) printf("Command ignored in present operating mode.\n");
  if (recvBuff[1]==0x6F) printf("Other reasons.\n");
  if (recvBuff[1]==0x52) printf("Device not in Remote mode.\n");
  if (recvBuff[1]==0x72) printf("Value out of range.\n");
  if (recvBuff[1]==0x53) printf("Missing STX.\n");

    }

    close(sockfd);
    return 0;
}
