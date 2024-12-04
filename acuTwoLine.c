/* Two Line   command */

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
  /*acuTwoLineCmd acuTwoLineCommand = {0,0,0,0,0,0,0,0};*/
  acuTwoLineCmd acuTwoLineCommand;
  char line0[25];
  char line1[70];
  char line2[70];

  char dummy[70];
  short checksum;
  char filename[20];


  strcpy(filename,argv[1]);
  FILE *file = fopen(filename,"r");
  if (file == NULL) {
  printf("Error opening file: %s\n",filename);
  exit(1);
  }
   
  fgets(line0,25,file);
  fgets(dummy,70,file);
  fgets(line1,70,file);
  fgets(dummy,70,file);
  fgets(line2,70,file);
  fclose(file);
  printf("Name: %s %d\n",line0,strlen(line0));
  printf("Line 1: %s %d\n",line1,strlen(line1));
  printf("Line 2: %s %d\n",line2,strlen(line2));

  strncpy(acuTwoLineCommand.line0,line0,24);
  strncpy(acuTwoLineCommand.line1,line1,69);
  strncpy(acuTwoLineCommand.line2,line2,69);


  acuTwoLineCommand.stx = 0x2;
  acuTwoLineCommand.id = 0x45; /* page 21 of ICD section 4.1.1.6  E cmd */
  acuTwoLineCommand.datalength = 0xa9;
  acuTwoLineCommand.etx = 0x3;

  checksum = checkSum((char*)(&acuTwoLineCommand), sizeof(acuTwoLineCommand));

  if(checksum > 0xffff) checksum=checksum & 0xffff;
  
  
  acuTwoLineCommand.checksum = checksum;


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

  memcpy(sendBuff,(char*)&acuTwoLineCommand,sizeof(acuTwoLineCommand));
  n = send(sockfd,sendBuff,sizeof(acuTwoLineCommand),0);
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

  if (recvBuff[0]!=0x6) {
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
