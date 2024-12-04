/****************************************************************

acuMonitor.c

Nimesh Patel

version 1.0
23 May 2023
	
This code has acu monitoring points that used to be in glttrack.c.
This version uses redis instead of dsm.

*****************************************************************/

#include <stdio.h>
#include <sys/utsname.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <termio.h>
#include <sys/time.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <pthread.h>
#include <hiredis.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

#include "acuData.h"
#include "acuCommands.h"

#define ACC "gltobscon"

/* redis server (gltobscon) */
#define REDIS_SERVER "192.168.1.11"
#define REDIS_PORT 6379

/* ACU ports, see 3.1.1.2 of ICD */
#define ACU_MONITOR_PORT 9110

/* ACU IP address */
#define ACU_IP_ADDRESS "192.168.1.103"

#define DEBUG 0
#define COORDINATES_SVC 1
#define T1950   2433281.0
#define T2000   2451544.0

#define MSEC_PER_DEG 3600000.
#define MSEC_PER_HR 3600000.      
#define AUPERDAY_KMPERSEC 1731.45717592 

#define MICRODEG_TO_ARCSEC 0.0036


/* redis server (gltobscon) */
#define REDIS_SERVER "192.168.1.11"
#define REDIS_PORT 6379

/************************ Function declarations *************************/
void *ACUstatus();
void *ACUiostatus();
void *ACUselftest();
short checkSum (char *buff, int size);
void redisWriteShort(char *hashName, char *fieldName, short variable);
void redisWriteInt(char *hashName, char *fieldName, int variable);
void redisWriteFloat(char *hashName, char *fieldName,float variable);
void redisWriteDouble(char *hashName, char *fieldName, double variable);
void redisWriteString(char *hashName, char *fieldName, char *variable);
extern void metrology();

/** Global variables *********************************************** */


struct sigaction action, old_action;
int sigactionInt;

pthread_t	ACUstatusTID,ACUiostatusTID,ACUselftestTID ;

	struct sched_param param,param2,param3,param4;
	pthread_attr_t attr,attr2,attr3,attr4;
	int policy = SCHED_FIFO;


       int sockfdMonitor=0;
     
        double az_enc_from_acu,el_enc_from_acu;

char redisData[1024];
redisContext *redisC;
redisReply *redisResp;
struct timeval redisTimeout = {1,500000}; /*1.5 seconds for redis timeout */

/*end of global variables***************************************************/

/* Functions to write data  to Redis */

void redisWriteShort(char *hashName, char *fieldName, short variable) {
        sprintf(redisData,"HSET %s %s %h",hashName,fieldName,variable);
        redisResp = redisCommand(redisC,redisData);
}

void redisWriteInt(char *hashName, char *fieldName, int variable) {
        sprintf(redisData,"HSET %s %s %d",hashName,fieldName,variable);
        redisResp = redisCommand(redisC,redisData);
}
void redisWriteFloat(char *hashName, char *fieldName,float variable) {
        sprintf(redisData,"HSET %s %s %f",hashName,fieldName,variable);
        redisResp = redisCommand(redisC,redisData);
}
void redisWriteDouble(char *hashName, char *fieldName, double variable) {
        sprintf(redisData,"HSET %s %s %lf",hashName,fieldName,variable);
        redisResp = redisCommand(redisC,redisData);
}

void redisTSADD(char *fieldName, double variable) {
        sprintf(redisData,"TS.ADD %s * %lf",fieldName,variable);
        redisResp = redisCommand(redisC,redisData);
}

void redisWriteString(char *hashName, char *fieldName, char *variable) {
        sprintf(redisData,"HSET %s %s %s",hashName,fieldName,variable);
        redisResp = redisCommand(redisC,redisData);
}


