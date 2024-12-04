/* program for fast reading of encoders with timestamp*/
/* N. A. Patel, 10 September 2018 */
/* added tiltmeter readings for fast readout of tiltmeters. ACU firmware 1.0.7
21 May 2021 */
/* Initially this program did not work due to errors in ACU firmware configuration.
With help from Dirk Gerdes from Vertex, visiting at the site, and debugging using wireshark, 
we got the UDP broadcast to work. 30 Oct 2018 NAP */

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

#define BUFLEN 1024
#define PORT 8200

#define MICRODEG_TO_ARCSEC  0.0036

void die(char *s)
{
    perror(s);
    exit(1);
}

 
int main(void)
{
  int sockfd = 0,n = 0,len;
  char recvBuff[1024];
  char sendBuff[1024];
  int days,hh,mm,sockAddrSize;
  double hours,minutes,seconds;
  struct sockaddr_in serv_addr;
  encoderData acuEncodersResp;


  memset(recvBuff, '0' ,sizeof(recvBuff));
  memset(sendBuff, '0' ,sizeof(sendBuff));

  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0))< 0) {
     die("Socket error");
    }
  
  sockAddrSize = sizeof(struct sockaddr_in);
 
  memset((char *) &serv_addr, 0, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(sockfd,(struct sockaddr *)&serv_addr, sockAddrSize) == -1)
    {
    close(sockfd);
    printf("bind error\n");
    }


  while(1) {

  memset(recvBuff,'\0',BUFLEN);
 
 
  n = recvfrom(sockfd, (char *)&acuEncodersResp, sizeof(acuEncodersResp),0,(struct sockaddr *)&serv_addr,&len);

  if( n < 0)  die("recvfrom()");


/*
  printf ("Received %d bytes from ACU.\n",acuEncodersResp.datalength);
*/


  hours = acuEncodersResp.timeOfDay1/3600000.;
  hh = (int)hours;
  minutes = (hours-hh)*60.; 
  mm = (int) minutes;
  seconds = (minutes-mm)*60.;
  printf ("Time: (day, hh:mm:ss.sss):  %d %02d:%02d:%02.3f ",acuEncodersResp.dayOfYear1,hh,mm,seconds);
  printf ("azPos (deg): %f ",(double)acuEncodersResp.azPos1/1.0e6);
  printf ("elPos (deg): %f ",(double)acuEncodersResp.elPos1/1.0e6);
  printf ("tilt1x1 (arcsec): %f ",(double)acuEncodersResp.tilt1x1*MICRODEG_TO_ARCSEC);
  printf ("tilt1y1 (arcsec): %f ",(double)acuEncodersResp.tilt1y1*MICRODEG_TO_ARCSEC);
  printf ("tilt2x1 (arcsec): %f ",(double)acuEncodersResp.tilt2x1*MICRODEG_TO_ARCSEC);
  printf ("tilt2y1 (arcsec): %f ",(double)acuEncodersResp.tilt2y1*MICRODEG_TO_ARCSEC);
  printf ("tilt3x1 (arcsec): %f ",(double)acuEncodersResp.tilt3x1*MICRODEG_TO_ARCSEC);
  printf ("tilt3y1 (arcsec): %f \n",(double)acuEncodersResp.tilt3y1*MICRODEG_TO_ARCSEC);

  hours = acuEncodersResp.timeOfDay2/3600000.;
  hh = (int)hours;
  minutes = (hours-hh)*60.; 
  mm = (int) minutes;
  seconds = (minutes-mm)*60.;
  printf ("Time: (day, hh:mm:ss.sss):  %d %02d:%02d:%02.3f ",acuEncodersResp.dayOfYear2,hh,mm,seconds);
  printf ("azPos (deg): %f ",(double)acuEncodersResp.azPos2/1.0e6);
  printf ("elPos (deg): %f ",(double)acuEncodersResp.elPos2/1.0e6);
  printf ("tilt1x2 (arcsec): %f ",(double)acuEncodersResp.tilt1x2*MICRODEG_TO_ARCSEC);
  printf ("tilt1y2 (arcsec): %f ",(double)acuEncodersResp.tilt1y2*MICRODEG_TO_ARCSEC);
  printf ("tilt2x2 (arcsec): %f ",(double)acuEncodersResp.tilt2x2*MICRODEG_TO_ARCSEC);
  printf ("tilt2y2 (arcsec): %f ",(double)acuEncodersResp.tilt2y2*MICRODEG_TO_ARCSEC);
  printf ("tilt3x2 (arcsec): %f ",(double)acuEncodersResp.tilt3x2*MICRODEG_TO_ARCSEC);
  printf ("tilt3y2 (arcsec): %f \n",(double)acuEncodersResp.tilt3y2*MICRODEG_TO_ARCSEC);

  hours = acuEncodersResp.timeOfDay3/3600000.;
  hh = (int)hours;
  minutes = (hours-hh)*60.; 
  mm = (int) minutes;
  seconds = (minutes-mm)*60.;
  printf ("Time: (day, hh:mm:ss.sss):  %d %02d:%02d:%02.3f ",acuEncodersResp.dayOfYear3,hh,mm,seconds);
  printf ("azPos (deg): %f ",(double)acuEncodersResp.azPos3/1.0e6);
  printf ("elPos (deg): %f ",(double)acuEncodersResp.elPos3/1.0e6);
  printf ("tilt1x3 (arcsec): %f ",(double)acuEncodersResp.tilt1x3*MICRODEG_TO_ARCSEC);
  printf ("tilt1y3 (arcsec): %f ",(double)acuEncodersResp.tilt1y3*MICRODEG_TO_ARCSEC);
  printf ("tilt2x3 (arcsec): %f ",(double)acuEncodersResp.tilt2x3*MICRODEG_TO_ARCSEC);
  printf ("tilt2y3 (arcsec): %f ",(double)acuEncodersResp.tilt2y3*MICRODEG_TO_ARCSEC);
  printf ("tilt3x3 (arcsec): %f ",(double)acuEncodersResp.tilt3x3*MICRODEG_TO_ARCSEC);
  printf ("tilt3y3 (arcsec): %f \n",(double)acuEncodersResp.tilt3y3*MICRODEG_TO_ARCSEC);

  hours = acuEncodersResp.timeOfDay4/3600000.;
  hh = (int)hours;
  minutes = (hours-hh)*60.; 
  mm = (int) minutes;
  seconds = (minutes-mm)*60.;
  printf ("Time: (day, hh:mm:ss.sss):  %d %02d:%02d:%02.3f ",acuEncodersResp.dayOfYear4,hh,mm,seconds);
  printf ("azPos (deg): %f ",(double)acuEncodersResp.azPos4/1.0e6);
  printf ("elPos (deg): %f ",(double)acuEncodersResp.elPos4/1.0e6);
  printf ("tilt1x4 (arcsec): %f ",(double)acuEncodersResp.tilt1x4*MICRODEG_TO_ARCSEC);
  printf ("tilt1y4 (arcsec): %f ",(double)acuEncodersResp.tilt1y4*MICRODEG_TO_ARCSEC);
  printf ("tilt2x4 (arcsec): %f ",(double)acuEncodersResp.tilt2x4*MICRODEG_TO_ARCSEC);
  printf ("tilt2y4 (arcsec): %f ",(double)acuEncodersResp.tilt2y4*MICRODEG_TO_ARCSEC);
  printf ("tilt3x4 (arcsec): %f ",(double)acuEncodersResp.tilt3x4*MICRODEG_TO_ARCSEC);
  printf ("tilt3y4 (arcsec): %f \n",(double)acuEncodersResp.tilt3y4*MICRODEG_TO_ARCSEC);

  hours = acuEncodersResp.timeOfDay5/3600000.;
  hh = (int)hours;
  minutes = (hours-hh)*60.; 
  mm = (int) minutes;
  seconds = (minutes-mm)*60.;
  printf ("Time: (day, hh:mm:ss.sss):  %d %02d:%02d:%02.3f ",acuEncodersResp.dayOfYear5,hh,mm,seconds);
  printf ("azPos (deg): %f ",(double)acuEncodersResp.azPos5/1.0e6);
  printf ("elPos (deg): %f ",(double)acuEncodersResp.elPos5/1.0e6);
  printf ("tilt1x5 (arcsec): %f ",(double)acuEncodersResp.tilt1x5*MICRODEG_TO_ARCSEC);
  printf ("tilt1y5 (arcsec): %f ",(double)acuEncodersResp.tilt1y5*MICRODEG_TO_ARCSEC);
  printf ("tilt2x5 (arcsec): %f ",(double)acuEncodersResp.tilt2x5*MICRODEG_TO_ARCSEC);
  printf ("tilt2y5 (arcsec): %f ",(double)acuEncodersResp.tilt2y5*MICRODEG_TO_ARCSEC);
  printf ("tilt3x5 (arcsec): %f ",(double)acuEncodersResp.tilt3x5*MICRODEG_TO_ARCSEC);
  printf ("tilt3y5 (arcsec): %f \n",(double)acuEncodersResp.tilt3y5*MICRODEG_TO_ARCSEC);

  hours = acuEncodersResp.timeOfDay6/3600000.;
  hh = (int)hours;
  minutes = (hours-hh)*60.; 
  mm = (int) minutes;
  seconds = (minutes-mm)*60.;
  printf ("Time: (day, hh:mm:ss.sss):  %d %02d:%02d:%02.3f ",acuEncodersResp.dayOfYear6,hh,mm,seconds);
  printf ("azPos (deg): %f ",(double)acuEncodersResp.azPos6/1.0e6);
  printf ("elPos (deg): %f ",(double)acuEncodersResp.elPos6/1.0e6);
  printf ("tilt1x6 (arcsec): %f ",(double)acuEncodersResp.tilt1x6*MICRODEG_TO_ARCSEC);
  printf ("tilt1y6 (arcsec): %f ",(double)acuEncodersResp.tilt1y6*MICRODEG_TO_ARCSEC);
  printf ("tilt2x6 (arcsec): %f ",(double)acuEncodersResp.tilt2x6*MICRODEG_TO_ARCSEC);
  printf ("tilt2y6 (arcsec): %f ",(double)acuEncodersResp.tilt2y6*MICRODEG_TO_ARCSEC);
  printf ("tilt3x6 (arcsec): %f ",(double)acuEncodersResp.tilt3x6*MICRODEG_TO_ARCSEC);
  printf ("tilt3y6 (arcsec): %f \n",(double)acuEncodersResp.tilt3y6*MICRODEG_TO_ARCSEC);

  hours = acuEncodersResp.timeOfDay7/3600000.;
  hh = (int)hours;
  minutes = (hours-hh)*60.; 
  mm = (int) minutes;
  seconds = (minutes-mm)*60.;
  printf ("Time: (day, hh:mm:ss.sss):  %d %02d:%02d:%02.3f ",acuEncodersResp.dayOfYear7,hh,mm,seconds);
  printf ("azPos (deg): %f ",(double)acuEncodersResp.azPos7/1.0e6);
  printf ("elPos (deg): %f ",(double)acuEncodersResp.elPos7/1.0e6);
  printf ("tilt1x7 (arcsec): %f ",(double)acuEncodersResp.tilt1x7*MICRODEG_TO_ARCSEC);
  printf ("tilt1y7 (arcsec): %f ",(double)acuEncodersResp.tilt1y7*MICRODEG_TO_ARCSEC);
  printf ("tilt2x7 (arcsec): %f ",(double)acuEncodersResp.tilt2x7*MICRODEG_TO_ARCSEC);
  printf ("tilt2y7 (arcsec): %f ",(double)acuEncodersResp.tilt2y7*MICRODEG_TO_ARCSEC);
  printf ("tilt3x7 (arcsec): %f ",(double)acuEncodersResp.tilt3x7*MICRODEG_TO_ARCSEC);
  printf ("tilt3y7 (arcsec): %f \n",(double)acuEncodersResp.tilt3y7*MICRODEG_TO_ARCSEC);

  hours = acuEncodersResp.timeOfDay8/3600000.;
  hh = (int)hours;
  minutes = (hours-hh)*60.; 
  mm = (int) minutes;
  seconds = (minutes-mm)*60.;
  printf ("Time: (day, hh:mm:ss.sss):  %d %02d:%02d:%02.3f ",acuEncodersResp.dayOfYear8,hh,mm,seconds);
  printf ("azPos (deg): %f ",(double)acuEncodersResp.azPos8/1.0e6);
  printf ("elPos (deg): %f ",(double)acuEncodersResp.elPos8/1.0e6);
  printf ("tilt1x8 (arcsec): %f ",(double)acuEncodersResp.tilt1x8*MICRODEG_TO_ARCSEC);
  printf ("tilt1y8 (arcsec): %f ",(double)acuEncodersResp.tilt1y8*MICRODEG_TO_ARCSEC);
  printf ("tilt2x8 (arcsec): %f ",(double)acuEncodersResp.tilt2x8*MICRODEG_TO_ARCSEC);
  printf ("tilt2y8 (arcsec): %f ",(double)acuEncodersResp.tilt2y8*MICRODEG_TO_ARCSEC);
  printf ("tilt3x8 (arcsec): %f ",(double)acuEncodersResp.tilt3x8*MICRODEG_TO_ARCSEC);
  printf ("tilt3y8 (arcsec): %f \n",(double)acuEncodersResp.tilt3y8*MICRODEG_TO_ARCSEC);

  hours = acuEncodersResp.timeOfDay9/3600000.;
  hh = (int)hours;
  minutes = (hours-hh)*60.; 
  mm = (int) minutes;
  seconds = (minutes-mm)*60.;
  printf ("Time: (day, hh:mm:ss.sss):  %d %02d:%02d:%02.3f ",acuEncodersResp.dayOfYear9,hh,mm,seconds);
  printf ("azPos (deg): %f ",(double)acuEncodersResp.azPos9/1.0e6);
  printf ("elPos (deg): %f ",(double)acuEncodersResp.elPos9/1.0e6);
  printf ("tilt1x9 (arcsec): %f ",(double)acuEncodersResp.tilt1x9*MICRODEG_TO_ARCSEC);
  printf ("tilt1y9 (arcsec): %f ",(double)acuEncodersResp.tilt1y9*MICRODEG_TO_ARCSEC);
  printf ("tilt2x9 (arcsec): %f ",(double)acuEncodersResp.tilt2x9*MICRODEG_TO_ARCSEC);
  printf ("tilt2y9 (arcsec): %f ",(double)acuEncodersResp.tilt2y9*MICRODEG_TO_ARCSEC);
  printf ("tilt3x9 (arcsec): %f ",(double)acuEncodersResp.tilt3x9*MICRODEG_TO_ARCSEC);
  printf ("tilt3y9 (arcsec): %f \n",(double)acuEncodersResp.tilt3y9*MICRODEG_TO_ARCSEC);

  hours = acuEncodersResp.timeOfDay10/3600000.;
  hh = (int)hours;
  minutes = (hours-hh)*60.; 
  mm = (int) minutes;
  seconds = (minutes-mm)*60.;
  printf ("Time: (day, hh:mm:ss.sss):  %d %02d:%02d:%02.3f ",acuEncodersResp.dayOfYear10,hh,mm,seconds);
  printf ("azPos (deg): %f ",(double)acuEncodersResp.azPos10/1.0e6);
  printf ("elPos (deg): %f ",(double)acuEncodersResp.elPos10/1.0e6);
  printf ("tilt1x10 (arcsec): %f ",(double)acuEncodersResp.tilt1x10*MICRODEG_TO_ARCSEC);
  printf ("tilt1y10 (arcsec): %f ",(double)acuEncodersResp.tilt1y10*MICRODEG_TO_ARCSEC);
  printf ("tilt2x10 (arcsec): %f ",(double)acuEncodersResp.tilt2x10*MICRODEG_TO_ARCSEC);
  printf ("tilt2y10 (arcsec): %f ",(double)acuEncodersResp.tilt2y10*MICRODEG_TO_ARCSEC);
  printf ("tilt3x10 (arcsec): %f ",(double)acuEncodersResp.tilt3x10*MICRODEG_TO_ARCSEC);
  printf ("tilt3y10 (arcsec): %f \n",(double)acuEncodersResp.tilt3y10*MICRODEG_TO_ARCSEC);

/*
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
*/
  }

  close(sockfd);
 
  return 0;
}
