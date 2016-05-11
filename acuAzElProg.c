/* Az El Program Track */

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

short checkSum (char *buff, int size);
 
int main(int argc, char *argv[])
{
  FILE *fpin;
  int sockfd = 0,n = 0;
  char recvBuff[256];
  char sendBuff[256];
  struct sockaddr_in serv_addr;
  acuStatus acuStatusResp;
  acuCmd acuCommand;
  acuAzElProg acuAzElProgCommand;
  short checksum,acuDayOfYear;
  int acuTimeOfDay;
  int trackTime,newtime,timeForward=30000;
  int i,az,el;
  short clearstack;

    /* timeForward is the time step of 30 seconds to be added
       to track time from file, to have positions in future */

  /* reading the time (ms), az(microdeg), el(microdeg) from 
     a file: track.dat */
  fpin=fopen("track.dat","r");
  if(fpin==NULL) {
  printf("track.dat file could not be opened.\n");
  exit(0);
  }

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


/* get day of year from ACU and add it to struct acuAzElProgCommand.dayOfYear*/
/* get time of day from ACU */
  acuCommand.stx = 0x2;
  acuCommand.id = 0x71;
  acuCommand.datalength = 0x7;
  acuCommand.checksum = 0x78;
  acuCommand.etx = 0x3;

  memcpy(sendBuff,(char*)&acuCommand,sizeof(acuCommand));
  n = send(sockfd,sendBuff,sizeof(acuCommand),0);
  if (n<0) printf("ERROR writing to ACU.");
  printf("Wrote %d bytes to ACU\n",n);

  /* receive the ACK response from ACU */
  n = recv(sockfd, recvBuff, sizeof(acuStatusResp),0);

       if( n < 0)  printf("\n Read Error \n");

  if (recvBuff[0]==0x6) {
  n = recv(sockfd, (char *)&acuStatusResp, sizeof(acuStatusResp),0);

       acuTimeOfDay = acuStatusResp.timeOfDay;
       acuDayOfYear = acuStatusResp.dayOfYear;
  }

  /* now pass each az/el value to acu with newtime stamps */
  acuAzElProgCommand.stx = 0x2;
  acuAzElProgCommand.id = 0x4F; /* page 16 of ICD section 4.1.1.4  O cmd */
  acuAzElProgCommand.datalength = 0x17;

  acuAzElProgCommand.etx = 0x3;

  i=0;
  while((fscanf(fpin,"%d %d %d",&trackTime,&az,&el)) != EOF) {

  if(i==0) {acuAzElProgCommand.clearstack=0x0;}
  else {acuAzElProgCommand.clearstack=0x1;}

/*
 acuAzElProgCommand.clearstack=0x1;
*/

/*
  deltaTime = abs(acuTimeOfDay-trackTime);
*/
  newtime=acuTimeOfDay+trackTime;
 
  acuAzElProgCommand.timeOfDay=newtime;
  acuAzElProgCommand.dayOfYear=acuDayOfYear;
  acuAzElProgCommand.cmdAz = az;
  acuAzElProgCommand.cmdEl = el;
/*
  acuAzElProgCommand.timeOfDay=40200000;
  acuAzElProgCommand.dayOfYear=120;
  acuAzElProgCommand.cmdAz = 232466660;
  acuAzElProgCommand.cmdEl = 55571831;
*/

  acuAzElProgCommand.checksum=0;

  checksum = checkSum((char*)(&acuAzElProgCommand), sizeof(acuAzElProgCommand));
printf("checksum=0x%x before masking.\n",checksum);
  if(checksum > 0xffff) checksum=checksum & 0xffff;
printf("final checksum=0x%x.\n",checksum);
  acuAzElProgCommand.checksum = checksum;

printf("sending time, az, el: %d %d %d %d\n",
  acuAzElProgCommand.dayOfYear,
  acuAzElProgCommand.timeOfDay,
  acuAzElProgCommand.cmdAz,
  acuAzElProgCommand.cmdEl);

     memcpy(sendBuff,(char*)&acuAzElProgCommand,sizeof(acuAzElProgCommand));
     n = send(sockfd,sendBuff,sizeof(acuAzElProgCommand),0);
     if (n<0) printf("ERROR writing to ACU.");
     printf("Wrote %d bytes to ACU\n",n);
 
     /* receive the ACK response from ACU */
     n = recv(sockfd, recvBuff, sizeof(acuStatusResp),0); 
     printf("Received:  0x%x 0x%x from ACU\n",recvBuff[0],recvBuff[1]);

     if( n < 0)  printf("\n Read Error \n"); 

/*check if ACK is received, then receive the response and parse it */
     if (recvBuff[0]==0x6) {

     printf("ACU: ACK, OK \n");
     }

      else {
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
 i++;
 printf("line:%d\n",i);

 } /* while loop for az/el values */


  close(sockfd);
  fclose(fpin);
 
  return 0;
}

short checkSum (char *buff,int size) {
  
  short i=0,sum=0;

      while(size) {
      sum = sum + (short) buff[i]; 
      size--;
      i++;
      }
  sum -= 5; /*subtract the sum of STX 0x2 and ETX 0x3, 
           first and last bytes */
  return sum;
}
