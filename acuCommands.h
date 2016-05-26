/* In the following, id, datalength and checksum are set
for each command as needed */

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
