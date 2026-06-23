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
extern void redisWriteShort(char *hashName, char *fieldName, short variable);
extern void redisWriteInt(char *hashName, char *fieldName, int variable);
extern void redisWriteFloat(char *hashName, char *fieldName,float variable);
extern void redisWriteDouble(char *hashName, char *fieldName, double variable);
extern void redisWriteString(char *hashName, char *fieldName, char *variable);


void metrology() {

  time_t timestamp;
  static time_t last_minute_update = 0;
  /* FIX: dsm_open/close moved to a static flag so DSM is opened once,
     not on every 1-second call. */
  static int dsm_initialized = 0;
  time_t system_time = time(NULL);
  int dsm_status;

  /* FIX: zero-initialize all variables so that if a dsm_read fails and
     leaves a variable untouched, we write 0 to Redis instead of garbage.
     This is what was causing all the valgrind "uninitialised value" errors:
     the stack values were indeterminate on first entry. */
  int tilt1x=0,tilt1y=0,tilt2x=0,tilt2y=0,tilt3x=0,tilt3y=0;
  int tilt1Temp=0,tilt2Temp=0,tilt3Temp=0;
  int linearSensor1=0,linearSensor2=0,linearSensor3=0,linearSensor4=0;
  int tiltAzCorr=0,tiltElCorr=0;
  int linearSensorAzCorr=0,linearSensorElCorr=0;
  int SPEMazCorr=0,SPEMelCorr=0;
  short tempSensor[42];
  short azm1t=0,azm2t=0,elm1t=0,elm2t=0,elm3t=0,elm4t=0;
  float azm1c=0.,azm2c=0.,elm1c=0.,elm2c=0.,elm3c=0.,elm4c=0.;
  float temperature=0.,pressure=0.,humidity=0.,windspeed=0.,winddir=0.;
  double actualAz=0.,actualEl=0.;
  int acuDayOfYear=0,acuHour=0;
  int i;

  /* zero-init the whole tempSensor array */
  for(i=0;i<42;i++) tempSensor[i]=0;

  /* FIX: open DSM once at program start, not every second.
     Previously dsm_open() was called on every metrology() call (every 1 s)
     and dsm_close() on every return — extremely wasteful and fragile. */
  if (!dsm_initialized) {
    dsm_status = dsm_open();
    if(dsm_status != DSM_SUCCESS) {
      dsm_error_message(dsm_status, "dsm_open");
      exit(-1);
    }
    dsm_initialized = 1;
  }

  /* fast (1-second) updates: az/el position and time */
  dsm_status = dsm_read(DSM_HOST,"DSM_ACTUAL_AZ_DEG_D",&actualAz,&timestamp);
  if(dsm_status != DSM_SUCCESS)
    printf("Warning: DSM read failed! DSM_ACTUAL_AZ_DEG_D dsm_status=%d\n",dsm_status);
  dsm_status = dsm_read(DSM_HOST,"DSM_ACTUAL_EL_DEG_D",&actualEl,&timestamp);
  if(dsm_status != DSM_SUCCESS)
    printf("Warning: DSM read failed! DSM_ACTUAL_EL_DEG_D dsm_status=%d\n",dsm_status);
  redisWriteDouble("acu","azPosn",actualAz);
  redisWriteDouble("acu","elPosn",actualEl);

  dsm_status = dsm_read(DSM_HOST,"DSM_ACU_HOUR_L",&acuHour,&timestamp);
  if(dsm_status != DSM_SUCCESS)
    printf("Warning: DSM read failed! DSM_ACU_HOUR_L dsm_status=%d\n",dsm_status);
  dsm_status = dsm_read(DSM_HOST,"DSM_ACU_DAYOFYEAR_L",&acuDayOfYear,&timestamp);
  if(dsm_status != DSM_SUCCESS)
    printf("Warning: DSM read failed! DSM_ACU_DAYOFYEAR_L dsm_status=%d\n",dsm_status);
  redisWriteInt("acu","hour",(int)acuHour);
  redisWriteInt("acu","dayOfYear",(int)acuDayOfYear);

  /* slow (1-minute) updates: metrology time series */
  if(system_time - last_minute_update >= 60) {
    last_minute_update = system_time;

    dsm_status = dsm_read(DSM_HOST,"DSM_MET_TEMP_SENSOR_V41_S",tempSensor,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_MET_TEMP_SENSOR_V41_S dsm_status=%d\n",dsm_status);

    /* FIX: DSM_SPEM_AZCORR_L was read twice; second read was supposed to
       be DSM_SPEM_ELCORR_L (reading SPEMelCorr). Fixed below. */
    dsm_status = dsm_read(DSM_HOST,"DSM_SPEM_AZCORR_L",&SPEMazCorr,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_SPEM_AZCORR_L dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_SPEM_ELCORR_L",&SPEMelCorr,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_SPEM_ELCORR_L dsm_status=%d\n",dsm_status);

    dsm_status = dsm_read(DSM_HOST,"DSM_LINEAR_SENSOR_AZCORR_L",&linearSensorAzCorr,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_LINEAR_SENSOR_AZCORR_L dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_LINEAR_SENSOR_ELCORR_L",&linearSensorElCorr,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_LINEAR_SENSOR_ELCORR_L dsm_status=%d\n",dsm_status);

    dsm_status = dsm_read(DSM_HOST,"DSM_TILTAZCORR_L",&tiltAzCorr,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_TILTAZCORR_L dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_TILTELCORR_L",&tiltElCorr,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_TILTELCORR_L dsm_status=%d\n",dsm_status);

    dsm_status = dsm_read(DSM_HOST,"DSM_LINEAR_SENSOR1_L",&linearSensor1,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_LINEAR_SENSOR1_L dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_LINEAR_SENSOR2_L",&linearSensor2,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_LINEAR_SENSOR2_L dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_LINEAR_SENSOR3_L",&linearSensor3,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_LINEAR_SENSOR3_L dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_LINEAR_SENSOR4_L",&linearSensor4,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_LINEAR_SENSOR4_L dsm_status=%d\n",dsm_status);

    dsm_status = dsm_read(DSM_HOST,"DSM_TILT1TEMP_L",&tilt1Temp,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_TILT1TEMP_L dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_TILT2TEMP_L",&tilt2Temp,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_TILT2TEMP_L dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_TILT3TEMP_L",&tilt3Temp,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_TILT3TEMP_L dsm_status=%d\n",dsm_status);

    dsm_status = dsm_read(DSM_HOST,"DSM_TILT1X_L",&tilt1x,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_TILT1X_L dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_TILT1Y_L",&tilt1y,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_TILT1Y_L dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_TILT2X_L",&tilt2x,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_TILT2X_L dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_TILT2Y_L",&tilt2y,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_TILT2Y_L dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_TILT3X_L",&tilt3x,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_TILT3X_L dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_TILT3Y_L",&tilt3y,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_TILT3Y_L dsm_status=%d\n",dsm_status);

    dsm_status = dsm_read(DSM_HOST,"DSM_AZ_MOTOR1_TEMP_S",&azm1t,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_AZ_MOTOR1_TEMP_S dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_AZ_MOTOR2_TEMP_S",&azm2t,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_AZ_MOTOR2_TEMP_S dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_EL_MOTOR1_TEMP_S",&elm1t,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_EL_MOTOR1_TEMP_S dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_EL_MOTOR2_TEMP_S",&elm2t,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_EL_MOTOR2_TEMP_S dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_EL_MOTOR3_TEMP_S",&elm3t,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_EL_MOTOR3_TEMP_S dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_EL_MOTOR4_TEMP_S",&elm4t,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_EL_MOTOR4_TEMP_S dsm_status=%d\n",dsm_status);

    dsm_status = dsm_read(DSM_HOST,"DSM_AZ_MOTOR1_CURRENT_F",&azm1c,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_AZ_MOTOR1_CURRENT_F dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_AZ_MOTOR2_CURRENT_F",&azm2c,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_AZ_MOTOR2_CURRENT_F dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_EL_MOTOR1_CURRENT_F",&elm1c,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_EL_MOTOR1_CURRENT_F dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_EL_MOTOR2_CURRENT_F",&elm2c,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_EL_MOTOR2_CURRENT_F dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_EL_MOTOR3_CURRENT_F",&elm3c,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_EL_MOTOR3_CURRENT_F dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_EL_MOTOR4_CURRENT_F",&elm4c,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_EL_MOTOR4_CURRENT_F dsm_status=%d\n",dsm_status);

    /* FIX: DSM_WEATHER_TEMP_C_F was read twice; removed duplicate. */
    dsm_status = dsm_read(DSM_HOST,"DSM_WEATHER_TEMP_C_F",&temperature,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_WEATHER_TEMP_C_F dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_WEATHER_PRESS_MBAR_F",&pressure,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_WEATHER_PRESS_MBAR_F dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_WEATHER_HUMIDITY_F",&humidity,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_WEATHER_HUMIDITY_F dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_WEATHER_WINDSPEED_MPS_F",&windspeed,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_WEATHER_WINDSPEED_MPS_F dsm_status=%d\n",dsm_status);
    dsm_status = dsm_read(DSM_HOST,"DSM_WEATHER_WINDDIR_AZDEG_F",&winddir,&timestamp);
    if (dsm_status != DSM_SUCCESS)
      printf("Warning: DSM read failed! DSM_WEATHER_WINDDIR_AZDEG_F dsm_status=%d\n",dsm_status);

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

    /* tempSensor[0..41] — array declared as short[42], DSM variable is V41
       (41 elements, indices 0-40); index 41 is written defensively as 0
       since it is out of the DSM variable's range. */
    redisTSADD("met.tempSensor0", (float)tempSensor[0]/100.);
    redisTSADD("met.tempSensor1", (float)tempSensor[1]/100.);
    redisTSADD("met.tempSensor2", (float)tempSensor[2]/100.);
    redisTSADD("met.tempSensor3", (float)tempSensor[3]/100.);
    redisTSADD("met.tempSensor4", (float)tempSensor[4]/100.);
    redisTSADD("met.tempSensor5", (float)tempSensor[5]/100.);
    redisTSADD("met.tempSensor6", (float)tempSensor[6]/100.);
    redisTSADD("met.tempSensor7", (float)tempSensor[7]/100.);
    redisTSADD("met.tempSensor8", (float)tempSensor[8]/100.);
    redisTSADD("met.tempSensor9", (float)tempSensor[9]/100.);
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
    /* FIX: removed duplicate block that re-sent motTempAz2..motTempEl4 */

    redisTSADD("met.motCurrentAz1",azm1c);
    redisTSADD("met.motCurrentAz2",azm2c);
    redisTSADD("met.motCurrentEl1",elm1c);
    redisTSADD("met.motCurrentEl2",elm2c);
    redisTSADD("met.motCurrentEl3",elm3c);
    redisTSADD("met.motCurrentEl4",elm4c);

  } /* end 60-second update */

  /* FIX: dsm_close() removed from here; DSM stays open for the program's
     lifetime. If a controlled shutdown is ever needed, call dsm_close()
     from a signal handler in main, not here. */
}
