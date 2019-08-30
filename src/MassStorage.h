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

// Command Block Wrapper Struct
typedef struct
{
	uint32_t Signature;
	uint32_t Tag;
	uint32_t TransferLength;
	uint8_t Flags;
	uint8_t LUN;
	uint8_t CommandLength;
	uint8_t CommandData[16];
}  __attribute__((packed)) msCommandBlockWrapper_t;

// MSC Command Status Wrapper Struct
typedef struct
{
	uint32_t Signature;
	uint32_t Tag;
	uint32_t DataResidue;
	uint8_t  Status;
}  __attribute__((packed)) msCommandStatusWrapper_t;

// MSC Device Capacity Struct
typedef struct
{
	uint32_t Blocks;
	uint32_t BlockSize;
} msSCSICapacity_t;

// MSC Inquiry Command Reponse Struct
typedef struct
{
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

// Request Sense Response Struct
typedef struct
{
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
uint8_t mscInit(void);
bool deviceAvailable(void);
bool deviceInitialized(void);
uint8_t	WaitDriveReady(void);
uint8_t readSectors(void *sectorBuffer,uint32_t BlockAddress, uint16_t Blocks);
uint8_t writeSectors(void *sectorBuffer,uint32_t BlockAddress, uint16_t Blocks);
uint8_t msProcessError(uint8_t msStatus);
msSCSICapacity_t *getDriveCapacity(void);
msInquiryResponse_t *getDriveInquiry(void);
uint8_t getDriveSense(msRequestSenseResponse_t *mscSense);
#ifdef __cplusplus
}
#endif

// C++ prototypes
void hexDump(const void *ptr, uint32_t len);
uint8_t mscInit(void);
bool deviceAvailable(void);
bool deviceInitialized(void);
uint8_t	WaitDriveReady(void);
uint8_t readSectors(void *sectorBuffer,uint32_t BlockAddress, uint16_t Blocks);
uint8_t writeSectors(void *sectorBuffer,uint32_t BlockAddress, uint16_t Blocks);
uint8_t msProcessError(uint8_t msStatus);
msSCSICapacity_t *getDriveCapacity(void);
msInquiryResponse_t *getDriveInquiry(void);
uint8_t getDriveSense(msRequestSenseResponse_t *mscSense);
#endif //_MASS_STORAGE_H_

