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

#include "USBHost_t36.h"
#include "MassStorage.h"

//--------------------------------------------------------------------------
class msController : public USBDriver {
public:
	msController(USBHost &host) { init(); }
	msController(USBHost *host) { init(); }
	msDriveInfo_t msDriveInfo;
	bool mscTransferComplete = false;
	uint8_t mscInit(void);
	void msReset();
	uint8_t msGetMaxLun();
	void msCurrentLun(uint8_t lun) {currentLUN = lun;}
	uint8_t msCurrentLun() {return currentLUN;}
	bool available() { delay(0); return deviceAvailable; }
	uint8_t checkConnectedInitialized(void);
	uint16_t getIDVendor() {return idVendor; }
	uint16_t getIDProduct() {return idProduct; }
	uint8_t getHubNumber() { return hubNumber; }
	uint8_t getHubPort() { return hubPort; }
	uint8_t getDeviceAddress() { return deviceAddress; }
	uint8_t WaitMediaReady();
	uint8_t msTestReady();
	uint8_t msReportLUNs(uint8_t *Buffer);
	uint8_t msStartStopUnit(uint8_t mode);
	uint8_t msReadDeviceCapacity(msSCSICapacity_t * const Capacity);
	uint8_t msDeviceInquiry(msInquiryResponse_t * const Inquiry);
	uint8_t msProcessError(uint8_t msStatus);
	uint8_t msRequestSense(msRequestSenseResponse_t * const Sense);
	uint8_t msRequestSense(void *Sense);

	uint8_t msReadBlocks(const uint32_t BlockAddress, const uint16_t Blocks,
						 const uint16_t BlockSize, void * sectorBuffer);
	uint8_t msWriteBlocks(const uint32_t BlockAddress, const uint16_t Blocks,
                        const uint16_t BlockSize,	void * sectorBuffer);
protected:
	virtual bool claim(Device_t *device, int type, const uint8_t *descriptors, uint32_t len);
	virtual void control(const Transfer_t *transfer);
	virtual void disconnect();
	static void callbackIn(const Transfer_t *transfer);
	static void callbackOut(const Transfer_t *transfer);
	void new_dataIn(const Transfer_t *transfer);
	void new_dataOut(const Transfer_t *transfer);
	void init();
	uint8_t msDoCommand(msCommandBlockWrapper_t *CBW, void *buffer);
	uint8_t msGetCSW(void);
private:
	Pipe_t mypipes[3] __attribute__ ((aligned(32)));
	Transfer_t mytransfers[7] __attribute__ ((aligned(32)));
	strbuf_t mystring_bufs[1];
	uint32_t packetSizeIn;
	uint32_t packetSizeOut;
	Pipe_t *datapipeIn;
	Pipe_t *datapipeOut;
	uint32_t endpointIn = 0;
	uint32_t endpointOut = 0;
	setup_t setup;
	uint8_t report[8];
	uint8_t maxLUN = 0;
	uint8_t currentLUN = 0;
	msSCSICapacity_t msCapacity;
	msInquiryResponse_t msInquiry;
	msRequestSenseResponse_t msSense;
	uint16_t idVendor = 0;
	uint16_t idProduct = 0;
	uint8_t hubNumber = 0;
	uint8_t hubPort = 0;
	uint8_t deviceAddress = 0;
	volatile bool msOutCompleted = false;
	volatile bool msInCompleted = false;
	volatile bool msControlCompleted = false;
	uint32_t CBWTag = 0;
	bool deviceAvailable = false;
};

#endif
