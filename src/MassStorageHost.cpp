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
#include "attachYieldFunc.h"

// Setup two Circular_Buffers (read/write).
Circular_Buffer<uint8_t, (uint16_t)MAX_TRANSFERS,sizeof(MSC_transfer_t)> mscRead ;
Circular_Buffer<uint8_t, (uint16_t)MAX_TRANSFERS,sizeof(MSC_transfer_t)> mscWrite;

static volatile uint8_t mscError = MS_CBW_PASS; // Setup a global Read/Write status variable.
static volatile uint32_t bytesRead = 0;   // Current transfer byte count for read or write.
static volatile uint32_t bytesWritten = 0;   // Current transfer byte count for read or write.
static volatile bool readComplete = true;		 // Flag to signal one async read finished.
static volatile bool writeComplete = true;		 // Flag to signal one async write finished.

// A small hex dump function
void msHost::hexDump(const void *ptr, uint32_t len) {
  uint32_t  i = 0, j = 0;
  uint8_t   c=0;
  const uint8_t *p = (const uint8_t *)ptr;

  Serial.printf("BYTE      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
  Serial.printf("---------------------------------------------------------\n");
  for(i = 0; i <= (len-1); i+=16) {
   Serial.printf("%4.4lx      ",i);
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

// Initialize mscHost.
void msHost::mscHostInit(void) {
	pinMode(READ_PIN, OUTPUT); // Init disk read activity indicator.
	pinMode(WRITE_PIN, OUTPUT); // Init disk write activity indicator.
	attachYieldFunc(msDispatchTransfer); // Start monitoring the queues. (Thanks luni64).
}

// Get the current total transfer read byte count
uint32_t msHost::getBytesRead(void) {
	return bytesRead;
}

// Clear total transfer read byte count
void msHost::clearBytesRead(void) {
	bytesRead = 0;
}

// Get the current total transfer written byte count
uint32_t msHost::getBytesWritten(void) {
	return bytesWritten;
}

// Clear total transfer byte count
void msHost::clearBytesWritten(void) {
	bytesWritten = 0;
}

// Get transfer status from last transaction.
uint8_t msHost::transferStatus(void) {
		return mscError;
}

// Get Read Queue Size.
uint16_t msHost::getReadQueueSize(void) {
	return mscRead.size();
}

// Get Write Queue Size.
uint16_t msHost::getWriteQueueSize(void) {
	return mscWrite.size();
}

// Add MSC sector(s) transfer to the read queue.
void msHost::mscReadToCB(const MSC_transfer_t &msTransfer) {
	uint8_t buf[sizeof(MSC_transfer_t)];
	memmove(buf, &msTransfer, sizeof(msTransfer));
	mscRead.push_back(buf, sizeof(MSC_transfer_t)); // Add transfer to end of read queue.
}

// Add MSC sector(s) transfer to the write queue.
void msHost::mscWriteToCB(const MSC_transfer_t &msTransfer) {
	uint8_t buf[sizeof(MSC_transfer_t)];
	memmove(buf, &msTransfer, sizeof(msTransfer));
	mscWrite.push_back(buf, sizeof(MSC_transfer_t)); // Add transfer to end of write queue.
}

// -------------------------------------------------------------------------------------------------
// Dispatch queued reads and writes (Circular Buffer) via attachYieldFunc() (Uses EventResponder). -
// -------------------------------------------------------------------------------------------------
void msHost::msDispatchTransfer(void) {
	msController *pDrive;  // An MSC drive instance temporary pointer.
	MSC_transfer_t rdTemp; // Temporary read transfer struct.
	MSC_transfer_t wrTemp; // Temporary write transfer struct.
	uint8_t buf[sizeof(MSC_transfer_t)];
	mscError = MS_CBW_PASS; // Clear any residual error code.

	// Return if Queues are both empty.
	if(!(mscRead.size() || mscWrite.size())) { // Is there data in the queues?
		return; // Nothing to do queues are empty. Skip it and return.
	}

	// -----------------------------------------------------------------
	// Writes should lead reads ????. Does that make sense ????        -
	//------------------------------------------------------------------
	// If data in the write queue, get the next transfer
	// and dispatch the write transfer.
	if(mscWrite.size() && readComplete) { // Make sure any read operation is complete.
#ifdef DBGqueue
		Serial.printf("mscWrite.size().size = %d\n",mscWrite.size());
#endif
		mscWrite.pop_front(buf, sizeof(MSC_transfer_t)); // Get the next transfer in the write queue.
		memmove(&wrTemp, buf, sizeof(MSC_transfer_t)); // Copy it to the temporary transfer struct.
		writeComplete = false; // Signal mscRead() to wait.
		pDrive = (msController *)wrTemp.pDrive; // Retrieve the drive instance we are using.
		// Check to see if the last MSC 3 stage transfer was completed.
		while(!pDrive->mscTransferComplete || !readComplete) yield();
		digitalWriteFast(WRITE_PIN, HIGH); // If so, turn on the red LED and proceed with dispatch.
		mscError = pDrive->msWriteBlocks(wrTemp.BlockAddress, wrTemp.Blocks, 512, wrTemp.buffer); // Perform Write op.
		digitalWriteFast(WRITE_PIN, LOW); // Turn off the red LED.
		bytesWritten += (uint32_t)(wrTemp.Blocks * 512); // Add this transfer to the total transfer count.
		writeComplete = true;  // Signal Write Complete.
	}

	// -----------------------------------------------------------------
	// Reads follow writes. Not sure if there should be checks for     -
	// writing and reading to the same blocks.                         -
	//------------------------------------------------------------------
	// If data in the read queue, get the next transfer
	// and dispatch the read transfer.
	if(mscRead.size() && writeComplete) { // Make sure any write operation is complete.
#ifdef DBGqueue
		Serial.printf("mscRead.size().size = %d\n",mscRead.size());
#endif
		mscRead.pop_front(buf, sizeof(MSC_transfer_t)); // Get the next transfer in the read queue.
		memmove(&rdTemp, buf, sizeof(MSC_transfer_t)); // Copy it to the temporary transfer struct.
		readComplete = false; // Signal mscWrite() to wait.
		pDrive = (msController *)rdTemp.pDrive; // Retrieve the drive instance we are using.
		// Check to see if the last MSC 3 stage transfer was completed.
		while(!pDrive->mscTransferComplete || !writeComplete) yield();
		digitalWriteFast(READ_PIN, HIGH); // If so, turn on the green LED and proceed with dispatch.
		mscError = pDrive->msReadBlocks(rdTemp.BlockAddress, rdTemp.Blocks, 512, rdTemp.buffer); // Perform Read op.
		digitalWriteFast(READ_PIN, LOW); // Turn off the green LED.
		bytesRead += (uint32_t)(rdTemp.Blocks * 512); // Add this transfer to the total transfer count.
		readComplete = true;  // Signal Read Complete.
	}

	// Check for errors.
	if(mscError != MS_CBW_PASS) {
		mscRead.clear();	// Stop the rest of read transfers.
		mscWrite.clear();	// Stop the rest of write transfers.
		Serial.printf("Transfer Failed - CODE: %d\n", mscError);
		return;
	}
}

// Read Sectors (Blocking).
uint8_t msHost::readSectors(void *pDrive, void *sectorBuffer,uint32_t BlockAddress, uint16_t Blocks) {
#ifdef DBGprint
	Serial.printf("readSectors()\n");
#endif
	// Check if device is plugged in and initialized
	if((mscError = ((msController *)pDrive)->checkConnectedInitialized()) != MS_CBW_PASS) {
		return mscError;
	}
	mscError = ((msController *)pDrive)->msReadBlocks(BlockAddress,Blocks,(uint16_t)512,sectorBuffer);
	bytesRead += (uint32_t)(Blocks * 512); // Add this transfer to the total transfer count.
	return mscError;
}

// Write Sectors (Blocking).
uint8_t msHost::writeSectors(void *pDrive, void *sectorBuffer,uint32_t BlockAddress, uint16_t Blocks) {
#ifdef DBGprint
	Serial.printf("writeSectors()\n");
#endif
	// Check if device is plugged in and initialized
	if((mscError = ((msController *)pDrive)->checkConnectedInitialized()) != MS_CBW_PASS) {
		return mscError;
	}
	mscError = ((msController *)pDrive)->msWriteBlocks(BlockAddress,Blocks,(uint16_t)512,sectorBuffer);
	bytesWritten += (uint32_t)(Blocks * 512); // Add this transfer to the total transfer count.
	return mscError;
}

// ---------------------------------------------------------------------------------------------------------
// Read Sectors using queued reads. (Non-Blocking).
//	*pDrive is used as a msController pointer to a USB drive object.
// ---------------------------------------------------------------------------------------------------------
uint8_t msHost::asyncReadSectors(void *pDrive, void *sectorBuffer, const uint32_t BlockAddress, const uint16_t Blocks) {
#ifdef DBGprint
	Serial.printf("asyncReadSectors()\n");
#endif
	MSC_transfer_t temp;
	uint16_t readQueueSize = 0;

	// Check if device is plugged in and initialized
	if((mscError = ((msController *)pDrive)->checkConnectedInitialized()) != MS_CBW_PASS) {
		return mscError;
	}
	// Setup a temporary transfer struct to queue up.
	temp.buffer = sectorBuffer;
	temp.BlockAddress = BlockAddress;
	temp.Blocks = Blocks;
	temp.pDrive = pDrive;
	temp.type = CMD_RD_10;	// CBW read op.	
	mscReadToCB(temp);  	// Queue the disk read.
	while((readQueueSize = getReadQueueSize()) >= (uint32_t)(MAX_TRANSFERS - 1))
		yield(); // If queue full wait. (Must yield).
	return MS_CBW_PASS; // Keep the compiler happy:)
}

// ---------------------------------------------------------------------------------------------------------
// Write Sectors using queued writes. (Non-Blocking).
//	*pDrive is used as a msController pointer to a USB drive object.
// ---------------------------------------------------------------------------------------------------------
uint8_t msHost::asyncWriteSectors(void *pDrive, void *sectorBuffer, const uint32_t BlockAddress, const uint16_t Blocks) {
#ifdef DBGprint
	Serial.printf("asyncWriteSectors()\n");
#endif
	MSC_transfer_t temp;
	uint16_t writeQueueSize = 0;
	// Check if device is plugged in and initialized
	if((mscError = ((msController *)pDrive)->checkConnectedInitialized()) != MS_CBW_PASS) {
		return mscError;
	}
	temp.buffer = sectorBuffer;
	temp.BlockAddress = BlockAddress;
	temp.Blocks = Blocks;
	temp.pDrive = pDrive;
	temp.type = CMD_WR_10;	// CBW write op.
	mscWriteToCB(temp);  	// Queue the disk write.
	while((writeQueueSize = getWriteQueueSize()) >= (uint32_t)(MAX_TRANSFERS - 1))
		yield(); // If queue full wait. (Must yield).
	return MS_CBW_PASS; // Keep the compiler happy:)
}

