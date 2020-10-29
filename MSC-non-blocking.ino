/*
 * This sketch attempts to demonstrate the use of MSC in a non-blocking
 * async way. It is doing direct sector reads and writes so it WILL
 * DESTROY any formatting on the drive when do writes to the disk. Plan
 * on reformatting the drive if writes are used.
 * To use the non-blocking version of MSC with a disk OS like FatFs will
 * require some kind of sync mechanism. I know, I tried without success.
 * FatFs is would have to be heavly modfied to do this. Possibly an RTOS
 * like FreeRTOS or ChRt maybe TeensyThreads? Those are above my skill
 * level at this time.
 * I am using tonton81's Circular Buffer and luni64's attachYieldFunc to
 * do this. More proof of concept I guess.
 * 
*/
#include <Arduino.h>
#include "msc.h"
#include "MassStorage.h"

// For this sketch we need to define USE_EXTENAL_INIT in
// MassStorage.h. Explained there.
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
USBHub hub3(myusb);
USBHub hub4(myusb);

msController msDrive1(myusb);
msController msDrive2(myusb);
msHost mscHost;

uint8_t drvInitOk = 0;
#define SecBufSize (32 * 1024)
uint8_t SecReadBuf[SecBufSize];
uint8_t SecWriteBuf[SecBufSize];
uint32_t startSector = 1000000; // Way up there:)

#define RW_COUNT 1000
#define FILLCHAR 0xaa // Change this to write a different value to sectors.

// Uncomment "#define BLOCKING" to wait for the complete transfer to complete and
// the time it took. Reads and writes will block until transfer is complete.
//#define BLOCKING


// Show USB drive information for the selected USB drive.
int showUSBDriveInfo(msController *drive) {
	if(drive == &msDrive1) {
		Serial.printf(F("msDrive1 is "));
	} else {
		Serial.printf(F("msDrive2 is "));
	}
	if(drive->msDriveInfo.mounted == true) {// check if mounted.
		Serial.printf(F("Mounted\n\n"));
	} else {
		Serial.printf(F("NOT Mounted\n\n"));
	}
	// Now we will print out the information.
	Serial.printf(F("   connected %d\n"),drive->msDriveInfo.connected);
	Serial.printf(F("   initialized %d\n"),drive->msDriveInfo.initialized);
	Serial.printf(F("   USB Vendor ID: %4.4x\n"),drive->msDriveInfo.idVendor);
	Serial.printf(F("  USB Product ID: %4.4x\n"),drive->msDriveInfo.idProduct);
	Serial.printf(F("      HUB Number: %d\n"),drive->msDriveInfo.hubNumber);
	Serial.printf(F("        HUB Port: %d\n"),drive->msDriveInfo.hubPort);
	Serial.printf(F("  Device Address: %d\n"),drive->msDriveInfo.deviceAddress);
	Serial.printf(F("Removable Device: "));
	if(drive->msDriveInfo.inquiry.Removable == 1)
		Serial.printf(F("YES\n"));
	else
		Serial.printf(F("NO\n"));
	Serial.printf(F("        VendorID: %8.8s\n"),drive->msDriveInfo.inquiry.VendorID);
	Serial.printf(F("       ProductID: %16.16s\n"),drive->msDriveInfo.inquiry.ProductID);
	Serial.printf(F("      RevisionID: %4.4s\n"),drive->msDriveInfo.inquiry.RevisionID);
	Serial.printf(F("         Version: %d\n"),drive->msDriveInfo.inquiry.Version);
	Serial.printf(F("    Sector Count: %ld\n"),drive->msDriveInfo.capacity.Blocks);
	Serial.printf(F("     Sector size: %ld\n"),drive->msDriveInfo.capacity.BlockSize);
	Serial.printf(F("   Disk Capacity: %.f Bytes\n\n"),(double_t)drive->msDriveInfo.capacity.Blocks *
										(double_t)drive->msDriveInfo.capacity.BlockSize);
	return 0;
}

