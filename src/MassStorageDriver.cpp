/* MSC Teensy36 USB Host Mass Storage library
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

//MassStorageDriver.cpp

#include <Arduino.h>
//#include "USBHost_t36.h"  // Read this header first for key info
#include "msc.h"
#include "MassStorage.h"

#define print   USBHost::print_
#define println USBHost::println_

msRequestSenseResponse_t *sense;

// Big Endian/Little Endian
#define swap32(x) ((x >> 24) & 0xff) | \
				  ((x << 8) & 0xff0000) | \
				  ((x >> 8) & 0xff00) |  \
                  ((x << 24) & 0xff000000)

void msController::init()
{
	contribute_Pipes(mypipes, sizeof(mypipes)/sizeof(Pipe_t));
	contribute_Transfers(mytransfers, sizeof(mytransfers)/sizeof(Transfer_t));
	contribute_String_Buffers(mystring_bufs, sizeof(mystring_bufs)/sizeof(strbuf_t));
	driver_ready_for_device(this);
}
/*
bool msController::claim(Device_t *dev, int type, const uint8_t *descriptors, uint32_t len)
{
	println("msController claim this=", (uint32_t)this, HEX);
	// only claim at interface level

	if (type != 1) return false;
	if (len < 9+7+7) return false; // Interface descriptor + 2 endpoint decriptors 

	print_hexbytes(descriptors, len);

	uint32_t numendpoint = descriptors[4];
	if (numendpoint < 1) return false; 
	if (descriptors[5] != 8) return false; // bInterfaceClass, 8 = MASS Storage class
	if (descriptors[6] != 6) return false; // bInterfaceSubClass, 6 = SCSI transparent command set (SCSI Standards)
	if (descriptors[7] != 80) return false; // bInterfaceProtocol, 80 = BULK-ONLY TRANSPORT

	if (descriptors[10] != 5 || descriptors[17] != 5) return false; // Must have bulk-in and bulk-out endpoints

	uint8_t endpointType = (descriptors[12] | descriptors[19]) & 0x03; 
	println("endpointType = ",endpointType);
	endpointIn = descriptors[11]; // bulk-in descriptor 1 81h
	endpointOut = descriptors[18]; // bulk-out descriptor 2 02h

	if ((endpointIn & 0xF0) != 0x80) return false; // must be IN direction
	if ((endpointOut & 0xF0) != 0x00) return false; // must be OUT direction
	println("numendpoint=", numendpoint, HEX);
	println("endpointIn=", endpointIn, HEX);
	println("endpointOut=", endpointOut, HEX);

	uint32_t sizeIn = descriptors[13] | (descriptors[14] << 8);
	println("packet size in (msController) = ", sizeIn);

	uint32_t sizeOut = descriptors[20] | (descriptors[21] << 8);
	println("packet size out (msController) = ", sizeOut);
	packetSizeIn = sizeIn;	
	packetSizeOut = sizeOut;	

	uint32_t intervalIn = descriptors[15];
	uint32_t intervalOut = descriptors[22];

	println("polling intervalIn = ", intervalIn);
	println("polling intervalOut = ", intervalOut);
	datapipeIn = new_Pipe(dev, 2, endpointIn, 1, packetSizeIn, intervalIn);
	datapipeOut = new_Pipe(dev, 2, endpointOut, 0, packetSizeOut, intervalOut);

	datapipeIn->callback_function = callbackIn;
	datapipeOut->callback_function = callbackOut;

	msOutCompleted = false;
	msInCompleted = false;
	deviceAvailable = true;
	return true;
}
*/
bool msController::claim(Device_t *dev, int type, const uint8_t *descriptors, uint32_t len)
{
	println("msController claim this=", (uint32_t)this, HEX);
	// only claim at interface level

	if (type != 1) return false;
	if (len < 9+7+7) return false; // Interface descriptor + 2 endpoint decriptors 

	print_hexbytes(descriptors, len);

	uint32_t numendpoint = descriptors[4];
	if (numendpoint < 1) return false; 
	if (descriptors[5] != 8) return false; // bInterfaceClass, 8 = MASS Storage class
	if (descriptors[6] != 6) return false; // bInterfaceSubClass, 6 = SCSI transparent command set (SCSI Standards)
	if (descriptors[7] != 80) return false; // bInterfaceProtocol, 80 = BULK-ONLY TRANSPORT
#if 1
	uint8_t desc_index = 9;
	uint8_t in_index = 0xff, out_index = 0xff;

	println("numendpoint=", numendpoint, HEX);
	while (numendpoint--) {
		if ((descriptors[desc_index] != 7) || (descriptors[desc_index+1] != 5)) return false; // not an end point
		if (descriptors[desc_index+3] == 2) {  // Bulk end point
			if (descriptors[desc_index+2] & 0x80)
				in_index = desc_index;
			else
				out_index = desc_index;
		}
		desc_index += 7;	// point to next one...
	}
	if ((in_index == 0xff) || (out_index == 0xff)) return false;	// did not find end point
	endpointIn = descriptors[in_index+2]; // bulk-in descriptor 1 81h
	endpointOut = descriptors[out_index+2]; // bulk-out descriptor 2 02h

	println("endpointIn=", endpointIn, HEX);
	println("endpointOut=", endpointOut, HEX);

	uint32_t sizeIn = descriptors[in_index+4] | (descriptors[in_index+5] << 8);
	println("packet size in (msController) = ", sizeIn);

	uint32_t sizeOut = descriptors[out_index+4] | (descriptors[out_index+5] << 8);
	println("packet size out (msController) = ", sizeOut);
	packetSizeIn = sizeIn;	
	packetSizeOut = sizeOut;	

	uint32_t intervalIn = descriptors[in_index+6];
	uint32_t intervalOut = descriptors[out_index+6];

	println("polling intervalIn = ", intervalIn);
	println("polling intervalOut = ", intervalOut);
	datapipeIn = new_Pipe(dev, 2, endpointIn, 1, packetSizeIn, intervalIn);
	datapipeOut = new_Pipe(dev, 2, endpointOut, 0, packetSizeOut, intervalOut);
#else	

	if (descriptors[10] != 5 || descriptors[17] != 5) return false; // Must have bulk-in and bulk-out endpoints

	uint8_t endpointType = (descriptors[12] | descriptors[19]) & 0x03; 
	println("endpointType = ",endpointType);
	endpointIn = descriptors[11]; // bulk-in descriptor 1 81h
	endpointOut = descriptors[18]; // bulk-out descriptor 2 02h

	if ((endpointIn & 0xF0) != 0x80) return false; // must be IN direction
	if ((endpointOut & 0xF0) != 0x00) return false; // must be OUT direction
	println("numendpoint=", numendpoint, HEX);
	println("endpointIn=", endpointIn, HEX);
	println("endpointOut=", endpointOut, HEX);

	uint32_t sizeIn = descriptors[13] | (descriptors[14] << 8);
	println("packet size in (msController) = ", sizeIn);

	uint32_t sizeOut = descriptors[20] | (descriptors[21] << 8);
	println("packet size out (msController) = ", sizeOut);
	packetSizeIn = sizeIn;	
	packetSizeOut = sizeOut;	

	uint32_t intervalIn = descriptors[15];
	uint32_t intervalOut = descriptors[22];

	println("polling intervalIn = ", intervalIn);
	println("polling intervalOut = ", intervalOut);
	datapipeIn = new_Pipe(dev, 2, endpointIn, 1, packetSizeIn, intervalIn);
	datapipeOut = new_Pipe(dev, 2, endpointOut, 0, packetSizeOut, intervalOut);
#endif
	datapipeIn->callback_function = callbackIn;
	datapipeOut->callback_function = callbackOut;

	msOutCompleted = false;
	msInCompleted = false;
	msControlCompleted = false;
	deviceAvailable = true;
	return true;
}

