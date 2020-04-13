//Copyright 2019 by Walter Zimmer
// Version 08-jun-19
//
// use following lines for early definitions of multiple partition configuration in uSDFS.h
#include "Arduino.h"
#include "mscfs.h"
#include "MassStorage.h"
#include "msc.h"

//#include "osal.h"
#define MY_VOL_TO_PART
#if FF_MULTI_PARTITION		/* Multiple partition configuration */ 
	PARTITION VolToPart[] = {{DEV_SPI, 0}, //{ physical drive number, Partition: 0:Auto detect, 1-4:Forced partition)} 
							 {DEV_SDHC,0}, 
							 {DEV_USB, 0}, 
							 {DEV_USB, 1}, 
							 {DEV_USB, 2}
							 }; /* Volume - Partition resolution table */
#endif

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
USBHub hub3(myusb);
USBHub hub4(myusb);
msDriveInfo_t *driveInfo;

extern TCHAR driveLabel[4][64];	// Pointer to a buffer to return the volume driveLabel
extern DWORD serialNum[4];	 	// Pointer to a variable to return the volume serial number

FRESULT fr;
FIL filenum;
UINT wr, rd;
char buff[256];

// A simple function to get a string from the console.
char *getString(char *strbuf) {
	int i=0;
	*strbuf = 0;
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
#define BUFFSIZE (8*1024) // size of buffer to be written
uint32_t buffer[BUFFSIZE]; // 32768000 byte buffer.

void setup() {
	while(!Serial);

	Serial.printf("MSC READ/WRITE EXAMPLE\n\n");
	Serial.printf("NOTE: If you are using a HDD drive with a USB to SATA adapter\n");
	Serial.printf("      it can take a few seconds to connect on power up.\n");
	Serial.printf("      Also, 'USE_EXTERNAL_INIT' must be defined in\n");
	Serial.printf("      MassStorage.h for this sketch to compile with my\n");
	Serial.printf("      version of uSDFS.\n\n");
	Serial.printf("Plug in at least one USB Mass Storage devices (If not already plugged in).\n\n"); 

	myusb.begin();
	MSCFS_init();

	Serial.printf("Getting labels and serial numbers for all connected drives\n");
	getDriveLabels(255);
}

void loop() {
	char op = 0;
	uint32_t cntr = 0;
	uint32_t start = 0, finish = 0;
	float MegaBytes = 0;
	uint32_t bytesRW = 0;

	Serial.printf("\nDrive %s (SDHC) %s\n",getDriveName(1),driveLabel[1]); // Actually SDHC card.
	Serial.printf("Drive %s (USB0) %s\n",getDriveName(2),driveLabel[2]);
	Serial.printf("Drive %s (USB1) %s\n",getDriveName(3), driveLabel[3]);
	Serial.printf("Default drive is: %s (%s)\n", getDriveName(getCurrentDrive()),driveLabel[getCurrentDrive()]);
	// Fill the Read/Write buffer with data
	for(uint32_t i = 0; i < BUFFSIZE; i++) {
		buffer[i] = '0'+(i%10);
	}
	Serial.printf("\nSelect:\n");
	Serial.printf("   1) Write to then read from file: 'test.dat'\n");
	Serial.printf("   2) List Directory\n");
	while (!Serial.available());
	op = Serial.read();
	if(Serial.available()) Serial.read(); // Get rid of CR and/or LF if there.
	switch(op) {
		case '1':
			Serial.printf("\nWhich drive? 1:/, 2:/ or 3:/\n\n");
			getString(buff);
			buff[3] = 0;
			if(!driveAvailable(getDriveIndex(buff))) {
				Serial.printf("\nUSB drive %s is not available or not mounted\n",buff);
				break;
			}
			// Write to the file
			if((fr = f_open(&filenum, strcat(buff,"test.dat"), (FA_WRITE | FA_CREATE_ALWAYS))) != FR_OK) {
				Serial.printf("Open file for write failed with code: %d\n",fr);
				break;
			}
			Serial.printf("----------------------------------------------------------\n");
			Serial.printf("Writing to file %s\n",buff);
			start = micros();
			for(uint32_t i= 1; i <= 1000; i++) {
				cntr++;
				if(!(cntr%10)) Serial.printf("*");
				if(!(cntr%640)) Serial.printf("\n");
				if((fr = f_write(&filenum, buffer, BUFFSIZE*sizeof(buffer[0]), &wr)) != FR_OK) {
					Serial.printf("File write failed with code: %d\n",fr);
					break;
				}
				bytesRW += (uint32_t)wr;
			}
			f_close(&filenum);
			finish = (micros() - start);
			MegaBytes = (bytesRW*1.0f)/(1.0f*finish);
			Serial.printf("\nWrote %u bytes in %f seconds. Speed: %f MB/s\n",bytesRW,(1.0*finish)/1000000.0,MegaBytes);
			Serial.printf("----------------------------------------------------------\n\n");
			// Read back the file
			cntr = 0;
			bytesRW = 0;
			if((fr = f_open(&filenum, buff, FA_READ)) != FR_OK) {
				Serial.printf("Open file for read failed with code: %d\n",fr);
				break;
			}
			Serial.printf("----------------------------------------------------------\n");
			Serial.printf("Reading from file %s\n",buff);
			start = micros();
			for(uint32_t i= 1; i <= 1000; i++) {
				cntr++;
				if(!(cntr%10)) Serial.printf("*");
				if(!(cntr%640)) Serial.printf("\n");
				if((fr = f_read(&filenum, buffer, BUFFSIZE*sizeof(buffer[0]), &rd)) != FR_OK) {
					Serial.printf("File read failed with code: %d\n",fr);
					break;
				}
				bytesRW += (uint32_t)wr;
			}
			f_close(&filenum);
			finish = (micros() - start);
			MegaBytes = (bytesRW*1.0f)/(1.0f*finish);
			Serial.printf("\nRead %u bytes in %f seconds. Speed: %f MB/s\n",bytesRW,(1.0*finish)/1000000.0,MegaBytes);
			Serial.printf("----------------------------------------------------------\n");
			break;
		case '2':
			Serial.printf("\nEnter drive and filespec:\n");
			getString(buff);
			Serial.printf("----------------------------------------------------------\n");
			Serial.printf("Directory listing for %s\n", buff);
			Serial.printf("----------------------------------------------------------\n");
			lsdir(buff);
			break;
		default:
			break;
	}
}
