//Copyright 2020 by Warren Watson
// Version 02-Feb-20
//
// use following lines for early definitions of multiple partition configuration in uSDFS.h
#include "Arduino.h"
#include "uSDFS.h"
#include "MassStorage.h"
#include "msc.h"

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

FRESULT m_res;
FILINFO fno;

FATFS	FatFs0, *fs0;   /* Work area (file system object) for logical drive 1 */
FATFS	FatFs1, *fs1;   /* Work area (file system object) for logical drive 2 */
FATFS	FatFs2, *fs2;   /* Work area (file system object) for logical drive 3 */

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
USBHub hub3(myusb);
USBHub hub4(myusb);

const char usbDriveName[3][4] = {"2:/","3:/","1:/"};

const char *D1 = "1:/A_00001.dat";  // SDHC
const char *D2 = "2:/A_00001.dat";  // USB
const char *D3 = "3:/A_00001.dat";  // USB

TCHAR labelD1[64];	 /* Pointer to a buffer to return the volume label (SDHC)*/
TCHAR labelD2[64];	 /* Pointer to a buffer to return the volume label (USB)*/
TCHAR labelD3[64];	 /* Pointer to a buffer to return the volume label (USB1)*/

DWORD vsnD1 = 0;	 /* Pointer to a variable to return the volume serial number (SDHC)*/
DWORD vsnD2 = 0;	 /* Pointer to a variable to return the volume serial number (USB)*/
DWORD vsnD3 = 0;	 /* Pointer to a variable to return the volume serial number (USB1)*/

UINT bufferSize = 32*1024; 

FRESULT fileCopy(const char *src, const char *dest) {
    FIL fsrc, fdst;      /* File objects */
    FRESULT fr;          /* FatFs function common result code */
    UINT br, bw;         /* File read/write count */
	uint32_t flSize = 0;
	uint32_t cntr = 0;
	uint32_t start = 0, finish = 0;
	
	uint32_t buffer[bufferSize];   /* File copy buffer */

//	uint8_t *buffer;   /* File copy buffer */
//	buffer = (uint8_t *) malloc(bufferSize);
	start = micros();
    /* Open source file */
    fr = f_open(&fsrc, src, FA_READ);
    if (fr) return fr;
	flSize = f_size(&fsrc); // Size of source file.
    /* Create destination file on the drive 0 */
    fr = f_open(&fdst, dest, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr)	return fr;

    /* Copy source to destination */
    for (;;) {
		cntr++;
		if(!(cntr%10))Serial.printf("*");
		if(!(cntr%640)) Serial.printf("\n");
        fr = f_read(&fsrc, buffer, bufferSize, &br);  /* Read a chunk of source file */
        if (fr || br == 0) break; /* error or eof */
        fr = f_write(&fdst, buffer, br, &bw);            /* Write it to the destination file */
        if (fr || bw < br) break; /* error or disk full */
    }
    /* Close open files */
    f_close(&fsrc);
    f_close(&fdst);
//	free(buffer);
	finish = micros() - start;
    float MegaBytes = (flSize*1.0f)/(1.0f*finish);
	Serial.printf("Copied %u bytes in %lu us. Speed: %f MB/s\n",flSize,finish,MegaBytes);
	return fr;
}

uint8_t c = 0;

