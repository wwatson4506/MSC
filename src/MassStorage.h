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
 
 //MassStorage.h

#include "Arduino.h"
#include "msc.h"

#ifndef _MASS_STORAGE_H_
#define _MASS_STORAGE_H_

// SCSI Commands (minimal set)
#define CBWSIGNATURE	 0x43425355UL
#define CSWSIGNATURE	 0x53425355UL
#define CMDDIRDATAOUT	 0x00
#define CMDDIRDATAIN	 0x80
#define CMDRDCAPACITY10	 0x25
#define CMDINQUIRY       0x12
#define CMDRD10			 0x28
#define CMDWR10			 0x2A
#define CMDTESTUNITREADY 0x00
#define CMDREQUESTSENSE  0x03
#define CMDSTARTSTOPUNIT 0x1B
#define CMDREPORTLUNS    0xA0

// Command Status Wrapper Error Codes
#define	MS_CBW_PASS 		0
#define	MS_CBW_FAIL  		1
#define	MS_CBW_PHASE_ERROR	2
#define MS_CSW_TAG_ERROR	253
#define MS_CSW_SIG_ERROR	254
#define MS_SCSI_ERROR		255

// SCSI Sense Key codes
#define MSNOTREADY			0x02
#define MSMEDIUMERROR		0x03
#define MSILLEGALREQUEST	0x05
#define MSUNITATTENTION		0x06
#define MSLBAOUTOFRANGE     0x21
#define MSMEDIACHANGED      0x28
#define MSMEDIUMNOTPRESENT  0x3A

// SCSI Error Codes
#define MSMEDIACHANGEDERR	0x2A
#define MSNOMEDIAERR		0x28
#define MSUNITNOTREADY		0x23
#define MSBADLBAERR			0x29
#define MSCMDERR			0x26

#define MAXLUNS				16

// Big Endian/Little Endian
#define swap32(x) ((x >> 24) & 0xff) | \
				  ((x << 8) & 0xff0000) | \
				  ((x >> 8) & 0xff00) |  \
                  ((x << 24) & 0xff000000)

#define MAXDRIVES 2 // Could be more but is this even practical?
#define MAXTRANSFERS 10

typedef class msController *Device_p;
#define MSC1 0
#define MSC2 1 
#define MSC3 2
#define MSC4 3

/*
Define  USE_EXTERNAL_INIT to Initialize MSC Externally.
If defined, MSC needs the following setup in the ino sketch:
 	USBHost myusb;
	myusb.begin();
    mscInit();
*/
#define USE_EXTERNAL_INIT

// Command Block Wrapper Struct
typedef struct {
	uint32_t Signature;
	uint32_t Tag;
	uint32_t TransferLength;
	uint8_t Flags;
	uint8_t LUN;
	uint8_t CommandLength;
	uint8_t CommandData[16];
}  __attribute__((packed)) msCommandBlockWrapper_t;

// MSC Command Status Wrapper Struct
typedef struct {
	uint32_t Signature;
	uint32_t Tag;
	uint32_t DataResidue;
	uint8_t  Status;
}  __attribute__((packed)) msCommandStatusWrapper_t;

// MSC Transfer Struct. (WIP)
typedef struct MSC_transfer MSC_transfer_t;
struct MSC_transfer {
	bool completed;
	void *buffer;
	msCommandBlockWrapper_t *CBW;
	msCommandStatusWrapper_t *CSW;
};

// MSC Device Capacity Struct
typedef struct {
	uint32_t Blocks;
	uint32_t BlockSize;
} __attribute__((packed)) msSCSICapacity_t;

// MSC Inquiry Command Reponse Struct
typedef struct {
	unsigned DeviceType          : 5;
	unsigned PeripheralQualifier : 3;
	unsigned Reserved            : 7;
	unsigned Removable           : 1;
	uint8_t  Version;
	unsigned ResponseDataFormat  : 4;
	unsigned Reserved2           : 1;
	unsigned NormACA             : 1;
	unsigned TrmTsk              : 1;
	unsigned AERC                : 1;
	uint8_t  AdditionalLength;
	uint8_t  Reserved3[2];
	unsigned SoftReset           : 1;
	unsigned CmdQue              : 1;
	unsigned Reserved4           : 1;
	unsigned Linked              : 1;
	unsigned Sync                : 1;
	unsigned WideBus16Bit        : 1;
	unsigned WideBus32Bit        : 1;
	unsigned RelAddr             : 1;
	uint8_t  VendorID[8];
	uint8_t  ProductID[16];
	uint8_t  RevisionID[4];
}  __attribute__((packed)) msInquiryResponse_t;

