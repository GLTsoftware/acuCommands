/* Az El (preset mode)  command */

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
  int sockfd = 0,n = 0;
  char recvBuff[256];
  char sendBuff[256];
  struct sockaddr_in serv_addr;
  acuStatus acuStatusResp;
  acuCmd acuCommand;
  acuAzElCmd acuAzElRateCommand;
  double cmdAzRate,cmdElRate;
  short checksum;

  if(argc<2) {
  printf("Usage: acuAzElRate -0.5 0.3\n");
  exit(0);
  }

  cmdAzRate = atof(argv[1]);
  cmdElRate = atof(argv[2]);
  if((cmdAzRate<-6.)||(cmdAzRate>6.)) {
  printf("Invalid commanded az rate;  should be between -6 and 6 deg/s.\n");
  exit(0);
  }

  if((cmdElRate<-3.)||(cmdElRate>3.)) {
  printf("Invalid commanded el rate; should be between -3 and 3 deg/s.\n");
  exit(0);
  }

  cmdAzRate *= 1.0e6;
  cmdElRate *= 1.0e6;

  acuAzElRateCommand.cmdAz = (int)cmdAzRate;
  acuAzElRateCommand.cmdEl = (int)cmdElRate;

  printf("%d %d\n",acuAzElRateCommand.cmdAz,acuAzElRateCommand.cmdEl);

  acuAzElRateCommand.stx = 0x2;
  acuAzElRateCommand.id = 0x52; /* page 14 of ICD section 4.1.1.2  R cmd */
  acuAzElRateCommand.datalength = 0x13;
  acuAzElRateCommand.cmdPol = 0x0;
  acuAzElRateCommand.etx = 0x3;


  checksum = checkSum((char*)(&acuAzElRateCommand), sizeof(acuAzElRateCommand));

/*
  printf("checksum=%d 0x%x\n",checksum,checksum);
*/

  if(checksum > 0xffff) checksum=checksum & 0xffff;

/*
  printf("after masking... checksum=%d 0x%x\n",checksum,checksum);
*/
  
  acuAzElRateCommand.checksum = checksum;

/*
  printf("sending ... checksum=0x%x\n",acuAzElRateCommand.checksum);
*/

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

  memcpy(sendBuff,(char*)&acuAzElRateCommand,sizeof(acuAzElRateCommand));
  n = send(sockfd,sendBuff,sizeof(acuAzElRateCommand),0);
  if (n<0) printf("ERROR writing to ACU.");
  printf("Wrote %d bytes to ACU\n",n);
 
  /* receive the ACK response from ACU */
  n = recv(sockfd, recvBuff, sizeof(acuStatusResp),0); 
  printf("Received:  0x%x 0x%x from ACU\n",recvBuff[0],recvBuff[1]);

  if( n < 0)  printf("\n Read Error \n"); 

  /* check if ACK is received, then receive the response and parse it */
  if (recvBuff[0]==0x6) {

  printf("ACU: ACK, OK \n");
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
