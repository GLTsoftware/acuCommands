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
 
int main(void)
{
  int sockfd = 0,n = 0;
  char recvBuff[256];
  char sendBuff[256];
  struct sockaddr_in serv_addr;
  acuStatus acuStatusResp;
  acuCmd acuCommand;
  acuAuxCmd acuAuxCommand;
  int commandCode=0;

  printf("Enter the command code:\n");
  printf("(0) Normal operation (no Aux mode)\n");
  printf("(1) Az Aux 1\n");
  printf("(2) Az Aux 2\n");
  printf("(3) El Aux 1\n");
  printf("(4) El Aux 2 \n");
  scanf("%d",&commandCode);

  if((commandCode<0)||(commandCode>4)) {
  printf("Invalid command code.\n");
  exit(0);
  }


/*
    char stx;
    char id;
    short datalength;
    char azMode;
    char elMode;
    short checksum;
    char etx;
*/

  acuAuxCommand.stx = 0x2;
  acuAuxCommand.id = 0x78; /* page 25 of ICD section 4.1.1.9  x cmd */
  acuAuxCommand.datalength = 0x9;
  acuAuxCommand.etx = 0x3;


printf("Entered command code: %d\n",commandCode);

   /* for now, setting both az and el modes to be same */
  switch (commandCode) {
  
  case 0:
      acuAuxCommand.azMode = 0x0;   
      acuAuxCommand.elMode = 0x0;   
      break;
  case 1:
      acuAuxCommand.azMode = 0x1;  
      break;
  case 2:
      acuAuxCommand.azMode = 0x2;   
      break;
  case 3:
      acuAuxCommand.elMode = 0x1; 
      break;
  case 4:
      acuAuxCommand.elMode = 0x2;
      break;
  }

  printf("sending preset code: 0x%x\n",acuAuxCommand.azMode);
  acuAuxCommand.checksum = acuAuxCommand.id + acuAuxCommand.datalength+
                         acuAuxCommand.azMode+acuAuxCommand.elMode;


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

  memcpy(sendBuff,(char*)&acuAuxCommand,sizeof(acuAuxCommand));
  n = send(sockfd,sendBuff,sizeof(acuAuxCommand),0);
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