void msController::control(const Transfer_t *transfer)
{
	println("control CallbackIn (msController)");
	print_hexbytes(report, 8);
	msControlCompleted = true;

}

void msController::callbackIn(const Transfer_t *transfer)
{
	println("msController CallbackIn (static)");
	if (transfer->driver) {
		print("transfer->qtd.token = ");
		println(transfer->qtd.token & 255);
		((msController *)(transfer->driver))->new_dataIn(transfer);
	}
}

void msController::callbackOut(const Transfer_t *transfer)
{
	println("msController CallbackOut (static)");
	if (transfer->driver) {
		print("transfer->qtd.token = ");
		println(transfer->qtd.token & 255);
		((msController *)(transfer->driver))->new_dataOut(transfer);
	}
}

void msController::disconnect()
{
	deviceAvailable = false;
	deviceInitialized = false;
	println("Device Disconnected...");
}

/*
void msController::new_dataOut(const Transfer_t *transfer)
{
	println("msController dataOut (static)");
	msOutCompleted = true; // Last out transaction is completed.
}

void msController::new_dataIn(const Transfer_t *transfer)
{
	println("msController dataIn (static)");
	msInCompleted = true; // Last in transaction is completed.
}
*/

void msController::new_dataOut(const Transfer_t *transfer)
{
	uint32_t len = transfer->length - ((transfer->qtd.token >> 16) & 0x7FFF);
	println("msController dataOut (static)", len, DEC);
	print_hexbytes((uint8_t*)transfer->buffer, (len < 32)? len : 32 );
	msOutCompleted = true; // Last out transaction is completed.
}

