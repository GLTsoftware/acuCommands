/* Metrology data */
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

#include "acuData.h"
#include "acuCommands.h"
 
int main(void)
{
  int sockfd = 0,n = 0;
  char recvBuff[256];
  char sendBuff[256];
  struct sockaddr_in serv_addr;
  metrologyData metrologyStatusResp;
  acuCmd acuCommand;

  acuCommand.stx = 0x2;
  acuCommand.id = 0x76; /* v for metrology; 4.1.4.1, p35 */
  acuCommand.datalength = 0x7;
  acuCommand.checksum = 0x7D;
  acuCommand.etx = 0x3;

  memset(recvBuff, '0' ,sizeof(recvBuff));
  memset(sendBuff, '0' ,sizeof(sendBuff));

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0) {
      printf("\n Error : Could not create socket \n");
      return 1;
    }
 
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(9010);
  serv_addr.sin_addr.s_addr = inet_addr("172.16.5.95");
 
  if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) {
      printf("\n Error : Connect Failed \n");
      return 1;
    }

  memcpy(sendBuff,(char*)&acuCommand,sizeof(acuCommand));
  n = send(sockfd,sendBuff,sizeof(acuCommand),0);
  if (n<0) printf("ERROR writing to ACU.");
  printf("Wrote %d bytes to ACU\n",n);
 
  /* receive the ACK response from ACU */
  n = recv(sockfd, recvBuff, sizeof(metrologyStatusResp),0); 
  printf("Received:  0x%x 0x%x from ACU\n",recvBuff[0],recvBuff[1]);

  if( n < 0)  printf("\n Read Error \n"); 

  /* check if ACK is received, then receive the response and parse it */
  if (recvBuff[0]==0x6) {

  n = recv(sockfd, (char *)&metrologyStatusResp, sizeof(metrologyStatusResp),0);

  printf("Read %d bytes from ACU\n",n);
  printf ("Received %d bytes from ACU.\n",metrologyStatusResp.datalength);
  printf ("Received ID from ACU: 0x%x\n",metrologyStatusResp.id);
  printf ("General HVAC/metrology status byte 1-4: 0x%x 0x%x 0x%x 0x%x \n",metrologyStatusResp.GeneralMetrologyStatus[0],
metrologyStatusResp.GeneralMetrologyStatus[1],
metrologyStatusResp.GeneralMetrologyStatus[2],
metrologyStatusResp.GeneralMetrologyStatus[3]);

     if(metrologyStatusResp.GeneralMetrologyStatus[0] & 1)
         printf("Bus station support cone Profibus error.\n");
     if(metrologyStatusResp.GeneralMetrologyStatus[0] & 2)
         printf("Bus station yoke left Profibus error.\n");
     if(metrologyStatusResp.GeneralMetrologyStatus[0] & 4)
         printf("Bus station yoke right Profibus error.\n");
     if(metrologyStatusResp.GeneralMetrologyStatus[0] & 8)
         printf("Bus station receiver cabin Profibus error.\n");
     if(metrologyStatusResp.GeneralMetrologyStatus[0] & 16)
         printf("Bus station support cone breaker error.\n");
     if(metrologyStatusResp.GeneralMetrologyStatus[0] & 32)
         printf("Bus station yoke left breaker error.\n");
     if(metrologyStatusResp.GeneralMetrologyStatus[0] & 64)
         printf("Bus station yoke right breaker error.\n");
     if(metrologyStatusResp.GeneralMetrologyStatus[0] & 128)
         printf("Bus station receiver cabin breaker error.\n");

     if(metrologyStatusResp.GeneralMetrologyStatus[1] & 1)
         printf("Tiltmeter 1 readout error.\n");
     if(metrologyStatusResp.GeneralMetrologyStatus[1] & 2)
         printf("Tiltmeter 2 readout error.\n");
     if(metrologyStatusResp.GeneralMetrologyStatus[1] & 4)
         printf("Linear sensor 1 readout error.\n");
     if(metrologyStatusResp.GeneralMetrologyStatus[1] & 8)
         printf("Linear sensor 2 readout error.\n");
     if(metrologyStatusResp.GeneralMetrologyStatus[1] & 16)
         printf("Linear sensor 3 readout error.\n");
     if(metrologyStatusResp.GeneralMetrologyStatus[1] & 32)
         printf("Linear sensor 4 readout error.\n");

  printf ("SPEMazCorr: %d \n",metrologyStatusResp.SPEMazCorr);
  printf ("SPEMelCorr: %d \n",metrologyStatusResp.SPEMelCorr);

  printf ("linearSensor1: %d \n",metrologyStatusResp.linearSensor1);
  printf ("linearSensor2: %d \n",metrologyStatusResp.linearSensor2);
  printf ("linearSensor3: %d \n",metrologyStatusResp.linearSensor3);
  printf ("linearSensor4: %d \n",metrologyStatusResp.linearSensor4);

  printf ("Tilt1x: %d Tilt1y: %d Tilt1temp: %d \n",
                       metrologyStatusResp.tilt1x,
                       metrologyStatusResp.tilt1y,
                       metrologyStatusResp.tilt1Temp);

  printf ("Tilt2x: %d Tilt2y: %d Tilt2temp: %d \n",
                       metrologyStatusResp.tilt2x,
                       metrologyStatusResp.tilt2y,
                       metrologyStatusResp.tilt2Temp);

  printf ("Tilt3x: %d Tilt3y: %d Tilt3temp: %d \n",
                       metrologyStatusResp.tilt3x,
                       metrologyStatusResp.tilt3y,
                       metrologyStatusResp.tilt3Temp);

  }
  printf("Temperature sensors: 1, 2, 3, ... 7\n");
  printf("                     8, 9, 10, ... etc.\n\n");

 /* tempSensor should be 0.01 */

  printf("%d %d %d %d %d %d %d\n",
                  metrologyStatusResp.tempSensor[0],
                  metrologyStatusResp.tempSensor[1],
                  metrologyStatusResp.tempSensor[2],
                  metrologyStatusResp.tempSensor[3],
                  metrologyStatusResp.tempSensor[4],
                  metrologyStatusResp.tempSensor[5],
                  metrologyStatusResp.tempSensor[6]);

  printf("%d %d %d %d %d %d %d\n",
                  metrologyStatusResp.tempSensor[7],
                  metrologyStatusResp.tempSensor[8],
                  metrologyStatusResp.tempSensor[9],
                  metrologyStatusResp.tempSensor[10],
                  metrologyStatusResp.tempSensor[11],
                  metrologyStatusResp.tempSensor[12],
                  metrologyStatusResp.tempSensor[13]);

  printf("%d %d %d %d %d %d %d\n",
                  metrologyStatusResp.tempSensor[14],
                  metrologyStatusResp.tempSensor[15],
                  metrologyStatusResp.tempSensor[16],
                  metrologyStatusResp.tempSensor[17],
                  metrologyStatusResp.tempSensor[18],
                  metrologyStatusResp.tempSensor[19],
                  metrologyStatusResp.tempSensor[20]);

  printf("%d %d %d %d %d %d %d\n",
                  metrologyStatusResp.tempSensor[21],
                  metrologyStatusResp.tempSensor[22],
                  metrologyStatusResp.tempSensor[23],
                  metrologyStatusResp.tempSensor[24],
                  metrologyStatusResp.tempSensor[25],
                  metrologyStatusResp.tempSensor[26],
                  metrologyStatusResp.tempSensor[27]);

  printf("%d %d %d %d %d %d %d\n",
                  metrologyStatusResp.tempSensor[28],
                  metrologyStatusResp.tempSensor[29],
                  metrologyStatusResp.tempSensor[30],
                  metrologyStatusResp.tempSensor[31],
                  metrologyStatusResp.tempSensor[32],
                  metrologyStatusResp.tempSensor[33],
                  metrologyStatusResp.tempSensor[34]);


  if (recvBuff[0]==0x2) {
  printf("ACU refuses the command...reason:");
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