int main(int argc, char *argv[]) {
    
    double          az, el;  

    double          hr;

    int             azint, elint ;

    int             icount = 0, app_pos_flag = 0;

    short           ret;


    /* The following variables are defined for time calculations */
 int              hours, minutes;	
	double seconds,et,delta=0.;

    /* variables for actual az */
    double          az_actual, el_actual;
    double          az_actual_disp, el_actual_disp;

    /* end of time variables definitions */


    double          az_disp, el_disp ;
    double          az_disp_rm, el_disp_rm;


    int             initflag = 0;

	char line[BUFSIZ];


    double          az_actual_msec, el_actual_msec;

    unsigned long          az_actual_msec_int, el_actual_msec_int;

    double          posn_error, az_error, el_error;


    short           azoff_int, eloff_int;



	double az_actual_corrected;

	float az_tracking_error,el_tracking_error;
	short dummyshortint;
	float dummyFloat;
	double dummyDouble;
	char dummyByte;

        time_t timestamp;

	
        /* for ACU ethernet communications */
       struct sockaddr_in serv_addr;

     int iT,respACU,firstTrack=0;

    /* END OF VARIABLE DECLARATIONS */

    /********Initializations**************************/


       /* initialize connection to redis */
       redisC = redisConnectWithTimeout(REDIS_SERVER,REDIS_PORT,redisTimeout);
       if (redisC == NULL || redisC->err) {
        if (redisC) {
            printf("Connection error: %s\n", redisC->errstr);
            redisFree(redisC);
          } else {
            printf("Connection error: can't allocate redis context\n");
        }
       }


        /* initializing ACU ethernet communications */

/*
        if((sockfdMonitor = socket(AF_INET, SOCK_STREAM, 0))< 0) {
            printf("\n Error : Could not create socket \n");
            return 1;
          }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(ACU_MONITOR_PORT);
        serv_addr.sin_addr.s_addr = inet_addr(ACU_IP_ADDRESS);

        if(connect(sockfdMonitor, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)           {
            printf("\n Error : Connect Failed \n");
            return 1;
          }
*/

 
/*
	pthread_attr_init(&attr);
	if (pthread_create(&ACUstatusTID, &attr, ACUstatus,
			 (void *) 0) == -1) { 
	perror("main: pthread_create ACUstatus");
	exit(-1);
	}
	param2.sched_priority=10;
	pthread_attr_setschedparam(&attr,&param);
	pthread_setschedparam(ACUstatusTID,policy,&param);

	pthread_attr_init(&attr2);
	if (pthread_create(&ACUselftestTID, &attr2, ACUselftest,
			 (void *) 0) == -1) { 
	perror("main: pthread_create ACUselftest");
	exit(-1);
	}
	param2.sched_priority=10;
	pthread_attr_setschedparam(&attr2,&param2);
	pthread_setschedparam(ACUselftestTID,policy,&param2);

	pthread_attr_init(&attr4);
	if (pthread_create(&ACUiostatusTID, &attr4, ACUiostatus,
			 (void *) 0) == -1) { 
	perror("main: pthread_create ACUiostatus");
	exit(-1);
	}
	param4.sched_priority=10;
	pthread_attr_setschedparam(&attr4,&param4);
	pthread_setschedparam(ACUiostatusTID,policy,&param4);
*/

/*
icount=0;
*/
while(1) {
/*
if(icount%10==0) printf("\n");
 else printf(". ");
*/
metrology();
sleep(60);
/*
icount++;
*/
}


return(0);
}				/* end of main */


