#define DEBUG 1
#include <stdio.h>
#include <stdlib.h>
#include <termio.h>
#include <time.h>
#include <math.h>
#include <curses.h>
#include "dsm.h"
#include "hiredis.h"
#define DSM_HOST "gltacc"
#define DSM_SUCCESS 0
#define MICRODEG_TO_ARCSEC 0.0036

extern char redisData[1024];
extern redisContext *redisC;
extern redisReply *redisResp;
extern void redisTSADD(char *fieldName, double variable);

void metrology() {

  time_t timestamp,system_time;
  char timeString[26];
  float seconds;
  struct tm* systemTime;
  int dsm_status;
  int tilt1x,tilt1y,tilt2x,tilt2y,tilt3x,tilt3y;
  int tilt1Temp,tilt2Temp,tilt3Temp;
  int linearSensor1,linearSensor2,linearSensor3,linearSensor4;
  int tiltAzCorr,tiltElCorr;
  int linearSensorAzCorr,linearSensorElCorr;
  int SPEMazCorr,SPEMelCorr;
  short tempSensor[41];
  short azm1t,azm2t,elm1t,elm2t,elm3t,elm4t;
  float temperature,pressure,humidity,windspeed,winddir;

    dsm_status = dsm_open();
  if(dsm_status != DSM_SUCCESS) {
    dsm_error_message(dsm_status, "dsm_open");
    exit(-1);
  }


/*
  system_time=time(NULL);
  redisTSADD("Metrology data at time: %s ",ctime(&system_time));
*/
  dsm_status = dsm_read(DSM_HOST,"DSM_MET_TEMP_SENSOR_V41_S",tempSensor,&timestamp);
  if (dsm_status != DSM_SUCCESS) {
  printf("Warning: DSM read failed! DSM_MET_TEMP_SENSOR_V41_S dsm_status=%d\n",dsm_status);
  }

  dsm_status = dsm_read(DSM_HOST,"DSM_SPEM_AZCORR_L",&SPEMazCorr,&timestamp);

  dsm_status = dsm_read(DSM_HOST,"DSM_SPEM_AZCORR_L",&SPEMazCorr,&timestamp);
  if (dsm_status != DSM_SUCCESS) {
  redisTSADD("Warning: DSM read failed! DSM_SPEM_AZCORR_L dsm_status=%d\n",dsm_status);
  }
  dsm_status = dsm_read(DSM_HOST,"DSM_SPEM_ELCORR_L",&SPEMelCorr,&timestamp);
  if (dsm_status != DSM_SUCCESS) {
  redisTSADD("Warning: DSM read failed! DSM_SPEM_ELCORR_L dsm_status=%d\n",dsm_status);
  }
  dsm_status = dsm_read(DSM_HOST,"DSM_LINEAR_SENSOR_AZCORR_L",&linearSensorAzCorr,&timestamp);
  if (dsm_status != DSM_SUCCESS) {
  redisTSADD("Warning: DSM read failed! DSM_LINEAR_SENSOR_AZCORR_L dsm_status=%d\n",dsm_status);
  }
  dsm_status = dsm_read(DSM_HOST,"DSM_LINEAR_SENSOR_ELCORR_L",&linearSensorElCorr,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_TILTAZCORR_L",&tiltAzCorr,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_TILTELCORR_L",&tiltElCorr,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_LINEAR_SENSOR1_L",&linearSensor1,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_LINEAR_SENSOR2_L",&linearSensor2,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_LINEAR_SENSOR3_L",&linearSensor3,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_LINEAR_SENSOR4_L",&linearSensor4,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_TILT1TEMP_L",&tilt1Temp,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_TILT2TEMP_L",&tilt2Temp,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_TILT3TEMP_L",&tilt3Temp,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_TILT1X_L",&tilt1x,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_TILT1Y_L",&tilt1y,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_TILT2X_L",&tilt2x,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_TILT2Y_L",&tilt2y,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_TILT3X_L",&tilt3x,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_TILT3Y_L",&tilt3y,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_AZ_MOTOR1_TEMP_S",&azm1t,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_AZ_MOTOR2_TEMP_S",&azm2t,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_EL_MOTOR1_TEMP_S",&elm1t,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_EL_MOTOR2_TEMP_S",&elm2t,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_EL_MOTOR3_TEMP_S",&elm3t,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_EL_MOTOR4_TEMP_S",&elm4t,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_WEATHER_TEMP_C_F",&temperature,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_WEATHER_PRESS_MBAR_F",&pressure,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_WEATHER_HUMIDITY_F",&humidity,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_WEATHER_WINDSPEED_MPS_F",&windspeed,&timestamp);
  dsm_status = dsm_read(DSM_HOST,"DSM_WEATHER_WINDDIR_AZDEG_F",&winddir,&timestamp);


  dsm_status = dsm_close();
  if(dsm_status != DSM_SUCCESS) {
      dsm_error_message(dsm_status, "dsm_close");
      exit(-1);
      }

  redisTSADD("met.temperature",temperature);
  redisTSADD("met.pressure",pressure);
  redisTSADD("met.humidity",humidity);
  redisTSADD("met.windspeed",windspeed);
  redisTSADD("met.winddir",winddir);

  redisTSADD("met.tilt1x",(float)tilt1x * MICRODEG_TO_ARCSEC);
  redisTSADD("met.tilt1y",(float)tilt1y * MICRODEG_TO_ARCSEC);
  redisTSADD("met.tilt2x",(float)tilt2x * MICRODEG_TO_ARCSEC);
  redisTSADD("met.tilt2y",(float)tilt2y * MICRODEG_TO_ARCSEC);
  redisTSADD("met.tilt3x",(float)tilt3x * MICRODEG_TO_ARCSEC);
  redisTSADD("met.tilt3y",(float)tilt3y * MICRODEG_TO_ARCSEC);

  redisTSADD("met.tilt1Temp",(float)tilt1Temp/100.);
  redisTSADD("met.tilt2Temp",(float)tilt2Temp/100.);
  redisTSADD("met.tilt3Temp",(float)tilt3Temp/100.);

  redisTSADD("met.linearSensor1",(float)linearSensor1*0.1);
  redisTSADD("met.linearSensor2",(float)linearSensor2*0.1);
  redisTSADD("met.linearSensor3",(float)linearSensor3*0.1);
  redisTSADD("met.linearSensor4",(float)linearSensor4*0.1);

  redisTSADD("met.tiltAzCorr",(float)tiltAzCorr);
  redisTSADD("met.tiltElCorr",(float)tiltElCorr);

  redisTSADD("met.spemAzCorr",(float)SPEMazCorr);
  redisTSADD("met.spemElCorr",(float)SPEMelCorr);

  redisTSADD("met.tempSensor0",(float)tempSensor[0]/100.);
  redisTSADD("met.tempSensor1",(float)tempSensor[1]/100.);
  redisTSADD("met.tempSensor2",(float)tempSensor[2]/100.);
  redisTSADD("met.tempSensor3",(float)tempSensor[3]/100.);
  redisTSADD("met.tempSensor4",(float)tempSensor[4]/100.);
  redisTSADD("met.tempSensor5",(float)tempSensor[5]/100.);
  redisTSADD("met.tempSensor6",(float)tempSensor[6]/100.);
  redisTSADD("met.tempSensor7",(float)tempSensor[7]/100.);
  redisTSADD("met.tempSensor8",(float)tempSensor[8]/100.);
  redisTSADD("met.tempSensor9",(float)tempSensor[9]/100.);
  redisTSADD("met.tempSensor10",(float)tempSensor[10]/100.);
  redisTSADD("met.tempSensor11",(float)tempSensor[11]/100.);
  redisTSADD("met.tempSensor12",(float)tempSensor[12]/100.);
  redisTSADD("met.tempSensor13",(float)tempSensor[13]/100.);
  redisTSADD("met.tempSensor14",(float)tempSensor[14]/100.);
  redisTSADD("met.tempSensor15",(float)tempSensor[15]/100.);
  redisTSADD("met.tempSensor16",(float)tempSensor[16]/100.);
  redisTSADD("met.tempSensor17",(float)tempSensor[17]/100.);
  redisTSADD("met.tempSensor18",(float)tempSensor[18]/100.);
  redisTSADD("met.tempSensor19",(float)tempSensor[19]/100.);
  redisTSADD("met.tempSensor20",(float)tempSensor[20]/100.);
  redisTSADD("met.tempSensor21",(float)tempSensor[21]/100.);
  redisTSADD("met.tempSensor22",(float)tempSensor[22]/100.);
  redisTSADD("met.tempSensor23",(float)tempSensor[23]/100.);
  redisTSADD("met.tempSensor24",(float)tempSensor[24]/100.);
  redisTSADD("met.tempSensor25",(float)tempSensor[25]/100.);
  redisTSADD("met.tempSensor26",(float)tempSensor[26]/100.);
  redisTSADD("met.tempSensor27",(float)tempSensor[27]/100.);
  redisTSADD("met.tempSensor28",(float)tempSensor[28]/100.);
  redisTSADD("met.tempSensor29",(float)tempSensor[29]/100.);
  redisTSADD("met.tempSensor30",(float)tempSensor[30]/100.);
  redisTSADD("met.tempSensor31",(float)tempSensor[31]/100.);
  redisTSADD("met.tempSensor32",(float)tempSensor[32]/100.);
  redisTSADD("met.tempSensor33",(float)tempSensor[33]/100.);
  redisTSADD("met.tempSensor34",(float)tempSensor[34]/100.);
  redisTSADD("met.tempSensor35",(float)tempSensor[35]/100.);
  redisTSADD("met.tempSensor36",(float)tempSensor[36]/100.);
  redisTSADD("met.tempSensor37",(float)tempSensor[37]/100.);
  redisTSADD("met.tempSensor38",(float)tempSensor[38]/100.);
  redisTSADD("met.tempSensor39",(float)tempSensor[39]/100.);
  redisTSADD("met.tempSensor40",(float)tempSensor[40]/100.);
  redisTSADD("met.tempSensor41",(float)tempSensor[41]/100.);
  redisTSADD("met.motTempAz1",(float)azm1t);
  redisTSADD("met.motTempAz2",(float)azm2t);
  redisTSADD("met.motTempEl1",(float)elm1t);
  redisTSADD("met.motTempEl2",(float)elm2t);
  redisTSADD("met.motTempEl3",(float)elm3t);
  redisTSADD("met.motTempEl4",(float)elm4t);
}
