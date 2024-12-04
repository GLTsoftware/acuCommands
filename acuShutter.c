/* Mode command */

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
  acuCmd acuCommand = {0,0,0,0,0};
  acuModeCmd acuModeCommand = {0,0,0,0,0,0,0,0};
  int commandCode=0;
   short checksum;

  printf("Enter the command code:\n");
  printf("(1) Open\n");
  printf("(2) Close\n");
  scanf("%d",&commandCode);

  if((commandCode<1)||(commandCode>2)) {
  printf("Invalid command code.\n");
  exit(0);
  }



  acuModeCommand.azMode = 0x0; /* set the Mode bits to zero*/
                                /* so only the controlWord is set below */
  acuModeCommand.elMode = 0x0; /* set the Mode bits to zero*/
   /* for now, setting both az and el modes to be same */
  switch (commandCode) {
  
  case 1:
      acuModeCommand.controlWord = 0x4000; /* open */
      break;
  case 2:
      acuModeCommand.controlWord = 0x8000; /* close */
      break;
  }

  printf("sending preset code: 0x%x\n",acuModeCommand.azMode);
  printf("sending controlWord code: 0x%x\n",acuModeCommand.controlWord);
  acuModeCommand.stx = 0x2;
  acuModeCommand.id = 0x4d; /* page 13 of ICD section 4.1.1.1  M cmd */
  acuModeCommand.datalength = 0x12;
  acuModeCommand.polMode = 0x0;
  acuModeCommand.etx = 0x3;

 checksum = checkSum((char*)(&acuModeCommand), sizeof(acuModeCommand));

  if(checksum > 0xffff) checksum=checksum & 0xffff;


  acuModeCommand.checksum = checksum;



  memset(recvBuff, '0' ,sizeof(recvBuff));
  memset(sendBuff, '0' ,sizeof(sendBuff));

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0) {
      printf("\n Error : Could not create socket \n");
      return 1;
    }
 
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(9010);
  serv_addr.sin_addr.s_addr = inet_addr("192.168.1.103");
 
  if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) {
      printf("\n Error : Connect Failed \n");
      return 1;
    }

  memcpy(sendBuff,(char*)&acuModeCommand,sizeof(acuModeCommand));
  n = send(sockfd,sendBuff,sizeof(acuModeCommand),0);
  if (n<0) printf("ERROR writing to ACU.");
  printf("Wrote %d bytes to ACU\n",n);
 
  /* receive the ACK response from ACU */
  n = recv(sockfd, recvBuff, sizeof(acuStatusResp),0); 
  printf("Received:  0x%x from ACU\n",recvBuff[0]);

  if( n < 0)  printf("\n Read Error \n"); 

  /* check if ACK is received, then receive the response and parse it */
  if (recvBuff[0]==0x6) {

  printf("ACU: ACK, OK \n");
  } else {
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

