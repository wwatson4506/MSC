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
uint32_t fre_clust, fre_sect, tot_sect;

TCHAR label[64];	 /* Pointer to a buffer to return the volume label */
DWORD vsn = 0;	 /* Pointer to a variable to return the volume serial number */

bool drv0 = false;
bool drv1 = false;

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
USBHub hub3(myusb);
USBHub hub4(myusb);

msDriveInfo_t *driveInfo;

const char usbDriveName[2][4] = {"2:/","3:/"};

void showDriveInfo(uint8_t drive) {

	f_getlabel(usbDriveName[drive], label, &vsn);
	Serial.printf("USB DRIVE %s INFO\n\n",usbDriveName[drive]);
	Serial.printf("Volume Label: %s\n", label);
	Serial.printf("Volume Serial Number: %lu\n\n",vsn);
	driveInfo = getDriveInfo(drive);
	Serial.printf("  USB Vendor ID: %4.4x\n",driveInfo->idVendor);
	Serial.printf(" USB Product ID: %4.4x\n",driveInfo->idProduct);
	Serial.printf("     HUB Number: %d\n",driveInfo->hubNumber);
	Serial.printf("       HUB Port: %d\n",driveInfo->hubPort);
	Serial.printf(" Device Address: %d\n",driveInfo->deviceAddress);
	Serial.printf("Removable Device: ");
	if(driveInfo->inquiry.Removable == 1)
		Serial.printf("YES\n");
	else
		Serial.printf("NO\n");
	Serial.printf("        VendorID: %8.8s\n",driveInfo->inquiry.VendorID);
	Serial.printf("       ProductID: %16.16s\n",driveInfo->inquiry.ProductID);
	Serial.printf("      RevisionID: %4.4s\n",driveInfo->inquiry.RevisionID);
	Serial.printf("         Version: %d\n",driveInfo->inquiry.Version);
	Serial.printf("    Sector Count: %ld\n",driveInfo->capacity.Blocks);
	Serial.printf("     Sector size: %ld\n",driveInfo->capacity.BlockSize);
	Serial.printf("   Disk Capacity: %.f Bytes\n",(double_t)driveInfo->capacity.Blocks *
										(double_t)driveInfo->capacity.BlockSize);
	Serial.printf("      Free Space: %.f Kilo Bytes\n\n",(double_t)getDfree(drive)); 
}

uint64_t getDfree(uint8_t drive) {
	switch(drive)
	{
		case 1:
			f_getfree("3:/", &fre_clust, &fs2);
			tot_sect = (fs2->n_fatent - 2) * fs2->csize;
			fre_sect = fre_clust * fs2->csize;
			break;
		case 0:
			f_getfree("2:/", &fre_clust, &fs1);
			tot_sect = (fs1->n_fatent - 2) * fs1->csize;
			fre_sect = fre_clust * fs1->csize;
			break;
		default:
			break;
	}
	return (uint64_t)(fre_sect / 2);
//	return (uint64_t)(fre_sect);
}

uint8_t c = 0;

void setup() {
	while(!Serial);

	Serial.printf("MSC MULTI USB DRIVE HOT PLUGGING TEST\n\n");
	Serial.printf("Waiting for drives to connect.\n\n");
	Serial.printf("NOTE: If you are using a HDD drive with a USB to SATA adapter\n");
	Serial.printf("      it can take a few seconds to connect on power up.\n");
	Serial.printf("      Also, 'USE_EXTENAL_INIT' must be defined in\n");
	Serial.printf("      MassStorage.h for this sketch to compile.\n");
	Serial.printf("Plug in at least two devices (If not already plugged in).\n\n"); 
	
	myusb.begin();
	mscInit();

	for(uint8_t i = 0; i < MAXDRIVES; i++) {
		if(!checkDeviceConnected(i)) {
			Serial.printf("Drive %d does not seem to be avaiable or plugged in!\n",i);
			while(!checkDeviceConnected(i));
		}
		Serial.printf("Drive %d has connected\n",i);
	}
	Serial.printf("Mounting Drive %s\n",usbDriveName[0]);
    m_res = f_mount(&FatFs1, usbDriveName[0], 1);			// Mount the File System (USB 2:/)
    if(m_res != 0) {
    	Serial.printf("Filesystem Mount Failed with code: %d, drive: %s\n",m_res, usbDriveName[0]);
    }
	drv0 = true; // Used to know when to remount drive.
	Serial.printf("Mounting Drive %s\n",usbDriveName[1]);
    m_res = f_mount(&FatFs2, usbDriveName[1], 1);			// Mount the File System (USB 3:/)
    if(m_res != 0) {
	    Serial.printf("Filesystem Mount Failed with code: %d, drive: %s\n",m_res, usbDriveName[1]);
    }
	Serial.printf("Drives Mounted\n");
	Serial.printf("Setting Default Drive To: %s\n",usbDriveName[0]);
	m_res = f_chdrive(usbDriveName[0]); // Change to default USB Drive 0
	drv1 = true; // Used to know when to remount drive.
}

void loop() {
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
			if(checkDeviceConnected(0)) {
				if(!drv0) { // Drive 2:/ was previuosly unplugged.
					// So Re-Mount the File System to reload label, serial number, total
					// drive capacity in bytes and free space left in kilo bytes.  
					m_res = f_mount(&FatFs1, usbDriveName[0], 1); // Re-Mount USB Drive.
					drv0 = true;
				}
				showDriveInfo(0);
			} else {
				drv0 = false; // Used to know when to remount drive after re-plug.
				Serial.printf("Drive %d does not seem to be avaiable or plugged in!\n",0);
			}
			break;
		case '2':
			if(checkDeviceConnected(1)) {
				if(!drv1) { // Same as USB drive 2:/.
					m_res = f_mount(&FatFs2, usbDriveName[1], 1); // Re-Mount the File System (USB 3:/)
					drv1 = true;
				}
				showDriveInfo(1);
			} else {
				drv1 = false; // Used to know when to remount drive after re-plug.
				Serial.printf("Drive %d does not seem to be avaiable or plugged in!\n",1);
			}
			break;
		default:
			break;
	}
}