void *ACUstatus() {

  int i,n = 0;
  char recvBuff[256];
  char sendBuff[256];
  int days,hh,mm;
  double az=0.,el=0.;
  double acuCmdAz,acuCmdEl;
  double hours,minutes,seconds;
  float az_tracking_error,el_tracking_error;
  acuStatus acuStatusResp;
  acuCmd acuCommand={};

  short acuModeAz,acuModeEl;
  char azServoStatus[2],elServoStatus[2];
  int acuDay,acuHour;
  char acuErrorMessage[256],acuSystemGS[6];


  acuCommand.stx = 0x2;
  acuCommand.id = 0x71;
  acuCommand.datalength = 0x7;
  acuCommand.checksum = 0x78;
  acuCommand.etx = 0x3;

  memset(recvBuff, '0' ,sizeof(recvBuff));
  memset(sendBuff, '0' ,sizeof(sendBuff));


  memcpy(sendBuff,(char*)&acuCommand,sizeof(acuCommand));

  while(1) {
	
  n = send(sockfdMonitor,sendBuff,sizeof(acuCommand),0);
  if (n<0) printf("ERROR writing to ACU. From ACUstatus");

  /* receive the ACK response from ACU */
  n = recv(sockfdMonitor, recvBuff, sizeof(acuStatusResp),0);

  if( n < 0)  printf("\n Read Error from ACUstatus\n");

  /* check if ACK is received, then receive the response and parse it */
  if (recvBuff[0]==0x6) {

  n = recv(sockfdMonitor, (char *)&acuStatusResp, sizeof(acuStatusResp),0);

/*
  printf("Read %d bytes from ACU\n",n);
  printf ("Received %d bytes from ACU.\n",acuStatusResp.datalength);
*/
/*
printf("Time: %d\n",acuStatusResp.timeOfDay);
*/
  hours = acuStatusResp.timeOfDay/3600000.;
  hh = (int)hours;
  minutes = (hours-hh)*60.;
  mm = (int) minutes;
  seconds = (minutes-mm)*60.;
/*
  printf ("ACU Time: (day, hh:mm:ss.sss):  %d %02d:%02d:%02.3f\n",acuStatusResp.dayOfYear,hh,mm,seconds);
*/
/*

  printf ("azPos (deg): %f \n",(double)acuStatusResp.azPos/1.0e6);
  printf ("elPos (deg): %f \n",(double)acuStatusResp.elPos/1.0e6);
  printf ("cmdAzPos (deg): %f \n",(double)acuStatusResp.cmdAzPos/1.0e6);
  printf ("cmdElPos (deg): %f \n",(double)acuStatusResp.cmdElPos/1.0e6);
  printf ("azStatusMode: 0x%x \n",acuStatusResp.azStatusMode);
  printf ("elStatusMode: 0x%x \n",acuStatusResp.elStatusMode);
  printf ("servoSystemStatusAz bytes 1,2: 0x%x 0x%x\n",acuStatusResp.servoSystemStatusAz[0],acuStatusResp.servoSystemStatusAz[1]);
*/

  az = (double)acuStatusResp.azPos/1.0e6;
  el = (double)acuStatusResp.elPos/1.0e6;
  acuCmdAz = (double)acuStatusResp.cmdAzPos/1.0e6;
  acuCmdEl = (double)acuStatusResp.cmdElPos/1.0e6;

/* Removed above offset after ACU change: 5 Mar 2018 */

  az_enc_from_acu=az;
  el_enc_from_acu=el; 

        sprintf(redisData,"HSET acu azPosn %lf",az);
        redisResp = redisCommand(redisC,redisData);

        sprintf(redisData,"HSET acu elPosn %lf",el);
        redisResp = redisCommand(redisC,redisData);

        sprintf(redisData,"HSET acu azCmdPosn %lf",acuCmdAz);
        redisResp = redisCommand(redisC,redisData);

        sprintf(redisData,"HSET acu elCmdPosn %lf",acuCmdEl);
        redisResp = redisCommand(redisC,redisData);

  az_tracking_error = (float)(acuCmdAz-az)*3600.;
  el_tracking_error = (float)(acuCmdEl-el)*3600.;

        sprintf(redisData,"HSET gltTrackComp azTrackingError %f",az_tracking_error);
        redisResp = redisCommand(redisC,redisData);

        sprintf(redisData,"HSET gltTrackComp elTrackingError %f",el_tracking_error);
        redisResp = redisCommand(redisC,redisData);

  acuModeAz=acuStatusResp.azStatusMode;
  acuModeEl=acuStatusResp.elStatusMode;
  strcpy(azServoStatus, acuStatusResp.servoSystemStatusAz);
  strcpy(elServoStatus, acuStatusResp.servoSystemStatusEl);
  acuHour=acuStatusResp.timeOfDay; /*msec*/
  acuDay=acuStatusResp.dayOfYear;
  
  for(i=0;i<6;i++) {
  acuSystemGS[i]=acuStatusResp.servoSystemGS[i];
  }

/*
  dsm_status = dsm_write(ACC,"DSM_ACU_SERVO_STATUS_AZ_C2",azServoStatus);
  if (dsm_status != DSM_SUCCESS) {
  printf("DSM write failed! DSM_ACU_SERVO_STATUS_AZ_C2 dsm_status=%d\n",dsm_status);
  }
  dsm_status = dsm_write(ACC,"DSM_ACU_SERVO_STATUS_EL_C2",elServoStatus);
  if (dsm_status != DSM_SUCCESS) {
  printf("DSM write failed! DSM_ACU_SERVO_STATUS_EL_C2 dsm_status=%d\n",dsm_status);
  }
  dsm_status = dsm_write(ACC,"DSM_ACU_MODE_STATUS_AZ_S",&acuModeAz);
  if (dsm_status != DSM_SUCCESS) {
  printf("DSM write failed! DSM_ACU_MODE_STATUS_AZ_S dsm_status=%d\n",dsm_status);
  }
  dsm_status = dsm_write(ACC,"DSM_ACU_MODE_STATUS_EL_S",&acuModeEl);
  if (dsm_status != DSM_SUCCESS) {
  printf("DSM write failed! DSM_ACU_MODE_STATUS_EL_S dsm_status=%d\n",dsm_status);
  }
  dsm_status = dsm_write(ACC,"DSM_ACU_SYSTEMGS_C6",acuSystemGS);
  if (dsm_status != DSM_SUCCESS) {
  printf("DSM write failed! DSM_ACU_SYSTEMGS_C6 dsm_status=%d\n",dsm_status);
  }
  dsm_status = dsm_write(ACC,"DSM_ACU_DAYOFYEAR_L",&acuDay);
  if (dsm_status != DSM_SUCCESS) {
  printf("DSM write failed! DSM_ACU_DAYOFYEAR_L dsm_status=%d\n",dsm_status);
  }
*/
        sprintf(redisData,"HSET acu dayOfYear %d",acuDay);
        redisResp = redisCommand(redisC,redisData);
        sprintf(redisData,"HSET acu hour %d",acuHour);
        redisResp = redisCommand(redisC,redisData);


/*
  if (acuStatusResp.azStatusMode == 0x1) printf(" Az stop.\n");
  if (acuStatusResp.azStatusMode == 0x21) printf(" Az Maintenance.\n");
  if (acuStatusResp.azStatusMode == 0x2) printf(" Az Preset.\n");
  if (acuStatusResp.azStatusMode == 0x3) printf(" Az Program Track.\n");
  if (acuStatusResp.azStatusMode == 0x4) printf(" Az Rate.\n");
  if (acuStatusResp.azStatusMode == 0x5) printf(" Az Sector Scan.\n");
  if (acuStatusResp.azStatusMode == 0x6) printf(" Az Survival Position (stow).\n");
  if (acuStatusResp.azStatusMode == 0xe) printf(" Az Maintenance Position (stow).\n");
  if (acuStatusResp.azStatusMode == 0x4e) printf(" Az Stow (stow pins not retracted).\n");
  if (acuStatusResp.azStatusMode == 0x26) printf(" Az unstow.\n");
  if (acuStatusResp.azStatusMode == 0x8) printf(" Az Two line track.\n");
  if (acuStatusResp.azStatusMode == 0x9) printf(" Az Star Track.\n");
  if (acuStatusResp.azStatusMode == 0x29) printf(" Az Sun Track.\n");

  if (acuStatusResp.elStatusMode == 1) printf(" El stop.\n");
  if (acuStatusResp.elStatusMode == 33) printf(" El Maintenance.\n");
  if (acuStatusResp.elStatusMode == 2) printf(" El Preset.\n");
  if (acuStatusResp.elStatusMode == 3) printf(" El Program Track.\n");
  if (acuStatusResp.elStatusMode == 4) printf(" El Rate.\n");
  if (acuStatusResp.elStatusMode == 5) printf(" El Sector Scan.\n");
  if (acuStatusResp.elStatusMode == 6) printf(" El Survival Position (stow).\n");
  if (acuStatusResp.elStatusMode == 14) printf(" El Maintenance Position (stow).\n");
  if (acuStatusResp.elStatusMode == 78) printf(" El Stow (stow pins not retracted).\n");
  if (acuStatusResp.elStatusMode == 38) printf(" El unstow.\n");
  if (acuStatusResp.elStatusMode == 8) printf(" El Two line track.\n");
  if (acuStatusResp.elStatusMode == 9) printf(" El Star Track.\n");
  if (acuStatusResp.elStatusMode == 41) printf(" El Sun Track.\n");

  if (acuStatusResp.servoSystemStatusAz[0] & 1)
                printf(" Az emergency limit.\n");
  if (acuStatusResp.servoSystemStatusAz[0] & 2)
                printf(" Az operating limit ccw.\n");
  if (acuStatusResp.servoSystemStatusAz[0] & 4)
                printf(" Az operating limit cw.\n");
  if (acuStatusResp.servoSystemStatusAz[0] & 8)
                printf(" Az prelimit ccw.\n");
  if (acuStatusResp.servoSystemStatusAz[0] & 16)
                printf(" Az prelimit cw.\n");
  if (acuStatusResp.servoSystemStatusAz[0] & 32)
                printf(" Az stow position.\n");
  if (acuStatusResp.servoSystemStatusAz[0] & 64)
                printf(" Az stow pin inserted.\n");
  if (acuStatusResp.servoSystemStatusAz[0] & 128)
                printf(" Az stow pin retracted.\n");

 if (acuStatusResp.servoSystemStatusAz[1] & 1)
                printf(" Az servo failure.\n");
  if (acuStatusResp.servoSystemStatusAz[1] & 2)
                printf(" Az brake failure.\n");
  if (acuStatusResp.servoSystemStatusAz[1] & 4)
                printf(" Az encoder failure.\n");
  if (acuStatusResp.servoSystemStatusAz[1] & 8)
                printf(" Az auxiliary mode.\n");
  if (acuStatusResp.servoSystemStatusAz[1] & 16)
                printf(" Az motion failure.\n");
  if (acuStatusResp.servoSystemStatusAz[1] & 32)
                printf(" Az CAN bus failure.\n");
  if (acuStatusResp.servoSystemStatusAz[1] & 64)
                printf(" Az axis disabled.\n");
  if (acuStatusResp.servoSystemStatusAz[1] & 128)
                printf(" Az computer disabled (local mode).\n");

  printf ("servoSystemStatusEl bytes 1,2: 0x%x 0x%x \n",acuStatusResp.servoSystemStatusEl[0],acuStatusResp.servoSystemStatusEl[1]);
  if (acuStatusResp.servoSystemStatusEl[0] & 1)
                printf(" El emergency limit.\n");
  if (acuStatusResp.servoSystemStatusEl[0] & 2)
                printf(" El operating limit ccw.\n");
  if (acuStatusResp.servoSystemStatusEl[0] & 4)
                printf(" El operating limit cw.\n");
  if (acuStatusResp.servoSystemStatusEl[0] & 8)
                printf(" El prelimit ccw.\n");
  if (acuStatusResp.servoSystemStatusEl[0] & 16)
                printf(" El prelimit cw.\n");
  if (acuStatusResp.servoSystemStatusEl[0] & 32)
                printf(" El stow position.\n");
  if (acuStatusResp.servoSystemStatusEl[0] & 64)
                printf(" El stow pin inserted.\n");
  if (acuStatusResp.servoSystemStatusEl[0] & 128)
                printf(" El stow pin retracted.\n");

  if (acuStatusResp.servoSystemStatusEl[1] & 1)
                printf(" El servo failure.\n");
  if (acuStatusResp.servoSystemStatusEl[1] & 2)
                printf(" El brake failure.\n");
  if (acuStatusResp.servoSystemStatusEl[1] & 4)
                printf(" El encoder failure.\n");
  if (acuStatusResp.servoSystemStatusEl[1] & 8)
                printf(" El auxiliary mode.\n");
  if (acuStatusResp.servoSystemStatusEl[1] & 16)
                printf(" El motion failure.\n");
  if (acuStatusResp.servoSystemStatusEl[1] & 32)
                printf(" El CAN bus failure.\n");
  if (acuStatusResp.servoSystemStatusEl[1] & 64)
                printf(" El axis disabled.\n");
  if (acuStatusResp.servoSystemStatusEl[1] & 128)
                printf(" El computer disabled (local mode).\n");

  printf ("servoSystemGStatus1-6: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",acuStatusResp.servoSystemGS[0],
  acuStatusResp.servoSystemGS[1],
  acuStatusResp.servoSystemGS[2],
  acuStatusResp.servoSystemGS[3],
  acuStatusResp.servoSystemGS[4],
  acuStatusResp.servoSystemGS[5]);
*/

/*
  if (acuStatusResp.servoSystemGS[0] & 1)
                printf(" Door Interlock.\n");
  if (acuStatusResp.servoSystemGS[0] & 2)
                printf(" SAFE .\n");
  if (acuStatusResp.servoSystemGS[0] & 64)
                printf(" Emergency off.\n");
  if (acuStatusResp.servoSystemGS[0] & 128)
                printf(" Not on source.\n");

  if (acuStatusResp.servoSystemGS[1] & 4)
                printf(" Time error.\n");
  if (acuStatusResp.servoSystemGS[1] & 8)
                printf(" Year error.\n");
  if (acuStatusResp.servoSystemGS[1] & 32)
                printf(" Green mode active.\n");
  if (acuStatusResp.servoSystemGS[1] & 64)
                printf(" Speed high.\n");
  if (acuStatusResp.servoSystemGS[1] & 128)
                printf(" Remote.\n");

  if (acuStatusResp.servoSystemGS[2] & 1)
                printf(" Spline green.\n");
  if (acuStatusResp.servoSystemGS[2] & 2)
                printf(" Spline yellow.\n");
  if (acuStatusResp.servoSystemGS[2] & 4)
                printf(" Spline red.\n");
  if (acuStatusResp.servoSystemGS[2] & 16)
                printf(" Gearbox oil level warning.\n");
  if (acuStatusResp.servoSystemGS[2] & 32)
                printf(" PLC interface ok.\n");

  if (acuStatusResp.servoSystemGS[3] & 1)
                printf(" PCU mode.\n");
  if (acuStatusResp.servoSystemGS[3] & 4)
                printf(" Tiltmeter error.\n");

  if (acuStatusResp.servoSystemGS[4] & 1)
                printf(" Cabinet overtemperature.\n");
  if (acuStatusResp.servoSystemGS[4] & 4)
                printf(" Shutter open.\n");
  if (acuStatusResp.servoSystemGS[4] & 8)
                printf(" Shutter closed.\n");
  if (acuStatusResp.servoSystemGS[4] & 16)
                printf(" Shutter failure.\n");
*/
  }

  if (recvBuff[0]==0x2) {
  sprintf(acuErrorMessage,"ACU refuses the command from ACUstatus...reason:");
  sleep(2);
  if (recvBuff[1]==0x43) sprintf(acuErrorMessage,"Checksum error.\n");
  if (recvBuff[1]==0x45) sprintf(acuErrorMessage,"ETX not received at expected position.\n");
  if (recvBuff[1]==0x49) sprintf(acuErrorMessage,"Unknown identifier.\n");
  if (recvBuff[1]==0x4C) sprintf(acuErrorMessage,"Wrong length (incorrect no. of bytes rcvd.\n");
  if (recvBuff[1]==0x6C) sprintf(acuErrorMessage,"Specified length does not match identifier.\n");
  if (recvBuff[1]==0x4D) sprintf(acuErrorMessage,"Command ignored in present operating mode.\n");
  if (recvBuff[1]==0x6F) sprintf(acuErrorMessage,"Other reasons.\n");
  if (recvBuff[1]==0x52) sprintf(acuErrorMessage,"Device not in Remote mode.\n");
  if (recvBuff[1]==0x72) sprintf(acuErrorMessage,"Value out of range.\n");
  if (recvBuff[1]==0x53) sprintf(acuErrorMessage,"Missing STX.\n");
  } else {sprintf(acuErrorMessage,"");}



        /* usleep(2000000); */
        usleep(3000000);
	} /* while loop */

/*   close(sockfdMonitor); */

	pthread_detach(ACUstatusTID);
	pthread_exit((void *) 0);
}

