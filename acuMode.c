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
  acuModeCmd acuModeCommand;
  int commandCode=0;

  printf("Enter the command code:\n");
  printf("(1) Stop\n");
  printf("(2) Maintenance\n");
  printf("(3) Preset\n");
  printf("(4) Program Track\n");
  printf("(5) Rate \n");
  printf("(6) Sector scan\n");
  printf("(7) Survival stow\n");
  printf("(8) Maintenance stow\n");
  printf("(9) Unstow \n");
  printf("(10) Two Line mode \n");
  scanf("%d",&commandCode);

  if((commandCode<1)||(commandCode>10)) {
  printf("Invalid command code.\n");
  exit(0);
  }


  acuModeCommand.stx = 0x2;
  acuModeCommand.id = 0x4d; /* page 13 of ICD section 4.1.1.1  M cmd */
  acuModeCommand.datalength = 0x12;
  acuModeCommand.polMode = 0x0;
  acuModeCommand.controlWord = 0x0;
  acuModeCommand.etx = 0x3;

   /* for now, setting both az and el modes to be same */
  switch (commandCode) {
  
  case 1:
      acuModeCommand.azMode = 0x1;   /* stop */
      break;
  case 2:
      acuModeCommand.azMode = 0x21;  /* maintenance */
      break;
  case 3:
      acuModeCommand.azMode = 0x2;   /* preset */
      break;
  case 4:
      acuModeCommand.azMode = 0x3;  /* program track */
      break;
  case 5:
      acuModeCommand.azMode = 0x4;  /* rate */
      break;
  case 6: 
      acuModeCommand.azMode = 0x5;  /* sector scan */
      break;
  case 7:
      acuModeCommand.azMode = 0x6;  /* survival stow */
      break;
  case 8:
      acuModeCommand.azMode = 0xe;  /* maint. stow */
      break;
  case 9:
      acuModeCommand.azMode = 0x26; /* unstow */
      break;
  case 10:
      acuModeCommand.azMode = 0x8; /* two line tracking */
      break;
  }

  printf("sending preset code: 0x%x\n",acuModeCommand.azMode);
  acuModeCommand.elMode=acuModeCommand.azMode;
  acuModeCommand.checksum = acuModeCommand.id + acuModeCommand.datalength+
                         acuModeCommand.azMode+acuModeCommand.elMode;


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
  printf("ACU refuses the command...reason: 0x%x\n",recvBuff[1]);
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
