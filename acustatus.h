  struct {
      char stx;
      char id;
      short datalength;
      int timeOfDay;
      int dayOfYear;
      int azPos;
      int elPos;
      int polPos;
      int cmdAzPos;
      int cmdElPos;
      int cmdPolPos;
      char azStatusMode;
      char elStatusMode;
      char polStatusMode;
      char arrayStatusMode;
      short servoSystemStatusAz;
      short spare1;
      short servoSystemStatusEl;
      short spare2;
      short servoSystemStatusPol;
      short spare3;
      char servoSystemGS1;
      char servoSystemGS2;
      char servoSystemGS3;
      char servoSystemGS4;
      char servoSystemGS5;
      char servoSystemGS6;
      int timeOffset;
      int positionOffsetAz;
      int positionOffsetEl;
      int positionOffsetPol;
      short freeTrackStackPosn;
      short pointingErrorCorrStatus;
      short spare4;
      int lubricationStatus;
      int srXpos;
      int srYpos;
      int srZpos;
      int srTxpos;
      int srTypos;
      int srXposErr;
      int srYposErr;
      int srZposErr;
      int srTxposErr;
      int srTyposErr;
      int srXposOffset;
      int srYposOffset;
      int srZposOffset;
      int srTxposOffset;
      int srTyposOffset;
      int HPCstatus;
      short srJackStatus1;
      short srJackStatus2;
      short srJackStatus3;
      short srJackStatus4;
      short srJackStatus5;
      short srJackStatus6;
      short srCaxisStatus;
      int srGeneralStatus;
      char srMode;
      short spare 5;
      short checksum;
      char etx;
  } acuStatus;
