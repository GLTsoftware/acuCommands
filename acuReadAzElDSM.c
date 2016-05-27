/* program for testing the new GLT ACU interface */
/* N. A. Patel, March 2014 */
/* 26 May 2016: reading az and el from ACU and writing to DSM for monitoring*/

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

#include "dsm.h"

#define ACC 140.109.177.49
 
int main(void)
{
  int dsm_status,dsm_open_status;
  double az=0.,el=0.;
  time_t timestamp;

  dsm_open_status = dsm_open();
    if(dsm_open_status != DSM_SUCCESS) {
    dsm_error_message(dsm_open_status, "dsm_open failed.");
    exit(-1);
  }

  dsm_status = dsm_read("140.109.177.49","DSM_AZ_POSN_DEG_D",&az,&timestamp);
  dsm_status = dsm_read("140.109.177.49","DSM_AZ_POSN_DEG_D",&az,&timestamp);
  dsm_status = dsm_read("140.109.177.49","DSM_EL_POSN_DEG_D",&el,&timestamp);

  if (dsm_status != DSM_SUCCESS) {
  printf("Warning: DSM read failed!\n");
  }

  printf("Read from DSM:\n");
  printf("az=%f deg at %d sec\n",az,timestamp);
  printf("el=%f deg at %d sec\n",el,timestamp);
  dsm_status = dsm_close();
      if(dsm_status != DSM_SUCCESS) {
      dsm_error_message(dsm_status, "dsm_close");
      exit(-1);
      }

 
  return 0;
}