void *ACUiostatus() {

  int n = 0;
  char recvBuff[256];
  char sendBuff[256];
  char acuErrorMessage[256];
  int days,hh,mm;
  double hours,minutes,seconds;
  ioStatus ioStatusResp;
  acuCmd acuCommand={};

  short azmotor1temp;
  short azmotor2temp;
  short azmotor3temp;
  short azmotor4temp;
  short elmotor1temp;
  short elmotor2temp;
  short elmotor3temp;
  short elmotor4temp;
  short az1motorcurrent;
  short az2motorcurrent;
  short el1motorcurrent;
  short el2motorcurrent;
  short el3motorcurrent;
  short el4motorcurrent;
  float az1motorcurrentF;
  float az2motorcurrentF;
  float el1motorcurrentF;
  float el2motorcurrentF;
  float el3motorcurrentF;
  float el4motorcurrentF;


  acuCommand.stx = 0x2;
  acuCommand.id = 0x51;
  acuCommand.datalength = 0x7;
  acuCommand.checksum = 0x58;
  acuCommand.etx = 0x3;

  memset(recvBuff, '0' ,sizeof(recvBuff));
  memset(sendBuff, '0' ,sizeof(sendBuff));

  memcpy(sendBuff,(char*)&acuCommand,sizeof(acuCommand));


	while(1) {
	
  n = send(sockfdMonitor,sendBuff,sizeof(acuCommand),0);
  if (n<0) printf("ERROR writing to ACU. From ACUiostatus");

  /* receive the ACK response from ACU */
  n = recv(sockfdMonitor, recvBuff, sizeof(ioStatusResp),0);

  if( n < 0)  printf("\n Read Error from ACUiostatus.\n");

  /* check if ACK is received, then receive the response and parse it */
  if (recvBuff[0]==0x6) {

  n = recv(sockfdMonitor, (char *)&ioStatusResp, sizeof(ioStatusResp),0);

/*
  printf("Read %d bytes from ACU\n",n);
  printf ("Received %d bytes from ACU.\n",ioStatusResp.datalength);
*/
/*
printf("Time: %d\n",ioStatusResp.timeOfDay);
*/
  hours = ioStatusResp.timeOfDay/3600000.;
  hh = (int)hours;
  minutes = (hours-hh)*60.;
  mm = (int) minutes;
  seconds = (minutes-mm)*60.;
/*
  printf ("ACU Time: (day, hh:mm:ss.sss):  %d %02d:%02d:%02.3f\n",ioStatusResp.dayOfYear,hh,mm,seconds);
*/
/*

  printf ("azPos (deg): %f \n",(double)ioStatusResp.azPos/1.0e6);
  printf ("elPos (deg): %f \n",(double)ioStatusResp.elPos/1.0e6);
  printf ("cmdAzPos (deg): %f \n",(double)ioStatusResp.cmdAzPos/1.0e6);
  printf ("cmdElPos (deg): %f \n",(double)ioStatusResp.cmdElPos/1.0e6);
  printf ("azStatusMode: 0x%x \n",ioStatusResp.azStatusMode);
  printf ("elStatusMode: 0x%x \n",ioStatusResp.elStatusMode);
  printf ("servoSystemStatusAz bytes 1,2: 0x%x 0x%x\n",ioStatusResp.servoSystemStatusAz[0],ioStatusResp.servoSystemStatusAz[1]);
*/

  azmotor1temp = (short) ioStatusResp.ServoAmpAz1status[0]-100;
  azmotor2temp = (short) ioStatusResp.ServoAmpAz2status[0]-100;

  azmotor3temp = (short) ioStatusResp.ServoAmpAz3status[0]-100;
  azmotor4temp = (short) ioStatusResp.ServoAmpAz4status[0]-100;

  elmotor1temp = (short) ioStatusResp.ServoAmpEl1status[0]-100;
  elmotor2temp = (short) ioStatusResp.ServoAmpEl2status[0]-100;
  elmotor3temp = (short) ioStatusResp.ServoAmpEl3status[0]-100;
  elmotor4temp = (short) ioStatusResp.ServoAmpEl4status[0]-100;

  az1motorcurrent = (short) ioStatusResp.Az1motorCurrent;
  az2motorcurrent = (short) ioStatusResp.Az2motorCurrent;
  el1motorcurrent = (short) ioStatusResp.El1motorCurrent;
  el2motorcurrent = (short) ioStatusResp.El2motorCurrent;
  el3motorcurrent = (short) ioStatusResp.El3motorCurrent;
  el4motorcurrent = (short) ioStatusResp.El4motorCurrent;

  az1motorcurrentF = (float)az1motorcurrent / 10.0;
  az2motorcurrentF = (float)az2motorcurrent / 10.0;
  el1motorcurrentF = (float)el1motorcurrent / 10.0;
  el2motorcurrentF = (float)el2motorcurrent / 10.0;
  el3motorcurrentF = (float)el3motorcurrent / 10.0;
  el4motorcurrentF = (float)el4motorcurrent / 10.0;

        sprintf(redisData,"HSET acu azMotor1Temp %h",azmotor1temp);
        redisResp = redisCommand(redisC,redisData);
        sprintf(redisData,"HSET acu azMotor2Temp %h",azmotor2temp);
        redisResp = redisCommand(redisC,redisData);
        sprintf(redisData,"HSET acu elMotor1Temp %h",elmotor1temp);
        redisResp = redisCommand(redisC,redisData);
        sprintf(redisData,"HSET acu elMotor2Temp %h",elmotor2temp);
        redisResp = redisCommand(redisC,redisData);
        sprintf(redisData,"HSET acu elMotor3Temp %h",elmotor3temp);
        redisResp = redisCommand(redisC,redisData);
        sprintf(redisData,"HSET acu elMotor4Temp %h",elmotor4temp);
        redisResp = redisCommand(redisC,redisData);

        sprintf(redisData,"HSET acu azMotor1Current %f",az1motorcurrentF);
        redisResp = redisCommand(redisC,redisData);
        sprintf(redisData,"HSET acu azMotor2Current %f",az2motorcurrentF);
        redisResp = redisCommand(redisC,redisData);
        sprintf(redisData,"HSET acu elMotor1Current %f",el1motorcurrentF);
        redisResp = redisCommand(redisC,redisData);
        sprintf(redisData,"HSET acu elMotor2Current %f",el2motorcurrentF);
        redisResp = redisCommand(redisC,redisData);
        sprintf(redisData,"HSET acu elMotor3Current %f",el3motorcurrentF);
        redisResp = redisCommand(redisC,redisData);
        sprintf(redisData,"HSET acu elMotor4Current %f",el4motorcurrentF);
        redisResp = redisCommand(redisC,redisData);


  } /* if recvBuff[0]  0x6 check */

  if (recvBuff[0]==0x2) {
  sprintf(acuErrorMessage,"ACU refuses the command from ACUstatus...reason:");
/*
  dsm_status = dsm_write(ACC,"DSM_ACU_ERROR_MESSAGE_C256",acuErrorMessage);
  if (dsm_status != DSM_SUCCESS) {
  printf("DSM write failed! DSM_ACU_ERROR_MESSAGE_C256 dsm_status=%d\n",dsm_status);
  }
*/
  sleep(2);
  if (recvBuff[1]==0x43) sprintf(acuErrorMessage,"Checksum error.\n");
  if (recvBuff[1]==0x45) sprintf(acuErrorMessage,"ETX not received at expected position.\n");
  if (recvBuff[1]==0x49) sprintf(acuErrorMessage,"Unknown identifier.\n");
  if (recvBuff[1]==0x4C) sprintf(acuErrorMessage,"Wrong length (incorrect no. of bytes rcvd.\n");
  if (recvBuff[1]==0x6C) sprintf(acuErrorMessage,"Specified length does not match identifier.\n");
  if (recvBuff[1]==0x4D) sprintf(acuErrorMessage,"Command ignored in present operating mode.\n");
  if (recvBuff[1]==0x6F) sprintf(acuErrorMessage,"Other reasons.\n");
  if (recvBuff[1]==0x52) sprintf(acuErrorMessage,"Device not in Remote mode.\n");
  if (recvBuff[1]==0x72) sprintf(acuErrorMessage,"Value out of range.\n");
  if (recvBuff[1]==0x53) sprintf(acuErrorMessage,"Missing STX.\n");
  } else {sprintf(acuErrorMessage,"");}

/*
  dsm_status = dsm_write(ACC,"DSM_ACU_ERROR_MESSAGE_C256",acuErrorMessage);
  if (dsm_status != DSM_SUCCESS) {
  printf("DSM write failed! DSM_ACU_ERROR_MESSAGE_C256 dsm_status=%d\n",dsm_status);
  }
*/

        usleep(3000000);
	} /* while loop */
/*  close(sockfdMonitor);*/

	pthread_detach(ACUiostatusTID);
	pthread_exit((void *) 0);
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


/* This function reports the status of the self test.
Only one pass of inquiry is made- repeated use with up-arrow
can show the updates. 
Nimesh Patel
*/

void *ACUselftest() {
  int n = 0;
  int loopTime=10000000;
  char recvBuff[256];
  char sendBuff[256];
  int i;
  acuCmd acuCommand;
  selfTestResults acuStResult;
  float selfTestValue=0.0;
  char selfTestValueBuff[4];

  acuCommand.stx = 0x2;
  acuCommand.id = 0x7A;
  acuCommand.datalength = 0x7;
  acuCommand.checksum = 0x81;
  acuCommand.etx = 0x3;

  memset(recvBuff, '0' ,sizeof(recvBuff));
  memset(sendBuff, '0' ,sizeof(sendBuff));

  memcpy(sendBuff,(char*)&acuCommand,sizeof(acuCommand));

  while(1) {

  n = send(sockfdMonitor,sendBuff,sizeof(acuCommand),0);

  if (n<0) printf("ERROR writing to ACU. From ACUselftest");
 
  /* receive the ACK response from ACU */
  n = recv(sockfdMonitor, recvBuff, sizeof(acuStResult),0); 

  if( n < 0)  printf("\n Read Error from ACUselftest\n"); 

  /* check if ACK is received, then receive the response and parse it */

  if (recvBuff[0]==0x6) {

  n = recv(sockfdMonitor, (char *)&acuStResult, sizeof(acuStResult),0);

  printf("Read %d bytes from ACU\n",n);
  printf ("Received %d bytes from ACU.\n",acuStResult.datalength);
  printf("Self Test Status:  %c\n",acuStResult.status);
  if(acuStResult.status=='A') printf("...Active...\n");
  
  if(acuStResult.status=='A') {
     printf("Current test:  %d\n",acuStResult.currentTest);
        redisWriteInt("acu","currentSelfTest",acuStResult.currentTest);
     printf("Number of failed tests:  %d\n",acuStResult.numOfFailedTests);
        redisWriteInt("acu","numOfFailedTests",acuStResult.numOfFailedTests);
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
   loopTime=3000000; /* change loop time to 2 sec */
   } else {
   loopTime=15000000; /* change loop time to 10 sec */
   }
  if(acuStResult.status=='C') printf("...Completed...\n");
  if(acuStResult.status=='I') printf("...Inactive...\n");
  if(acuStResult.status=='X') printf("...Aborted...\n");
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
  

  usleep(loopTime);
 
  }

	pthread_detach(ACUselftestTID);
	pthread_exit((void *) 0);
}
