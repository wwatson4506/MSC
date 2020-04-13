/* MSC Teensy36 USB Host Mass Storage library
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

//MassStorageDriver.cpp

#include <Arduino.h>
#include "msc.h"
#include "MassStorage.h"

#define print   USBHost::print_
#define println USBHost::println_


void msController::init()
{
	contribute_Pipes(mypipes, sizeof(mypipes)/sizeof(Pipe_t));
	contribute_Transfers(mytransfers, sizeof(mytransfers)/sizeof(Transfer_t));
	contribute_String_Buffers(mystring_bufs, sizeof(mystring_bufs)/sizeof(strbuf_t));
	driver_ready_for_device(this);
}

bool msController::claim(Device_t *dev, int type, const uint8_t *descriptors, uint32_t len)
{
	println("msController claim this=", (uint32_t)this, HEX);
	// only claim at interface level

	if (type != 1) return false;
//    if(dev->idVendor != 0x0BC2) return false;
	if (len < 9+7+7) return false; // Interface descriptor + 2 endpoint decriptors 
	print_hexbytes(descriptors, len);

	uint32_t numendpoint = descriptors[4];
	if (numendpoint < 1) return false; 
	if (descriptors[5] != 8) return false; // bInterfaceClass, 8 = MASS Storage class
	if (descriptors[6] != 6) return false; // bInterfaceSubClass, 6 = SCSI transparent command set (SCSI Standards)
	if (descriptors[7] != 80) return false; // bInterfaceProtocol, 80 = BULK-ONLY TRANSPORT

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
	datapipeIn->callback_function = callbackIn;
	datapipeOut->callback_function = callbackOut;
	idVendor = dev->idVendor;
	idProduct = dev->idProduct;
	hubNumber = dev->hub_address;
	deviceAddress = dev->address;
	hubPort = dev->hub_port; // Used for device ID with multiple drives.
	msOutCompleted = false;
	msInCompleted = false;
	msControlCompleted = false;
	deviceAvailable = true;
	deviceInitialized = true;
	return true;
}

void msController::disconnect()
{
	deviceAvailable = false;
	deviceInitialized = false;
	println("Device Disconnected...");
	deviceDisconnected((uint32_t)this);
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

void msController::new_dataOut(const Transfer_t *transfer)
{
	uint32_t len = transfer->length - ((transfer->qtd.token >> 16) & 0x7FFF);
	println("msController dataOut (static)", len, DEC);
	print_hexbytes((uint8_t*)transfer->buffer, (len < 32)? len : 32 );
	msOutCompleted = true; // Last out transaction is completed.
	if (completedFunction) {
		completedFunction(true);
	};
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
	if (completedFunction) {
		completedFunction(true);
	};
}

//---------------------------------------------------------------------------
// Perform Mass Storage Reset
void msController::msReset() {
	mk_setup(setup, 0x21, 0xff, 0, 0, 0);
	queue_Control_Transfer(device, &setup, NULL, this);
}

//---------------------------------------------------------------------------
// Get MAX LUN
uint8_t msController::msGetMaxLun() {
	report[0] = 0;
	mk_setup(setup, 0xa1, 0xfe, 0, 0, 1);
	msControlCompleted = false;
	queue_Control_Transfer(device, &setup, report, this);
	maxLUN = report[0];
	return maxLUN;
}

//---------------------------------------------------------------------------
// Test for completed in transfer 
bool msController::inTransferComplete() {
	if(!msInCompleted)
		return false;
	msInCompleted = false;
	return true;
}

//---------------------------------------------------------------------------
// Test for completed out transfer 
bool msController::outTransferComplete() {
	if(!msOutCompleted)
		return false;
	msOutCompleted = false;
	return true;
}

//---------------------------------------------------------------------------
// Send SCSI Command
void msController::msDoCommand(msCommandBlockWrapper_t *CBW)
{
	queue_Data_Transfer(datapipeOut, CBW, sizeof(msCommandBlockWrapper_t), this);
}

//---------------------------------------------------------------------------
// Retrieve SCSI Command Status Wapper
void msController::msGetCSW(msCommandStatusWrapper_t *CSW)
{
	queue_Data_Transfer(datapipeIn, CSW, sizeof(msCommandStatusWrapper_t), this);
}

//---------------------------------------------------------------------------
// Send SCSI data
//void msController::msTxData(msCommandBlockWrapper_t *CBW,	void *buffer) {
void msController::msTxData(MSC_transfer_t *transfer) {
	queue_Data_Transfer(datapipeOut, transfer->buffer, transfer->CBW->TransferLength, this);
}

//---------------------------------------------------------------------------
// Recieve SCSI data
//void msController::msRxData(msCommandBlockWrapper_t *CBW,	void *buffer) {
void msController::msRxData(MSC_transfer_t *transfer) {
	queue_Data_Transfer(datapipeIn, transfer->buffer, transfer->CBW->TransferLength, this);
}

