/* This program reports the status of the self test.
Only one pass of inquiry is made- repeated use with up-arrow
can show the updates. 
Nimesh Patel
*/

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
  int i;
  struct sockaddr_in serv_addr;
  selfTestResults acuStResult;
  acuCmd acuCommand;
  float selfTestValue=0.0;
  char selfTestValueBuff[4];

  acuCommand.stx = 0x2;
  acuCommand.id = 0x7A;
  acuCommand.datalength = 0x7;
  acuCommand.checksum = 0x81;
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
  printf("Wrote %d bytes to ACU\n",n);
 
  /* receive the ACK response from ACU */
  n = recv(sockfd, recvBuff, sizeof(acuStResult),0); 

  if( n < 0)  printf("\n Read Error \n"); 

  /* check if ACK is received, then receive the response and parse it */
  if (recvBuff[0]==0x6) {

  n = recv(sockfd, (char *)&acuStResult, sizeof(acuStResult),0);

  printf("Read %d bytes from ACU\n",n);
  printf ("Received %d bytes from ACU.\n",acuStResult.datalength);
  printf("Self Test Status:  %c\n",acuStResult.status);
  if(acuStResult.status=='A') printf("...Active...\n");
  if(acuStResult.status=='C') printf("...Completed...\n");
  if(acuStResult.status=='I') printf("...Inactive...\n");
  if(acuStResult.status=='X') printf("...Aborted...\n");
  printf("Current test:  %d\n",acuStResult.currentTest);
  printf("Number of failed tests:  %d\n",acuStResult.numOfFailedTests);
  
  for(i=0;i<acuStResult.numOfFailedTests;i++) {
  printf("Failed test number: %d\n",acuStResult.failedTestNum[i]);
  printf("Failed test value: %f\n",acuStResult.measuredValue[i]);
  selfTestValue=acuStResult.measuredValue[i];
  memcpy(selfTestValueBuff,(char*)&selfTestValue,sizeof(selfTestValue));
  printf(".......... 0x%x,0x%x,0x%x,0x%x\n",selfTestValueBuff[0],
                                            selfTestValueBuff[1],
                                            selfTestValueBuff[2],
                                            selfTestValueBuff[3]);
  }
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