void msController::new_dataIn(const Transfer_t *transfer)
{
	uint32_t len = transfer->length - ((transfer->qtd.token >> 16) & 0x7FFF);
	println("msController dataIn (static): ", len, DEC);
	print_hexbytes((uint8_t*)transfer->buffer, (len < 32)? len : 32 );
	if (len >= 4) {
		uint32_t l = *(uint32_t*)transfer->buffer;
		if (l == CBWSIGNATURE) println("** CBWSIGNATURE");
		else if (l == CSWSIGNATURE) println("** CSWSIGNATURE");
		else println("** ????");
	}


	msInCompleted = true; // Last in transaction is completed.
}

//---------------------------------------------------------------------------
// Perform Mass Storage Reset
void msController::msReset() {
	mk_setup(setup, 0x21, 0xff, 0, 0, 0);
	queue_Control_Transfer(device, &setup, NULL, this);
	deviceInitialized = true;
}

//---------------------------------------------------------------------------
// Get MAX LUN
uint8_t msController::msGetMaxLun() {

	mk_setup(setup, 0xa1, 0xfe, 0, 0, 1);
	msControlCompleted = false;
	queue_Control_Transfer(device, &setup, report, this);
	while (!msControlCompleted) ;
	maxLUN = report[0];
	return maxLUN;
}

uint8_t msController::WaitMediaReady() {
	uint8_t msResult = 0;
//	do {
		msResult = msTestReady();
//	} while(msResult);
// TODO: Process a Timeout
	return msResult;
}

/*
//---------------------------------------------------------------------------
// Send SCSI Command
uint8_t msController::msDoCommand(msCommandBlockWrapper_t *CBW,	void *buffer)
{
	if(CBWTag == 0xFFFFFFFF)
		CBWTag = 1;
	if((CBW->Flags == CMDDIRDATAIN)) { // Data from device
		queue_Data_Transfer(datapipeOut, CBW, sizeof(msCommandBlockWrapper_t), this);
		queue_Data_Transfer(datapipeIn, buffer, CBW->TransferLength, this);
		while(!msOutCompleted);  // Wait for out transaction to complete.
		while(!msInCompleted);  // Wait for in transaction to complete.
	} else { // Data to device
		queue_Data_Transfer(datapipeOut, CBW, sizeof(msCommandBlockWrapper_t), this);
		while(!msOutCompleted);  // Wait for out transaction to complete.
		queue_Data_Transfer(datapipeOut, buffer, CBW->TransferLength, this);
		while(!msOutCompleted);  // Wait for second out transaction to complete.
	}
	// Reset completion flags. 
 	msOutCompleted = false;
	msInCompleted = false;
	return msGetCSW();  // Get status of last transaction
}
*/

