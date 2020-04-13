//Copyright 2020 by Warren Watson
// Version 02-Feb-20
//
// use following lines for early definitions of multiple partition configuration in uSDFS.h
#include "Arduino.h"
#include "uSDFS.h"
#include "MassStorage.h"
#include "msc.h"
#include "mscfs.h"

// For this sketch we need to define USE_EXTENAL_INIT in
// MassStorage.h. Explained there.

#define MY_VOL_TO_PART
#if FF_MULTI_PARTITION		/* Multiple partition configuration */ 
	PARTITION VolToPart[] = {{DEV_SPI, 0}, //{ physical drive number, Partition: 0:Auto detect, 1-4:Forced partition)} 
							 {DEV_SDHC,0}, 
							 {DEV_USB, 0}, 
							 {DEV_USB, 1}, 
							 {DEV_USB, 2},
							 {DEV_USB1,0},
							 {DEV_USB1,1},
							 {DEV_USB1,2}
							 }; /* Volume - Partition resolution table */
#endif
// end of early definition

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
USBHub hub3(myusb);
USBHub hub4(myusb);

msDriveInfo_t *driveInfo;
extern TCHAR driveLabel[4][64];	// Pointer to a buffer to return the volume driveLabel
extern DWORD serialNum[4];	 	// Pointer to a variable to return the volume serial number

FRESULT mres;

char buff[256];

char *getString(char *strbuf) {
	int i=0;
	while(1) {
		while (!Serial.available());
			strbuf[i] = Serial.read();
			if (strbuf[i] == '\n') {
				strbuf[i] = '\0';
				break;
			}
		i++;
	}
	return strbuf;
}

void setup() {
	while(!Serial);

	Serial.printf("MSC MULTI USB DRIVE FILE LISTING TEST\n\n");
	Serial.printf("NOTE: If you are using a HDD drive with a USB to SATA adapter\n");
	Serial.printf("      it can take a few seconds to connect on power up.\n");
	Serial.printf("      Also, 'USE_EXTERNAL_INIT' must be defined in\n");
	Serial.printf("      MassStorage.h for this sketch to compile with my\n");
	Serial.printf("      version of uSDFS.\n\n");
	Serial.printf("Plug in at least one USB Mass Storage devices (If not already plugged in).\n\n"); 
	
	// Initialize USB HOST
	myusb.begin();
	// Initialize MSC File System
	MSCFS_init();
	Serial.printf("Getting labels and serial numbers for all connected drives\n");
	getDriveLabels(255);

}

void loop() {
			
	if((strcmp(driveLabel[2], "UnAvalialble") == 0) || (strcmp(driveLabel[3], "UnAvalialble") == 0))
		getDriveLabels(255);
	Serial.printf("\nEnter a directory path name to list.\n");
	Serial.printf("Or just press <ENTER> key to list root directory\n");
	Serial.printf("on the default drive\n");
	Serial.printf(" Examples:\n");
	Serial.printf("    2:/\n");
	Serial.printf("    2:/<directory name>\n");    
	Serial.printf("    2:/<directory name/filename\n");    
	Serial.printf("    2:/<filename>\n");    
	Serial.printf("    2:/<??.*>\n");    
	Serial.printf("    2:/<*.dat>\n");    
	Serial.printf("    etc...\n");    

	Serial.printf("Avalable drives are:\n");    

	Serial.printf("\nDrive %s (SDHC) %s\n",getDriveName(1),driveLabel[1]); // Actually SDHC card.
	Serial.printf("Drive %s (USB0) %s\n",getDriveName(2),driveLabel[2]);
	Serial.printf("Drive %s (USB1) %s\n",getDriveName(3), driveLabel[3]);
	Serial.printf("Default drive is: %s, %s\n", getDriveName(getCurrentDrive()),driveLabel[getCurrentDrive()]);
	getString(buff);

	Serial.printf("----------------------------------------------------------\n");
	Serial.printf("Directory listing for %s\n", buff);
	lsdir(buff);
	Serial.printf("----------------------------------------------------------\n");
}