// MSC Drive status/info struct
typedef struct {
	bool connected = false;
	bool initialized = false;
	uint8_t hubNumber;
	uint8_t hubPort;
	uint8_t deviceAddress;
	uint16_t idVendor;
	uint16_t idProduct;
	msSCSICapacity_t capacity;
	msInquiryResponse_t inquiry;	
} __attribute__((packed)) msDriveInfo_t;

//static msDriveInfo_t msDriveInfo[MAXDRIVES];

// Request Sense Response Struct
typedef struct {
	uint8_t  ResponseCode;
	uint8_t  SegmentNumber;
	unsigned SenseKey            : 4;
	unsigned Reserved            : 1;
	unsigned ILI                 : 1;
	unsigned EOM                 : 1;
	unsigned FileMark            : 1;
	uint8_t  Information[4];
	uint8_t  AdditionalLength;
	uint8_t  CmdSpecificInformation[4];
	uint8_t  AdditionalSenseCode;
	uint8_t  AdditionalSenseQualifier;
	uint8_t  FieldReplaceableUnitCode;
	uint8_t  SenseKeySpecific[3];
	uint8_t  padding[234];
}  __attribute__((packed)) msRequestSenseResponse_t;

// C prototypes
#ifdef __cplusplus
extern "C" {
#endif
void hexDump(const void *ptr, uint32_t len);
uint8_t findDevices(void);
uint8_t mscInit(void);
uint8_t initDrive(uint8_t device);
void deInitDrive(uint8_t device);
uint8_t setDrive(uint8_t drive);
uint8_t getDrive(void);
uint8_t getDriveCount(void);
msDriveInfo_t *getDriveInfo(uint8_t dev);
boolean deviceAvailable(uint8_t device);
boolean deviceInitialized(uint8_t device);
bool checkDeviceConnected(uint8_t device);
uint8_t msStartStopUnit(uint8_t mode);
uint8_t msTestReady(uint8_t drv);
uint8_t WaitMediaReady(uint8_t drv);
uint8_t msTransfer(MSC_transfer_t *transfer, void *buffer);
uint8_t readSectors(void *sectorBuffer,uint32_t BlockAddress, uint16_t Blocks);
uint8_t writeSectors(void *sectorBuffer,uint32_t BlockAddress, uint16_t Blocks);
void msCurrentLun(uint8_t lun);
uint8_t mscReportLUNs(uint8_t *Buffer);
uint8_t msStartStopUnit(uint8_t mode);
uint8_t msProcessError(uint8_t msStatus);
uint8_t mscDriveInquiry(uint8_t drv);
uint8_t getDriveInquiry(uint8_t drv);
//uint8_t getDriveInquiry(msInquiryResponse_t *mscInquiry, uint8_t drv);
uint8_t mscDriveCapacity(uint8_t drv);
uint8_t getDriveCapacity(uint8_t drv);
//uint8_t getDriveCapacity(msSCSICapacity_t *mscCapacity, uint8_t drv);
uint8_t mscDriveSense(msRequestSenseResponse_t *mscSense);
#ifdef __cplusplus
}
#endif

// C++ prototypes
void hexDump(const void *ptr, uint32_t len);
uint8_t findDevices(void);
uint8_t mscInit(void);
uint8_t initDrive(uint8_t device);
void deInitDrive(uint8_t device);
uint8_t setDrive(uint8_t drive);
uint8_t getDrive(void);
uint8_t getDriveCount(void);
msDriveInfo_t *getDriveInfo(uint8_t dev);
bool checkDeviceConnected(uint8_t device);
boolean deviceAvailable(uint8_t device);
boolean deviceInitialized(uint8_t device);
uint8_t msStartStopUnit(uint8_t mode);
uint8_t WaitMediaReady(uint8_t drv);
uint8_t msTransfer(MSC_transfer_t *transfer, void *buffer);
uint8_t readSectors(void *sectorBuffer,uint32_t BlockAddress, uint16_t Blocks);
uint8_t writeSectors(void *sectorBuffer,uint32_t BlockAddress, uint16_t Blocks);
void msCurrentLun(uint8_t lun);
uint8_t mscReportLUNs(uint8_t *Buffer);
uint8_t msStartStopUnit(uint8_t mode);
uint8_t msTestReady(uint8_t drv);
uint8_t msProcessError(uint8_t msStatus);
uint8_t mscDriveInquiry(uint8_t drv);
uint8_t getDriveInquiry(uint8_t drv);
//uint8_t getDriveInquiry(msInquiryResponse_t *mscInquiry, uint8_t drv);
uint8_t mscDriveCapacity(uint8_t drv);
uint8_t getDriveCapacity(uint8_t drv);
//uint8_t getDriveCapacity(msSCSICapacity_t *mscCapacity, uint8_t drv);
uint8_t mscDriveSense(msRequestSenseResponse_t *mscSense);
#endif //_MASS_STORAGE_H_