//---------------------------------------------------------------------------
// Send SCSI Command
uint8_t msController::msDoCommand(msCommandBlockWrapper_t *CBW,	void *buffer)
{
	uint8_t CSWResult = 0;

	if(CBWTag == 0xFFFFFFFF)
		CBWTag = 1;
	if((CBW->Flags == CMDDIRDATAIN)) { // Data from device
		queue_Data_Transfer(datapipeOut, CBW, sizeof(msCommandBlockWrapper_t), this);
		while(!msOutCompleted);  // Wait for out transaction to complete.
		msOutCompleted = false;
		queue_Data_Transfer(datapipeIn, buffer, CBW->TransferLength, this);
		while(!msInCompleted);  // Wait for in transaction to complete.
		msInCompleted = false;
		if((CSWResult = msGetCSW()) != 0)
			return CSWResult;
	} else { // Data to device
		queue_Data_Transfer(datapipeOut, CBW, sizeof(msCommandBlockWrapper_t), this);
		while(!msOutCompleted);  // Wait for out transaction to complete.
		msOutCompleted = false;
		queue_Data_Transfer(datapipeOut, buffer, CBW->TransferLength, this);
		while(!msOutCompleted);  // Wait for second out transaction to complete.
		msOutCompleted = false;
		if((CSWResult = msGetCSW()) != 0)
			return CSWResult;
	}
	return CSWResult;  // Return CSW status
}

//---------------------------------------------------------------------------
// Get Command Status Wrapper
uint8_t msController::msGetCSW() {
	uint8_t CSWResult = 0;
	msCommandStatusWrapper_t StatusBlockWrapper = (msCommandStatusWrapper_t)
	{
		.Signature = CSWSIGNATURE,
		.Tag = 0,
		.DataResidue = 0, // TODO: Proccess this if received.
		.Status = 0
	};
	queue_Data_Transfer(datapipeIn, &StatusBlockWrapper, sizeof(StatusBlockWrapper), this);
	while(!msInCompleted);
	msInCompleted = false;
	CSWResult = msCheckCSW(&StatusBlockWrapper);
	return CSWResult;
}

//---------------------------------------------------------------------------
// Process Possible Errors
uint8_t msController::msProcessError(uint8_t msStatus) {
	uint8_t msResult = 0;
	switch(msStatus) {
		case 0:
			return MS_CBW_PASS;
		case 1:
			return MS_CBW_FAIL;
		default:
			return msStatus;
	}
			
}

//---------------------------------------------------------------------------
// Check for valid CSW
uint8_t msController::msCheckCSW(msCommandStatusWrapper_t *CSW) {

	if(CSW->Signature != CSWSIGNATURE) return MS_CSW_SIG_ERROR; // Signature error
	if(CSW->Tag != CBWTag) return MS_CSW_TAG_ERROR; // Tag mismatch error
	if(CSW->Status != 0) return CSW->Status; // Actual status from last transaction 
	return MS_CBW_PASS; // Command transaction success (0)
}

//---------------------------------------------------------------------------
// Test Unit Ready
uint8_t msController::msTestReady() {
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = 0,
		.Flags              = CMDDIRDATAIN,
		.LUN                = 0,
		.CommandLength      = 6,
		.CommandData        = {CMDTESTUNITREADY, 0x00, 0x00, 0x00, 0x00, 0x00}
	};
	queue_Data_Transfer(datapipeOut, &CommandBlockWrapper, sizeof(CommandBlockWrapper), this);
	while(!msOutCompleted);
	msOutCompleted = false;
	return msGetCSW();
}

//---------------------------------------------------------------------------
// Start/Stop unit
uint8_t msController::msStartStopUnit(uint8_t mode) {
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = 0,
		.Flags              = CMDDIRDATAIN,
		.LUN                = 0,
		.CommandLength      = 6,
		.CommandData        = {CMDSTARTSTOPUNIT, 0x01, 0x00, 0x00, mode, 0x00}
	};
	queue_Data_Transfer(datapipeOut, &CommandBlockWrapper, sizeof(CommandBlockWrapper), this);
	while(!msOutCompleted);
	msOutCompleted = false;
	return msGetCSW();
}

