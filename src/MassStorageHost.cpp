/*
 * MSC Teensy36 USB Host Mass Storage library
 * Copyright (c) 2017-2019 Warren Watson.
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
//#include "USBHost_t36.h"
#include "msc.h"
#include "MassStorage.h"

//#define DEBUG_MSC
//#define DEBUG_MSC_VERBOSE

#ifndef DEBUG_MSC
#undef DEBUG_MSC_VERBOSE
void inline DBGPrintf(...) {};
#else
#define DBGPrintf Serial1.printf
#endif

#ifndef DEBUG_MSC_VERBOSE
void inline VDBGPrintf(...) {};
void inline DBGHexDump(const void *ptr, uint32_t len) {};
#else
#define VDBGPrintf Serial1.printf
#define DBGHexDump hexDump
#endif

//#define print   USBHost::print_
//#define println USBHost::println_

extern USBHost myusb;
msController msDrive1(myusb);
msController msDrive2(myusb);
msController msDrive3(myusb);

msSCSICapacity_t msCapacity;
msInquiryResponse_t msInquiry;
msRequestSenseResponse_t msSense;

// A small hex dump function
void hexDump(const void *ptr, uint32_t len)
{
  uint32_t  i = 0, j = 0;
  uint8_t   c=0;
  const uint8_t *p = (const uint8_t *)ptr;

  Serial1.printf("BYTE      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n\r");
  Serial1.printf("---------------------------------------------------------\n\r");
  for(i = 0; i <= (len-1); i+=16) {
   Serial1.printf("%4.4x      ",i);
   for(j = 0; j < 16; j++) {
      c = p[i+j];
      Serial1.printf("%2.2x ",c);
    }
    Serial1.printf("  ");
    for(j = 0; j < 16; j++) {
      c = p[i+j];
      if(c > 31 && c < 127)
        Serial1.printf("%c",c);
      else
        Serial1.printf(".");
    }
    Serial1.println();
  }

}

// Initialize Mass Storage Device
uint8_t mscInit(void) {
	uint8_t msResult = 0;

	while(!msDrive1.available());
	msDrive1.msReset();
	delay(1000);
	DBGPrintf("## mscInit before msgGetMaxLun: %d\n\r", msResult);
	uint8_t maxLUN = msDrive1.msGetMaxLun();
	DBGPrintf("## mscInit after msgGetMaxLun: %d\n\r", msResult);
	delay(150);
	//-------------------------------------------------------
	msResult = msDrive1.msStartStopUnit(1);
	msResult = msDrive1.WaitMediaReady();
	if(msResult)
		return msResult;
	// Test to see if multiple LUN see if we find right data...
	for (uint8_t currentLUN=0; currentLUN <= maxLUN; currentLUN++) {
		msDrive1.msCurrentLun(currentLUN);

		msResult = msDrive1.msDeviceInquiry(&msInquiry);
		DBGPrintf("## mscInit after msDeviceInquiry LUN(%d): Device Type: %d result:%d\n\r", currentLUN, msInquiry.DeviceType, msResult);
		if(msResult)
			return msResult;
		DBGHexDump(&msInquiry,sizeof(msInquiry));
		if (msInquiry.DeviceType == 0)
			break;
	} 

	// if device is not like hard disk, probably won't work! example CDROM... 
	if (msInquiry.DeviceType != 0)
		return MS_CSW_TAG_ERROR;


	//-------------------------------------------------------
	msResult = msDrive1.msReadDeviceCapacity(&msCapacity);
	DBGPrintf("## mscInit after msReadDeviceCapacity: %d\n\r", msResult);
	if(msResult)
		return msResult;
	DBGHexDump(&msCapacity,sizeof(msCapacity));
	return msResult;
}

// Wait for drive to be usable
bool deviceAvailable(void) {
	return msDrive1.available();   // Check for if device is connected
}

// Check if drive has been initialized
bool deviceInitialized(void) {
	return msDrive1.initialized();
}

// Wait for drive to be usable
uint8_t WaitDriveReady(void) {
	while(!msDrive1.available());     // Check drive is online
	if(!msDrive1.initialized())
		mscInit();  				  // Init drive if needed.
	return msDrive1.WaitMediaReady(); // Wait for it to be ready
}

// Read Sectors
uint8_t readSectors(void *sectorBuffer,uint32_t BlockAddress, uint16_t Blocks) {
	uint8_t msResult = 0;
	msResult = msDrive1.msReadBlocks(BlockAddress,Blocks,(uint16_t)512,sectorBuffer);
	return msResult;
}

// Write Sectors
uint8_t writeSectors(void *sectorBuffer,uint32_t BlockAddress, uint16_t Blocks) {
	uint8_t msResult = 0;
	msResult = msDrive1.msWriteBlocks(BlockAddress,Blocks,(uint16_t)512,sectorBuffer);
	return msResult;
}

// Get drive sense response 
uint8_t getDriveSense(msRequestSenseResponse_t *mscSense) {
	uint8_t msResult = 0;
	msResult = msDrive1.msRequestSense(mscSense);
	return msResult;
}


// Get drive capacity (Sector size and count)
msSCSICapacity_t *getDriveCapacity(void) {
	msDrive1.msReadDeviceCapacity(&msCapacity);
	return &msCapacity;
}

// Get drive information and parameters 
msInquiryResponse_t *getDriveInquiry(void) {
	msDrive1.msDeviceInquiry(&msInquiry);
	return &msInquiry;
}

