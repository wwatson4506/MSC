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

//#define print   USBHost::print_
//#define println USBHost::println_

extern USBHost myusb;
msController msDrive1(myusb);
msController msDrive2(myusb);
msController msDrive3(myusb);

msSCSICapacity_t msCapacity;
msInquiryResponse_t msInquiry;
msRequestSenseResponse_t msSense;

// A small hex dump funcion
void hexDump(const void *ptr, uint32_t len)
{
  uint32_t  i = 0, j = 0;
  uint8_t   c=0;
  const uint8_t *p = (const uint8_t *)ptr;

  Serial.printf("BYTE      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
  Serial.printf("---------------------------------------------------------\n");
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
    Serial.println();
  }

}

// Initialize Mass Storage Device
uint8_t mscInit(void) {
	uint8_t msResult = 1;

	while(!msDrive1.available());
	msDrive1.msReset();
	delay(1000);
	Serial.printf("## mscInit before msgGetMaxLun: %d\n", msResult);
	msResult = msDrive1.msGetMaxLun();
	Serial.printf("## mscInit after msgGetMaxLun: %d\n", msResult);
	delay(150);
	//-------------------------------------------------------
//	msDrive1.msStartStopUnit(0);
	msResult = msDrive1.WaitMediaReady();
	if(msResult)
		return msResult;
//	msResult = getDriveSense(&msSense);
//	hexDump(&msSense,sizeof(msSense));


	msResult = msDrive1.msDeviceInquiry(&msInquiry);
	if(msResult)
		return msResult;
	//-------------------------------------------------------
	msResult = msDrive1.msReadDeviceCapacity(&msCapacity);
	if(msResult)
		return msResult;
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
uint8_t readSectors(void *sectorBuffer,uint32_t BlockAddress, uint8_t Blocks) {
	uint8_t msResult = 0;
	msResult = msDrive1.msReadBlocks(BlockAddress,Blocks,512,sectorBuffer);
	return msResult;
}

// Write Sectors
uint8_t writeSectors(void *sectorBuffer,uint32_t BlockAddress, uint8_t Blocks) {
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