//---------------------------------------------------------------------------
// Read Mass Storage Device Capacity (Number of Blocks and Block Size)
uint8_t msController::msReadDeviceCapacity(msSCSICapacity_t * const Capacity)
{
	uint8_t result = 0;
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = sizeof(msSCSICapacity_t),
		.Flags              = CMDDIRDATAIN,
		.LUN                = 0,
		.CommandLength      = 10,
		.CommandData        = {CMDRDCAPACITY10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
	};
	result = msDoCommand(&CommandBlockWrapper, Capacity);
	Capacity->Blocks = swap32(Capacity->Blocks);
	Capacity->BlockSize = swap32(Capacity->BlockSize);
	return result;
}

//---------------------------------------------------------------------------
// Do Mass Storage Device Inquiry
uint8_t msController::msDeviceInquiry(msInquiryResponse_t * const Inquiry)
{
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = sizeof(msInquiryResponse_t),
		.Flags              = CMDDIRDATAIN,
		.LUN                = 0,
		.CommandLength      = 6,
		.CommandData        = {CMDINQUIRY,0x00,0x00,0x00,0x00,0x00}
	};
	return msDoCommand(&CommandBlockWrapper, Inquiry);
}

//---------------------------------------------------------------------------
// Request Sense Data
uint8_t msController::msRequestSense(msRequestSenseResponse_t * const Sense)
{
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = sizeof(msRequestSenseResponse_t),
		.Flags              = CMDDIRDATAIN,
		.LUN                = 0,
		.CommandLength      = 6,
		.CommandData        = {CMDREQUESTSENSE, 0x00, 0x00, 0x00, sizeof(msRequestSenseResponse_t), 0x00}
	};
	return msDoCommand(&CommandBlockWrapper, Sense);
}

//---------------------------------------------------------------------------
// Report LUNs
uint8_t msController::msReportLUNs(uint8_t *Buffer)
{
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = 512,
		.Flags              = CMDDIRDATAIN,
		.LUN                = 0,
		.CommandLength      = 8,
		.CommandData        = {CMDREPORTLUNS, 0x00, 0x00, 0x00, 0x00, 0x00, MAXLUNS, 0x00}
	};
	return msDoCommand(&CommandBlockWrapper, Buffer);
}

//---------------------------------------------------------------------------
// Read Sectors (Multi Sector Capable)
uint8_t msController::msReadBlocks(
									const uint32_t BlockAddress,
									const uint8_t Blocks,
									const uint16_t BlockSize,
									void * sectorBuffer)
	{
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = (uint32_t)(Blocks * BlockSize),
		.Flags              = CMDDIRDATAIN,
		.LUN                = 0,
		.CommandLength      = 10,
		.CommandData        = {CMDRD10, 0x00,
							  (uint8_t)(BlockAddress >> 24),
							  (uint8_t)(BlockAddress >> 16),
							  (uint8_t)(BlockAddress >> 8),
							  (uint8_t)(BlockAddress & 0xFF),
							   0x00, 0x00, Blocks, 0x00}
	};
	return msDoCommand(&CommandBlockWrapper, sectorBuffer);
}

//---------------------------------------------------------------------------
// Write Sectors (Multi Sector Capable)
uint8_t msController::msWriteBlocks(
                                  const uint32_t BlockAddress,
                                  const uint8_t Blocks,
                                  const uint16_t BlockSize,
								  void * sectorBuffer)
	{
	msCommandBlockWrapper_t CommandBlockWrapper = (msCommandBlockWrapper_t)
	{
		.Signature          = CBWSIGNATURE,
		.Tag                = ++CBWTag,
		.TransferLength     = (uint32_t)(Blocks * BlockSize),
		.Flags              = CMDDIRDATAOUT,
		.LUN                = 0,
		.CommandLength      = 10,
		.CommandData        = {CMDWR10, 0x00,
		                      (uint8_t)(BlockAddress >> 24),
							  (uint8_t)(BlockAddress >> 16),
							  (uint8_t)(BlockAddress >> 8),
							  (uint8_t)(BlockAddress & 0xFF),
							  0x00, 0x00, Blocks, 0x00}
	};
	return msDoCommand(&CommandBlockWrapper, sectorBuffer);
}

