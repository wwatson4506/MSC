/*
 * MSC Teensy36 USB Host Mass Storage library
 * Copyright (c) 2017-2020 Warren Watson.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// MassStorageHost.cpp
// This file is the interface between MSC and uSDFS 
#include <Arduino.h>
#include "msc.h"
#include "MassStorage.h"

//#define DEBUG_MSC
//#define DEBUG_MSC_VERBOSE

#ifndef DEBUG_MSC
#undef DEBUG_MSC_VERBOSE
void inline DBGPrintf(...) {};
#else
#define DBGPrintf Serial.printf
#endif

#ifndef DEBUG_MSC_VERBOSE
void inline VDBGPrintf(...) {};
void inline DBGHexDump(const void *ptr, uint32_t len) {};
#else
#define VDBGPrintf Serial.printf
#define DBGHexDump hexDump
#endif

//#define print   USBHost::print_
//#define println USBHost::println_

extern USBHost myusb;
msController msDrive1(myusb);
msController msDrive2(myusb);
Device_p mscDrives[] = {&msDrive1, &msDrive2};
//msController msDrive3(myusb);
//msController msDrive4(myusb);
//Device_p mscDrives[] = {&msDrive1, &msDrive2, &msDrive3};
//Device_p mscDrives[] = {&msDrive1, &msDrive2, &msDrive3, &msDrive4};

msRequestSenseResponse_t msSense;
msCommandStatusWrapper_t CSW;

static msDriveInfo_t msDriveInfo[MAXDRIVES];
static MSC_transfer_t myMSCtransfers[MAXTRANSFERS];
static uint8_t xferIndex = 0; 

static uint8_t currentDrive = MSC1; // Default Drive 0
static 	uint8_t numDevices = 0;

static uint8_t maxLUN = 16;
static uint8_t currentLUN = 0;
static uint32_t CBWTag = 0;

//Device_p mscDevice = mscDrives[currentDrive];

// A small hex dump function
void hexDump(const void *ptr, uint32_t len)
{
  uint32_t  i = 0, j = 0;
  uint8_t   c = 0;
  const uint8_t *p = (const uint8_t *)ptr;

  Serial.printf("BYTE      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n\r");
  Serial.printf("---------------------------------------------------------\n\r");
  for(i = 0; i <= (len-1); i+=16) {
   Serial.printf("%4.4x      ",i);
   for(j = 0; j < 16; j++) {
      c = p[i+j];
      Serial.printf("%2.2x ",c);
    }
    Serial.printf("  ");
    for(j = 0; j < 16; j++) {
      c = p[i+j];
      if(c > 31 && c < 127)
        Serial.printf("%c",c);
      else
        Serial.printf(".");
    }
    Serial.printf("\n");
  }

}

// Callback function for completed xfer. (Not used at this time)
static void transferCompleted(bool complete) {
//	myMSCtransfers[xferIndex].completed = complete;
}

// Do a complete 2 or 3 stage transfer.
uint8_t msTransfer(MSC_transfer_t *transfer, void *buffer) {
	msCommandStatusWrapper_t CSW; // Create a local CSW Struct
	transfer->completed = false;
	mscDrives[currentDrive]->msDoCommand(transfer->CBW); // Command stage.
	while(!mscDrives[currentDrive]->outTransferComplete());
	//If buffer == NULL then no data stage is needed.
	if(buffer != NULL) {  // Data stage needed.
		transfer->buffer = buffer;
		if(transfer->CBW->Flags == CMDDIRDATAIN) {
			mscDrives[currentDrive]->msRxData(transfer); // Data In Stage.
			while(!mscDrives[currentDrive]->inTransferComplete());
		} else {
			mscDrives[currentDrive]->msTxData(transfer); // Data Out Stage.
			while(!mscDrives[currentDrive]->outTransferComplete());
		}
	}
	transfer->CSW = &CSW;
	mscDrives[currentDrive]->msGetCSW(transfer->CSW); // Status Stage.
	while(!mscDrives[currentDrive]->inTransferComplete()); // WAIT FOR IT !!! :(
	transfer->completed = true; // All stages of this transfer have completed.
	//Check for special cases. 
	//If test for unit ready command is given then
	//  return the CSW status byte.
	//Bit 0 == 1 == not ready else
	//Bit 0 == 0 == ready.
	//And the Start/Stop Unit command as well.
	if((transfer->CBW->CommandData[0] == CMDTESTUNITREADY) ||
	   (transfer->CBW->CommandData[0] == CMDSTARTSTOPUNIT))
		return transfer->CSW->Status;
	else // Process possible SCSI errors.
		return msProcessError(transfer->CSW->Status);
}

// Set current LUN
void msCurrentLun(uint8_t lun) {currentLUN = lun;}

// Initialize a Mass Storage Device
uint8_t initDrive(uint8_t device) {
	uint8_t msResult = 0;
	// Callback function for completed In/Out trasfers.
	mscDrives[device]->attachCompleted(transferCompleted);
	// Wait for device to become available.
	while(!mscDrives[device]->available());
	
	mscDrives[device]->msReset(); // Reset device
	delay(1000);
	maxLUN = mscDrives[device]->msGetMaxLun();  // Get MAXLUN for device (Usualy returns 0).
	delay(150);
	//-------------------------------------------------------
// Start/Stop unit on hold for now. Not all devices respond to it
// the same way. Is it really needed for most devices? Not understanding
// it's use. Am able to to plug in a CDROM and load or eject the media.
// But that's all that works consistently with drives:(
//	msResult = msStartStopUnit(1);	// 0 = transition to stopped power contition
									// 1 = transition to active power contition
									// 2 = load media, tested on SATA CDROM
									// 3 = eject media, Tested on SATA CDROM
	msDriveInfo[device].connected = true;  // Device connected.
	msDriveInfo[device].initialized = true;
	msDriveInfo[device].hubNumber = mscDrives[device]->getHubNumber();  // Which HUB.
	msDriveInfo[device].hubPort = mscDrives[device]->getHubPort();  // Which HUB port.
	msDriveInfo[device].deviceAddress = mscDrives[device]->getDeviceAddress();  // Which HUB.
	msDriveInfo[device].idVendor = mscDrives[device]->getIDVendor();  // USB Vendor ID.
	msDriveInfo[device].idProduct = mscDrives[device]->getIDProduct();  // USB Product ID.

	msResult = getDriveInquiry(device);
	msResult = getDriveCapacity(device);

	// if device is not like hard disk, probably won't work! example CDROM... 
	if (msDriveInfo[device].inquiry.DeviceType != 0)
		return MS_CSW_TAG_ERROR;
	//-------------------------------------------------------
	msCurrentLun(currentLUN);
	return msResult;
}

// Deinit device:
// Clear all drive info to 0 for a MAss Storeage Device
void deInitDrive(uint8_t device) {
	memset(&msDriveInfo[device], 0, sizeof(msDriveInfo_t));
}

// Set current USB drive index. Used primarly in uSDFS.
uint8_t setDrive(uint8_t drive) {
	if(checkDeviceConnected(drive)) {
		currentDrive = drive;
		return currentDrive;
	}
	return currentDrive;
}

// Get current USB drive index. Used primarly in uSDFS.
uint8_t getDrive(void) {
	return currentDrive;
}

// Return a pointer to drive info struct
msDriveInfo_t *getDriveInfo(uint8_t device) {
	return &msDriveInfo[device];
}

// Find and initialize all available Mass Storage Devices.
// Set the timout waiting for devices to be available.
// #define MSC_CONNECT_TIMEOUT 5000 found in MassSorage.h.
// For each device found, initialize it and wait for it to be usable.
uint8_t mscInit(void) {
	uint32_t start = millis();
	numDevices = 0;
	while(((millis() - start) < MSC_CONNECT_TIMEOUT) && (numDevices < MAXDRIVES)) {
		if(mscDrives[numDevices]->available() && mscDrives[numDevices]->initialized()) {
			numDevices++;
		}
	}	
	// Limit number of devices to max allowed.
	if(numDevices > MAXDRIVES) numDevices = MAXDRIVES;
	// One or more devices found. Initialize and wait for it to be ready. 
	for(uint8_t i = 0; i < numDevices; i++) {
		initDrive(i); // Initialize device.
		WaitMediaReady(i); // wait for it to be usable.
	}
	return numDevices; // Global static device count. Set it.
}

// Return number of devices (drives) available
uint8_t getDriveCount(void) {
	return numDevices;
}

// This is called from the MSC driver disconnect interrupt.
// Deinit device and reduce availble device count.
void deviceDisconnected(uint32_t device) {
	for(int i = 0; i < MAXDRIVES; i++) {
		if((uint32_t)mscDrives[i] == (uint32_t)device) {
			deInitDrive(i); // Disconnected, deinit device.
			numDevices--; // Reduce number of available devices.
		}

	}
}

// Check if device is online or was disconnected.
// This one was tricky. not sure if it is right yet.
bool checkDeviceConnected(uint8_t device) {
	if(!deviceAvailable(device)) { // Check if device is still available.
		return false; // No, return not connected.
	}
	if(msDriveInfo[device].initialized) { // Device initialized?
		return true; // Yes, return initialized.
	}
	initDrive(device); // No, Initialize device.
	WaitMediaReady(device); // Wait for it to be usable.
	numDevices++; // Add to available devices.
	return true;
}	

// Check if device is connected. (Driver level)
bool deviceAvailable(uint8_t device) {
	if(mscDrives[device]->available()) { 
		return true;
	}
	return false;
}

// Check if drive has been initialized. (Driver level)
bool deviceInitialized(uint8_t device) {
	return mscDrives[device]->initialized();
}

// wait for device to become usable.
uint8_t WaitMediaReady(uint8_t drv) {
	uint8_t msResult;
	uint32_t start = millis();
	do {
		if((millis() - start) >= 1000) {
			return MSUNITNOTREADY;  // Not Ready Error.
		}
		msResult = msTestReady(drv);
	 } while(msResult == 1);
	return msResult;
}

// Read Sectors.
uint8_t readSectors(void *sectorBuffer,uint32_t BlockAddress, uint16_t Blocks) {
	if(CBWTag == 0xFFFFFFFF) CBWTag = 1;
	uint8_t BlockHi = (Blocks >> 8) & 0xFF;
	uint8_t BlockLo = Blocks & 0xFF;
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
	
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = (uint32_t)(Blocks * (uint16_t)512),
		.Flags              = CMDDIRDATAIN,
		.LUN                = currentLUN,
		.CommandLength      = 10,
		.CommandData        = {CMDRD10, 0x00,
							  (uint8_t)(BlockAddress >> 24),
							  (uint8_t)(BlockAddress >> 16),
							  (uint8_t)(BlockAddress >> 8),
							  (uint8_t)(BlockAddress & 0xFF),
							   0x00, BlockHi, BlockLo, 0x00}
	};
	myMSCtransfers[xferIndex].CBW = &CommandBlockWrapper;
	return msTransfer(&myMSCtransfers[xferIndex], sectorBuffer);
}

// Write Sectors.
uint8_t writeSectors(void *sectorBuffer,uint32_t BlockAddress, uint16_t Blocks) {
	if(CBWTag == 0xFFFFFFFF) CBWTag = 1;
	uint8_t BlockHi = (Blocks >> 8) & 0xFF;
	uint8_t BlockLo = Blocks & 0xFF;
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = (uint32_t)(Blocks * (uint16_t)512),
		.Flags              = CMDDIRDATAOUT,
		.LUN                = currentLUN,
		.CommandLength      = 10,
		.CommandData        = {CMDWR10, 0x00,
		                      (uint8_t)(BlockAddress >> 24),
							  (uint8_t)(BlockAddress >> 16),
							  (uint8_t)(BlockAddress >> 8),
							  (uint8_t)(BlockAddress & 0xFF),
							  0x00, BlockHi, BlockLo, 0x00}
	};
 	myMSCtransfers[xferIndex].CBW = &CommandBlockWrapper;
	return msTransfer(&myMSCtransfers[xferIndex], sectorBuffer);
}

// Get drive sense response.
uint8_t mscDriveSense(msRequestSenseResponse_t *mscSense) {
	if(CBWTag == 0xFFFFFFFF)
		CBWTag = 1;
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = sizeof(msRequestSenseResponse_t),
		.Flags              = CMDDIRDATAIN,
		.LUN                = currentLUN,
		.CommandLength      = 6,
		.CommandData        = {CMDREQUESTSENSE, 0x00, 0x00, 0x00, sizeof(msRequestSenseResponse_t), 0x00}
	};
 	myMSCtransfers[xferIndex].CBW = &CommandBlockWrapper;
	return msTransfer(&myMSCtransfers[xferIndex], mscSense);
}

// Retrieve device capacity data.
uint8_t getDriveCapacity(uint8_t drv) {
	uint8_t msResult, tempDrv = currentDrive;
	setDrive(drv);
	msResult = mscDriveCapacity(drv);
	msDriveInfo[drv].capacity.Blocks = swap32(msDriveInfo[drv].capacity.Blocks);
	msDriveInfo[drv].capacity.BlockSize = swap32(msDriveInfo[drv].capacity.BlockSize);
	setDrive(tempDrv);
	return msResult;
}

// Get drive capacity (Sector size and count) from decice.
uint8_t mscDriveCapacity(uint8_t drv) {
	if(CBWTag == 0xFFFFFFFF)
		CBWTag = 1;
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = sizeof(msSCSICapacity_t),
		.Flags              = CMDDIRDATAIN,
		.LUN                = currentLUN,
		.CommandLength      = 10,
		.CommandData        = {CMDRDCAPACITY10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
	};
 	myMSCtransfers[xferIndex].CBW = &CommandBlockWrapper;
	return msTransfer(&myMSCtransfers[xferIndex], &msDriveInfo[drv].capacity);
}

// Retrieve device inquiry data.
uint8_t getDriveInquiry(uint8_t drv) {
	uint8_t msResult, tempDrv = currentDrive;
	setDrive(drv);
	msResult = mscDriveInquiry(drv);
	setDrive(tempDrv);
	return msResult;
}

// Get drive information and parameters from device.
uint8_t mscDriveInquiry(uint8_t drv) {
	if(CBWTag == 0xFFFFFFFF)
		CBWTag = 1;
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = sizeof(msInquiryResponse_t),
		.Flags              = CMDDIRDATAIN,
		.LUN                = currentLUN,
		.CommandLength      = 6,
		.CommandData        = {CMDINQUIRY,0x00,0x00,0x00,sizeof(msInquiryResponse_t),0x00}
	};
 	myMSCtransfers[xferIndex].CBW = &CommandBlockWrapper;
	return msTransfer(&myMSCtransfers[xferIndex], &msDriveInfo[drv].inquiry);
}

//---------------------------------------------------------------------------
// Report LUNs.
uint8_t mscReportLUNs(uint8_t *Buffer)
{
	if(CBWTag == 0xFFFFFFFF)
		CBWTag = 1;
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = MAXLUNS,
		.Flags              = CMDDIRDATAIN,
		.LUN                = currentLUN,
		.CommandLength      = 12,
		.CommandData        = {CMDREPORTLUNS, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, MAXLUNS, 0x00, 0x00}
	};
 	myMSCtransfers[xferIndex].CBW = &CommandBlockWrapper;
	return msTransfer(&myMSCtransfers[xferIndex], Buffer);
}

//---------------------------------------------------------------------------
// Start/Stop unit.
uint8_t msStartStopUnit(uint8_t mode) {
	if(CBWTag == 0xFFFFFFFF)
		CBWTag = 1;
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = 0,
		.Flags              = CMDDIRDATAIN,
		.LUN                = currentLUN,
		.CommandLength      = 6,
		.CommandData        = {CMDSTARTSTOPUNIT, 0x01, 0x00, 0x00, mode, 0x00}
	};
 	myMSCtransfers[xferIndex].CBW = &CommandBlockWrapper;
	return msTransfer(&myMSCtransfers[xferIndex], NULL);
}


//---------------------------------------------------------------------------
// Test Unit Ready.
uint8_t msTestReady(uint8_t drv) {
	if(CBWTag == 0xFFFFFFFF)
		CBWTag = 1;
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = 0,
		.Flags              = CMDDIRDATAIN,
		.LUN                = currentLUN,
		.CommandLength      = 6,
		.CommandData        = {CMDTESTUNITREADY, 0x00, 0x00, 0x00, 0x00, 0x00}
	};
 	myMSCtransfers[xferIndex].CBW = &CommandBlockWrapper;
	return msTransfer(&myMSCtransfers[xferIndex], NULL);
}

// Proccess Possible SCSI errors.
uint8_t msProcessError(uint8_t msStatus) {
	uint8_t msResult = 0;
	
	switch(msStatus) {
		case MS_CBW_PASS:
			return MS_CBW_PASS;
			break;
		case MS_CBW_PHASE_ERROR:
			Serial.printf("SCSI Phase Error: %d\n",msStatus);
			return MS_SCSI_ERROR;
			break;
		case MS_CSW_TAG_ERROR:
			Serial.printf("CSW Tag Error: %d\n",MS_CSW_TAG_ERROR);
			return MS_CSW_TAG_ERROR;
			break;
		case MS_CSW_SIG_ERROR:
			Serial.printf("CSW Signature Error: %d\n",MS_CSW_SIG_ERROR);
			return MS_CSW_SIG_ERROR;
			break;
		case MS_CBW_FAIL:
			msResult = mscDriveSense(&msSense);
			if(msResult) return msResult;
			switch(msSense.SenseKey) {
				case MSUNITATTENTION:
					switch(msSense.AdditionalSenseCode) {
						case MSMEDIACHANGED:
							return MSMEDIACHANGEDERR;
							break;
						default:
							msStatus = MSUNITNOTREADY;
					}
				case MSNOTREADY:
					switch(msSense.AdditionalSenseCode) {
						case MSMEDIUMNOTPRESENT:
							msStatus = MSNOMEDIAERR;
							break;
						default:
							 msStatus = MSUNITNOTREADY;
					}
				case MSILLEGALREQUEST:
					switch(msSense.AdditionalSenseCode) {
						case MSLBAOUTOFRANGE:
							msStatus = MSBADLBAERR;
							break;
						default:
							msStatus = MSCMDERR;
					}
				default:
					msStatus = MS_SCSI_ERROR;
			}
		default:
			Serial.printf("SCSI Error: %d\n",msStatus);
			return msStatus;
	}
}
