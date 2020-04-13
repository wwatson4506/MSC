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

uint8_t c = 0;
void setup() {
	while(!Serial);
	Serial.printf("MSC MULTI USB DRIVE HOT PLUGGING TEST\n\n");
	Serial.printf("NOTE: If you are using a HDD drive with a USB to SATA adapter\n");
	Serial.printf("      it can take a few seconds to connect on power up.\n");
	Serial.printf("      Also, 'USE_EXTERNAL_INIT' must be defined in\n");
	Serial.printf("      MassStorage.h for this sketch to compile with my\n");
	Serial.printf("      version of uSDFS.\n\n");
	Serial.printf("Plug in at least one USB Mass Storage devices (If not already plugged in).\n\n"); 
	
	myusb.begin();
	MSCFS_init();
}

void loop() {
	Serial.printf("\nThis sketch also uses the changeDrive() function to test if\n");
	Serial.printf("the USB drive is connected\n");
	Serial.printf("\n\nSelect:\n");
	Serial.printf("    1) for drive 0 information.\n");
	Serial.printf("    2) for drive 1 information.\n\n");
	Serial.printf("Unplug 1 drive and retest. Then plug it back in again and retest.\n");
	Serial.printf("HINT: Watch USB HUB port numbers and device addresses after replugging ;>)\n\n");
	while(!Serial.available());
	c = Serial.read();
	if(Serial.available()) Serial.read(); // Get rid of CR or LF if there.

	switch(c) {
		case '1':
			if(changeDrive(2) == -1) {
				Serial.printf("Drive %d does not seem to be avaiable or plugged in!\n",2);
			} else {
				showUSBDriveInfo(2);
			}
			break;
		case '2':
			if(changeDrive(3) == -1) {
				Serial.printf("Drive %d does not seem to be avaiable or plugged in!\n",3);
			} else {
				showUSBDriveInfo(3);
			}
			break;
		default:
			break;
	}
}
