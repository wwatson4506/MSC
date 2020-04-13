//Copyright 2020 by Warren Watson
// Version 02-Feb-20
//
// use following lines for early definitions of multiple partition configuration in uSDFS.h
#include "Arduino.h"
#include "uSDFS.h"
#include "MassStorage.h"
#include "msc.h"
#include "mscfs.h"

// use following lines for early definitions of multiple partition configuration in uSDFS.h
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

// For this sketch we need to define USE_EXTENAL_INIT in
// MassStorage.h. Explained there.
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
USBHub hub3(myusb);
USBHub hub4(myusb);

msDriveInfo_t *driveInfo;

extern TCHAR driveLabel[4][64];	// Pointer to a buffer to return the volume driveLabel
extern DWORD serialNum[4];	 	// Pointer to a variable to return the volume serial number

FRESULT mres;

const char *D1 = "1:/32MEGfile.dat";  // SDHC
const char *D2 = "2:/32MEGfile.dat";  // USB
const char *D3 = "3:/32MEGfile.dat";  // USB1


bool D0available = false;
bool D1available = false;
bool D2available = false;
bool D3available = false;

uint8_t c = 0;
void setup() {
	while(!Serial);

	Serial.printf("MSC MULTI USB DRIVE FILE COPY TEST\n\n");
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
	Serial.printf("\nDrive %s (SDHC) %s\n",getDriveName(1),driveLabel[1]); // Actually SDHC card.
	Serial.printf("Drive %s (USB0) %s\n",getDriveName(2),driveLabel[2]);
	Serial.printf("Drive %s (USB1) %s\n",getDriveName(3), driveLabel[3]);

	Serial.printf("\nSelect:\n");
	Serial.printf("    1) to copy '32MEGfile.dat' from drive 2:/ to drive 3:/.\n");
	Serial.printf("    2) to copy '32MEGfile.dat' from drive 3:/ to drive 2:/.\n");
	Serial.printf("    3) to copy '32MEGfile.dat' from drive 2:/ to drive 1:/.\n");    
	Serial.printf("    4) to copy '32MEGfile.dat' from drive 1:/ to drive 2:/.\n");    
	Serial.printf("    5) to copy '32MEGfile.dat' from drive 3:/ to drive 1:/.\n");    
	Serial.printf("    6) to copy '32MEGfile.dat' from drive 1:/ to drive 3:/.\n");    

	while(!Serial.available());
	c = Serial.read();
	if(Serial.available()) Serial.read(); // Get rid of CR and/or LF if there.

	switch(c) {
		case '1':
			if(!driveAvailable(2)) { // Is the drive available and mounted?
				Serial.printf("\nUSB drive %s is not available or not mounted\n",getDriveName(2));
				break;
			}
			if(!driveAvailable(3)) { // Is the drive available and mounted?
				Serial.printf("\nUSB drive %s is not available or not mounted\n",getDriveName(3));
				break;
			}	
			if((strcmp(driveLabel[2], "UnAvalialble") == 0) || (strcmp(driveLabel[3], "UnAvalialble") == 0))
				getDriveLabels(255);
			Serial.printf("\n1) Copying from %s to %s\n", driveLabel[2],driveLabel[3]);
			mres = fileCopy(D2,D3,true); // D2 is source, D3 is destination
										 // Third parameter is stats flag.
										 // true: show progress bar and total
										 //       copy time and speed.
										 // false: Do silent copy.
			if(mres != 0) {
				Serial.printf("File Copy Failed with code: %d\n",mres);
			}
			break;
		case '2':
			if(!driveAvailable(3)) { // Is the drive available and mounted?
				Serial.printf("\nUSB drive %s is not available or not mounted\n",getDriveName(3));
				break;
			}	
			if(!driveAvailable(2)) { // Is the drive available and mounted?
				Serial.printf("\nUSB drive %s is not available or not mounted\n",getDriveName(2));
				break;
			}	
			if((strcmp(driveLabel[2], "UnAvalialble") == 0) || (strcmp(driveLabel[3], "UnAvalialble") == 0))
				getDriveLabels(255);
			Serial.printf("\n2) Copying from %s to %s\n", driveLabel[3],driveLabel[2]);
			mres = fileCopy(D3,D2,true);
			if(mres != 0) {
				Serial.printf("File Copy Failed with code: %d\n",mres);
			}
			break;
		case '3':
			if(!driveAvailable(2)) { // Is the drive available and mounted?
				Serial.printf("\nUSB drive %s is not available or not mounted\n",getDriveName(2));
				break;
			}	
			if(strcmp(driveLabel[2], "UnAvalialble") == 0)
				getDriveLabels(255);
			Serial.printf("\n3) Copying from %s to %s\n", driveLabel[2],driveLabel[1]);
			mres = fileCopy(D2,D1,true);
			if(mres != 0) {
				Serial.printf("File Copy Failed with code: %d\n",mres);
			}
			break;
		case '4':
			if(!driveAvailable(2)) { // Is the drive available and mounted?
				Serial.printf("\nUSB drive %s is not available or not mounted\n",getDriveName(2));
				break;
			}
			if(strcmp(driveLabel[2], "UnAvalialble") == 0)
				getDriveLabels(255);
			Serial.printf("\n4) Copying from %s to %s\n", driveLabel[1],driveLabel[2]);
			mres = fileCopy(D1,D2,true);
			if(mres != 0) {
				Serial.printf("File Copy Failed with code: %d\n",mres);
			}
			break;
		case '5':
			if(!driveAvailable(3)) { // Is the drive available and mounted?
				Serial.printf("\nUSB drive %s is not available or not mounted\n",getDriveName(3));
				break;
			}
			if(strcmp(driveLabel[3], "UnAvalialble") == 0)
				getDriveLabels(255);
			Serial.printf("\n5) Copying from %s to %s\n", driveLabel[3],driveLabel[1]);
			mres = fileCopy(D3,D1,true);
			if(mres != 0) {
				Serial.printf("File Copy Failed with code: %d\n",mres);
			}
			break;
		case '6':
			if(!driveAvailable(3)) { // Is the drive available and mounted?
				Serial.printf("\nUSB drive %s is not available or not mounted\n",getDriveName(3));
				break;
			}	
			if(strcmp(driveLabel[3], "UnAvalialble") == 0)
				getDriveLabels(255);
			Serial.printf("\n6) Copying from %s to %s\n", driveLabel[1],driveLabel[3]);
			mres = fileCopy(D1,D3,true);
			if(mres != 0) {
				Serial.printf("File Copy Failed with code: %d\n",mres);
			}
			break;
		default:
			break;
	}
	getDriveLabels(255);
}