// Read some sectors
uint8_t readsectors(msController *drive) {
	uint8_t msResult = MS_CBW_PASS;
	uint32_t cntr = 0;
	uint32_t start = 0, finish = 0;
	uint32_t i;
	uint32_t secTemp = startSector;
	
	mscHost.clearBytesRead(); // Must be called to clear the last transfer total byte count.
	start = micros();
	for(i = 0; i < RW_COUNT; i++) { 
#ifndef BLOCKING
		mscHost.asyncReadSectors(drive, SecReadBuf, startSector, sizeof(SecReadBuf)/512);
		msResult = mscHost.transferStatus(); // Check for failed transfer.
#else
		msResult = mscHost.readSectors(drive, SecReadBuf, startSector, sizeof(SecReadBuf)/512);
#endif
		if(msResult != MS_CBW_PASS) {
			return msResult;
		}
		cntr += SecBufSize;
		startSector += (sizeof(SecReadBuf)/512);
	}
	startSector = secTemp;
	finish = (micros() - start);
#ifndef BLOCKING
	Serial.printf(F("Read %lu bytes queued in %f seconds.\n"),cntr,(1.0*finish)/1000000.0);
	Serial.printf(F("----------------------------------------------------------\n"));
#else
	finish = (micros() - start);
	Serial.printf(F("Read %lu bytes in %f seconds.\n")),cntr,(1.0*finish)/1000000.0);
	Serial.printf(F("----------------------------------------------------------\n"));
#endif
	return msResult;
}

// Write some sectors
uint8_t writesectors(msController *drive) {
	uint8_t msResult = MS_CBW_PASS;
	uint32_t cntr = 0;
	uint32_t start = 0, finish = 0;
	uint32_t i;
	uint32_t secTemp = startSector;

	mscHost.clearBytesWritten(); // Must be called to clear the last total transfer byte count.
	start = micros();
	for(i = 0; i < RW_COUNT; i++) { 
#ifndef BLOCKING
		mscHost.asyncWriteSectors(drive, SecWriteBuf, startSector, sizeof(SecWriteBuf)/512);
		msResult = mscHost.transferStatus(); // Check for failed transfer.
#else
		msResult = mscHost.writeSectors(drive, SecReadBuf, startSector, sizeof(SecReadBuf)/512);
#endif
		if(msResult != MS_CBW_PASS) {
			return msResult;
		}
		cntr += SecBufSize;
		startSector += (sizeof(SecWriteBuf)/512);
	}
	startSector = secTemp;
	finish = (micros() - start);
#ifndef BLOCKING
	Serial.printf(F("Write %lu bytes queued in %f seconds.\n"),cntr,(1.0*finish)/1000000.0);
	Serial.printf("----------------------------------------------------------\n");
#else
	finish = (micros() - start);
	Serial.printf(F("Wrote %lu bytes in %f seconds.\n"),cntr,(1.0*finish)/1000000.0);
	Serial.printf(F("----------------------------------------------------------\n"));
#endif
	return msResult;
}

// Verify sector writes
uint8_t verifySectorWrites(msController *drive) {
	uint8_t msResult = MS_CBW_PASS;
	uint32_t cntr = 0;
	uint32_t i = 0, j = 0;
	uint32_t secTemp = startSector;
	bool errorFlag = false;
	
	for(i = 0; i < RW_COUNT; i++) { 
		fillBuff(0);
		msResult = mscHost.readSectors(drive, SecReadBuf, startSector, sizeof(SecReadBuf)/512);
		if(msResult != MS_CBW_PASS) {
			return msResult;
		}
		cntr += SecBufSize;
		startSector += (sizeof(SecReadBuf)/512);
		for(j = 0; j < SecBufSize; j++) {
			if(SecReadBuf[j] != (uint8_t)FILLCHAR) {
				errorFlag = true;
				break;
			}
		}
		if(errorFlag) {
			break;
		}
	}
	if(errorFlag) {
		mscHost.hexDump(SecReadBuf,SecBufSize);
		Serial.printf(F("Error: SecReadBuf[%lu] = %2.2X. Should be %2.2X\n"),j,SecReadBuf[j], (uint8_t)FILLCHAR);
	} else {
		Serial.printf(F("Sector(s) Write Verified\n"));
	}
	startSector = secTemp;
	return msResult;
}

// Fill sector buffer with a uint8_t char.
void fillBuff(uint8_t fillChar) {
	for(uint32_t i = 0; i < SecBufSize; i++) {
		SecReadBuf[i] = fillChar;
		SecWriteBuf[i] = fillChar;
	}
}
	
