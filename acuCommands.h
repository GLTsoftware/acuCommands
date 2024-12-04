typedef struct __attribute__ ((packed)) acuCmd {
    char stx;
    char id;
    short datalength;
    short checksum;
    char etx;
} acuCmd; 


typedef struct __attribute__ ((packed)) acuModeCmd {
    char stx;
    char id;
    short datalength;
    char azMode;
    char elMode;
    char polMode;
    short controlWord;
    short checksum;
    char etx;
} acuModeCmd; 

typedef struct __attribute__ ((packed)) acuAuxCmd {
    char stx;
    char id;
    short datalength;
    char azMode;
    char elMode;
    short checksum;
    char etx;
} acuAuxCmd; 

typedef struct __attribute__ ((packed)) acuAzElCmd {
    char stx;
    char id;
    short datalength;
    int cmdAz __attribute__ ((packed));
    int cmdEl __attribute__ ((packed));
    int cmdPol __attribute__ ((packed));
    short checksum;
    char etx;
} acuAzElCmd; 

typedef struct __attribute__ ((packed)) acuAzElProg {
    char stx;
    char id;
    short datalength;
    short clearstack;
    int timeOfDay;
    short dayOfYear;
    int cmdAz;
    int cmdEl;
    short checksum;
    char etx;
} acuAzElProg; 

typedef struct __attribute__ ((packed)) acuAzElOffsetCmd {
    char stx;
    char id;
    short datalength;
    int timeOffset __attribute__ ((packed));
    int azOffset __attribute__ ((packed));
    int elOffset __attribute__ ((packed));
    int polOffset __attribute__ ((packed));
    short checksum;
    char etx;
} acuAzElOffsetCmd;

typedef struct __attribute__ ((packed)) acuTwoLineCmd {
    char stx;
    char id;
    short datalength;
    char line0[24];
    char line1[69];
    char line2[69];
    short checksum;
    char etx;
} acuTwoLineCmd; 
