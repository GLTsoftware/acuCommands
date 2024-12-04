/* reads az el encoder readings with time stamp from ACU*/
/* N. A. Patel, 15 Sep 2018*/

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
  char recvBuff[1024];
  char sendBuff[1024];
  int days,hh,mm;
  double hours,minutes,seconds;
  struct sockaddr_in serv_addr;
  acuStatus acuStatusResp;
  acuCmd acuCommand;

  acuCommand.stx = 0x2;
  acuCommand.id = 0x71;
  acuCommand.datalength = 0x7;
  acuCommand.checksum = 0x78;
  acuCommand.etx = 0x3;

  memset(recvBuff, '0' ,sizeof(recvBuff));
  memset(sendBuff, '0' ,sizeof(sendBuff));

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0) {
      printf("\n Error : Could not create socket \n");
      return 1;
    }
 
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(9110);
  serv_addr.sin_addr.s_addr = inet_addr("192.168.1.103");
 
  if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) {
      printf("\n Error : Connect Failed \n");
      return 1;
    }

  memcpy(sendBuff,(char*)&acuCommand,sizeof(acuCommand));
  n = send(sockfd,sendBuff,sizeof(acuCommand),0);
  if (n<0) printf("ERROR writing to ACU.");
 
  /* receive the ACK response from ACU */
  n = recv(sockfd, recvBuff, sizeof(acuStatusResp),0); 

  if( n < 0)  printf("\n Read Error \n"); 

  /* check if ACK is received, then receive the response and parse it */
  if (recvBuff[0]==0x6) {

  n = recv(sockfd, (char *)&acuStatusResp, sizeof(acuStatusResp),0);

/*
  printf("Read %d bytes from ACU\n",n);
  printf ("Received %d bytes from ACU.\n",acuStatusResp.datalength);
*/

  hours = acuStatusResp.timeOfDay/3600000.;
  hh = (int)hours;
  minutes = (hours-hh)*60.; 
  mm = (int) minutes;
  seconds = (minutes-mm)*60.;
  printf ("%d %d %02d:%02d:%02.3f %f %f\n",acuStatusResp.timeOfDay,
              acuStatusResp.dayOfYear,hh,mm,seconds,
              (double)acuStatusResp.azPos/1.0e6,
              (double)acuStatusResp.elPos/1.0e6);
  }

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