void setup() {
	while(!Serial);
	Serial.printf(F("Teensy USBHost MSC non-blocking demo\n\n"));

	// Init USBHost
	myusb.begin();
	// Init MSC Host
	mscHost.mscHostInit();

	Serial.printf(F("Checking for msDrive1\n"));
	if((drvInitOk = msDrive1.mscInit()) != 0) {
		Serial.printf(F("msDrive1 init FAILED: %d\n\n"),drvInitOk);
	} else {
		Serial.printf(F("msDrive1 init: OK\n\n"));
	}
	Serial.printf(F("Checking for msDrive2\n"));
	if((drvInitOk = msDrive2.mscInit()) != 0) {
		Serial.printf(F("msDrive2 init FAILED: %d\n\n"),drvInitOk);
	} else {
		Serial.printf(F("msDrive2 init: OK\n\n"));
	}
}

void loop() {
	char op = 0;
	uint8_t msResult = 0;

	Serial.printf(F("\nSelect:\n"));
	Serial.printf(F("   1) Test Sector Read Drive 1\n"));
	Serial.printf(F("   2) Test Sector Read Drive 2\n"));
	Serial.printf(F("   3) Test Sector Write Drive 1 ******** CAUTION!!! DESTROY'S FORMATTING OF DRIVE!! *********\n"));
	Serial.printf(F("   4) Test Sector Write Drive 2 ******** CAUTION!!! DESTROY'S FORMATTING OF DRIVE!! *********\n"));
	Serial.printf(F("   5) Verify Sector Write Drive 1\n"));
	Serial.printf(F("   6) Verify Sector Write Drive 2\n"));
	Serial.printf(F("   7) Zero Sector Buffer\n"));
	Serial.printf(F("   8) Hex Dump Sector Buffer\n"));

	while(!Serial.available()) {
		yield();
		op = Serial.read();
	}
	if(Serial.available()) Serial.read(); // Get rid of CR or LF if there.

	switch(op) {
		case '1':
			Serial.printf(F("\nReading From USB Drive 1:"));
#ifndef BLOCKING
			Serial.printf(F(" (non-blocking)\n"));
#else
			Serial.printf(F(" (blocking)\n"));

#endif
			if((msResult = readsectors(&msDrive1)))
				Serial.printf(F("msDrive1.msReadBlocks Failed. Code : %d\n"), msResult);
//				showUSBDriveInfo(&msDrive1);
			break;
		case '2':
			Serial.printf(F("\nReading From USB Drive 2:"));
#ifndef BLOCKING
			Serial.printf(F(" (non-blocking)\n"));
#else
			Serial.printf(F(" (blocking)\n"));

#endif
			if((msResult = readsectors(&msDrive2)))
				Serial.printf(F("msDrive1.msReadBlocks Failed. Code : %d\n"), msResult);
//				showUSBDriveInfo(&msDrive2);
			break;
		case '3':
			Serial.printf(F("\nWrite To USB Drive 1:"));
#ifndef BLOCKING
			Serial.printf(F(" (non-blocking)\n"));
#else
			Serial.printf(F(" (blocking)\n"));

#endif
			fillBuff((uint8_t)FILLCHAR);
			if((msResult = writesectors(&msDrive1)))
				Serial.printf(F("msDrive1.msWriteBlocks Failed. Code : %d\n"), msResult);
//				showUSBDriveInfo(&msDrive1);
			break;
		case '4':
			Serial.printf(F("\nWrite To USB Drive 2:"));
#ifndef BLOCKING
			Serial.printf(F(" (non-blocking)\n"));
#else
			Serial.printf(F(" (blocking)\n"));

#endif
			fillBuff((uint8_t)FILLCHAR);
			if((msResult = writesectors(&msDrive2)))
				Serial.printf(F("msDrive2.msWriteBlocks Failed. Code : %d\n"), msResult);
//				showUSBDriveInfo(&msDrive2);
			break;
		case '5':
			Serial.printf(F("\nVerifing Write To USB Drive 1 (blocking)\n"));
			if((msResult = verifySectorWrites(&msDrive1)))
				Serial.printf(F("msDrive1.verifySectorWrites Failed. Code : %d\n"), msResult);
			break;
		case '6':
			Serial.printf(F("\nVerifing Write To USB Drive 2 (blocking)\n"));
			if((msResult = verifySectorWrites(&msDrive2)))
				Serial.printf(F("msDrive2.verifySectorWrites Failed. Code : %d\n"), msResult);
			break;
		case '7':
			fillBuff(0);
			break;
		case '8':
			mscHost.hexDump(SecReadBuf,SecBufSize);
			break;
		default:
			break;
	}
}
