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
 
 // msc.h

#ifndef _MSC_H_
#define _MSC_H_

#include "USBHost_t36.h"  // Read this header first for key info
#include "MassStorage.h"

//--------------------------------------------------------------------------
class msController : public USBDriver {
public:
	msController(USBHost &host) { init(); }
	msController(USBHost *host) { init(); }
	void msReset();
	uint8_t msGetMaxLun();
//	uint8_t msCurrentLun() {return currentLUN;}
	bool available() { delay(0); return deviceAvailable; }
	bool initialized() { delay(0); return deviceInitialized; }
	uint16_t getIDVendor() {return idVendor; }
	uint16_t getIDProduct() {return idProduct; }
	uint8_t getHubNumber() { return hubNumber; }
	uint8_t getHubPort() { return hubPort; }
	uint8_t getDeviceAddress() { return deviceAddress; }
	void attachCompleted(void (*f)(bool complete)) { completedFunction = f; }
	void msDoCommand(msCommandBlockWrapper_t *CBW);
	bool inTransferComplete(void);
	bool outTransferComplete(void);
	void msTxData(MSC_transfer_t *transfer);
	void msRxData(MSC_transfer_t *transfer);
	void msGetCSW(msCommandStatusWrapper_t *CSW);

protected:
	virtual bool claim(Device_t *device, int type, const uint8_t *descriptors, uint32_t len);
	virtual void control(const Transfer_t *transfer);
	virtual void disconnect();
	static void callbackIn(const Transfer_t *transfer);
	static void callbackOut(const Transfer_t *transfer);
	void new_dataIn(const Transfer_t *transfer);
	void new_dataOut(const Transfer_t *transfer);
	void init();
private:
	Pipe_t mypipes[3] __attribute__ ((aligned(32)));
	Transfer_t mytransfers[3] __attribute__ ((aligned(32)));
	strbuf_t mystring_bufs[1];
	void (*completedFunction)(bool complete);
	uint32_t packetSizeIn;
	uint32_t packetSizeOut;
	Pipe_t *datapipeIn;
	Pipe_t *datapipeOut;
	uint32_t endpointIn = 0;
	uint32_t endpointOut = 0;
	setup_t setup;
	uint8_t report[8];
	uint8_t maxLUN = 0;
	uint16_t idVendor = 0;
	uint16_t idProduct = 0;
	uint8_t hubNumber = 0;
	uint8_t hubPort = 0;
	uint8_t deviceAddress = 0;
//	uint8_t currentLUN = 0;
	volatile bool msOutCompleted = false;
	volatile bool msInCompleted = false;
	volatile bool msControlCompleted = false;
	bool deviceAvailable = false;
	bool deviceInitialized = false;
};

#endif