void setup() {
	while(!Serial);

	Serial.printf("MSC MULTI USB DRIVE FILE COPY TEST\n\n");
	Serial.printf("Waiting for drives to connect.\n\n");
	Serial.printf("NOTE: If you are using a HDD drive with a USB to SATA adapter\n");
	Serial.printf("      it can take a few seconds to connect on power up.\n");
	Serial.printf("      Also, 'USE_EXTERNAL_INIT' must be defined in\n");
	Serial.printf("      MassStorage.h for this sketch to compile.\n\n");
	Serial.printf("Plug in at least two devices (If not already plugged in).\n\n"); 
	
	myusb.begin();
	mscInit();

	for(uint8_t i = 0; i < MAXDRIVES; i++) {
		if(!checkDeviceConnected(i)) {
			Serial.printf("Drive %d does not seem to be avaiable or plugged in! (Maybe it's an HDD Drive)\n",i);
			while(!checkDeviceConnected(i));
		}
		Serial.printf("Drive %d has connected\n",i);
	}
    m_res = f_mount(&FatFs0, usbDriveName[2], 1);			// Mount the File System (SDHC 1:/)
    if(m_res != 0) {
    	Serial.printf("Filesystem Mount Failed with code: %d, drive: %s\n",m_res, usbDriveName[2]);
    }
	f_getlabel(usbDriveName[2], labelD1, &vsnD1);
	Serial.printf("\nMounted Drive %s (SDHC) %s\n",usbDriveName[2],labelD1); // Actually SDHC card.

    m_res = f_mount(&FatFs1, usbDriveName[0], 1);			// Mount the File System (USB 2:/)
    if(m_res != 0) {
    	Serial.printf("Filesystem Mount Failed with code: %d, drive: %s\n",m_res, usbDriveName[0]);
    }
	f_getlabel(usbDriveName[0], labelD2, &vsnD2);
	Serial.printf("Mounted Drive %s (USB)  %s\n",usbDriveName[0],labelD2);

    m_res = f_mount(&FatFs2, usbDriveName[1], 1);			// Mount the File System (USB 3:/)
    if(m_res != 0) {
	    Serial.printf("Filesystem Mount Failed with code: %d, drive: %s\n",m_res, usbDriveName[1]);
    }
	f_getlabel(usbDriveName[1], labelD3, &vsnD3);
	Serial.printf("Mounted Drive %s (USB1) %s\n",usbDriveName[1], labelD3);

	Serial.printf("All Drives Successfully Mounted\n\n");
	Serial.printf("Setting Default Drive To: %s (%s)\n",usbDriveName[0], labelD2);
	m_res = f_chdrive(usbDriveName[0]); // Change to default USB Drive 0
}

void loop() {
	Serial.printf("\nDrive %s (SDHC) %s\n",usbDriveName[2],labelD1); // Actually SDHC card.
	Serial.printf("Drive %s (USB)  %s\n",usbDriveName[0],labelD2);
	Serial.printf("Drive %s (USB1) %s\n",usbDriveName[1], labelD3);
	Serial.printf("\nSelect:\n");
	Serial.printf("    1) to copy 'A_00001.dat' from drive 2:/ to drive 3:/.\n");
	Serial.printf("    2) to copy 'A_00001.dat' from drive 3:/ to drive 2:/.\n");
	Serial.printf("    3) to copy 'A_00001.dat' from drive 2:/ to drive 1:/.\n");    
	Serial.printf("    4) to copy 'A_00001.dat' from drive 1:/ to drive 2:/.\n");    
	Serial.printf("    5) to copy 'A_00001.dat' from drive 3:/ to drive 1:/.\n");    
	Serial.printf("    6) to copy 'A_00001.dat' from drive 1:/ to drive 3:/.\n");    

	while(!Serial.available());
	c = Serial.read();
	if(Serial.available()) Serial.read(); // Get rid of CR or LF if there.

	switch(c) {
		case '1':
			Serial.printf("\n1) Copying from %s to %s\n", labelD2,labelD3);
			m_res = fileCopy(D2,D3);
			if(m_res != 0) {
				Serial.printf("File Copy Failed with code: %d\n",m_res);
			}
			break;
		case '2':
			Serial.printf("\n2) Copying from %s to %s\n", labelD3,labelD2);
			m_res = fileCopy(D3,D2);
			if(m_res != 0) {
				Serial.printf("File Copy Failed with code: %d\n",m_res);
			}
			break;
		case '3':
			Serial.printf("\n3) Copying from %s to %s\n", labelD2,labelD1);
			m_res = fileCopy(D2,D1);
			if(m_res != 0) {
				Serial.printf("File Copy Failed with code: %d\n",m_res);
			}
			break;
		case '4':
			Serial.printf("\n4) Copying from %s to %s\n", labelD1,labelD2);
			m_res = fileCopy(D1,D2);
			if(m_res != 0) {
				Serial.printf("File Copy Failed with code: %d\n",m_res);
			}
			break;
		case '5':
			Serial.printf("\n5) Copying from %s to %s\n", labelD3,labelD1);
			m_res = fileCopy(D3,D1);
			if(m_res != 0) {
				Serial.printf("File Copy Failed with code: %d\n",m_res);
			}
			break;
		case '6':
			Serial.printf("\n6) Copying from %s to %s\n", labelD1,labelD3);
			m_res = fileCopy(D1,D3);
			if(m_res != 0) {
				Serial.printf("File Copy Failed with code: %d\n",m_res);
			}
			break;
		default:
			break;
	}
}
