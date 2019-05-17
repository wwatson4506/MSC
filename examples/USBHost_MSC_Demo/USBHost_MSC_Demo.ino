// Copyright 2016 by Walter Zimmer
// Copied from uSDFS_test.ino and
// modified for use with USBHost_t36 and msc
// 2019 by Warren Watson

#include <wchar.h>
#include "ff.h"                   // File System
#include <USBHost_t36.h>
#include "MassStorage.h"

#define USE_USB_SERIAL
#ifdef USE_USB_SERIAL
	#define SERIALX Serial
#else
	#define SERIALX Serial1
#endif

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
USBHub hub3(myusb);
USBHub hub4(myusb);

/* Stop with dying message */
void die(const char *text, FRESULT rc)
{ SERIALX.printf("%s: Failed with rc=%u.\r\n", text,rc);  for (;;) delay(100); }

FRESULT rc;        /* Result code */
FATFS fatfs0, *fs0;      /* File system object */
FATFS fatfs1, *fs1;      /* File system object */
FIL fil;        /* File object */
DIR dir;        /* Directory object */
FILINFO fno;      /* File information object */
UINT bw, br;
TCHAR buff[128];
TCHAR label[64]; /* Pointer to a buffer to return the volume label */
DWORD vsn = 0;	 /* Pointer to a variable to return the volume serial number */
uint32_t fre_clust, fre_sect, tot_sect;

msSCSICapacity_t *diskCapacity;
msInquiryResponse_t *diskInquiry;

#define BUFFSIZE (2*512) // 1024 

uint8_t buffer[BUFFSIZE]__attribute__( ( aligned ( 16 ) ) );
UINT wr;

uint32_t start = 0 , end = 0;

// the following is needed to print TCHAR strings (16 bit) on Teensy
// exFAT requires 16 bit TCHAR  
char text[80];
char * tchar2char( TCHAR * tcharString, int nn, char * charString)
{   int ii;
    for(ii = 0; ii<nn; ii++) 
    { charString[ii] = (char)tcharString[ii];
      if(!charString[ii]) break;
    }
    return charString;
}

uint64_t getDfree(uint8_t drive) {
	switch(drive)
	{
		case 0:
			f_getfree("0:", &fre_clust, &fs0);
			tot_sect = (fs0->n_fatent - 2) * fs0->csize;
			fre_sect = fre_clust * fs0->csize;
			break;
		case 1:
			f_getfree("1:", &fre_clust, &fs1);
			tot_sect = (fs1->n_fatent - 2) * fs1->csize;
			fre_sect = fre_clust * fs1->csize;
			break;
		default:
			break;
	}
	return (uint64_t)(fre_sect / 2);
}

void rwDirTest(uint8_t drive) {
  
	switch(drive) {
		case 0:
			rc = f_chdrive("0:/"); // Change to Device 0:/
			f_getlabel("0:/", label, &vsn);  // Get Volume lablel and serial #
			break;
		case 1:
			rc = f_chdrive("1:/"); // Change to Device 1:/
			f_getlabel("1:/", label, &vsn);  // Get Volume lablel and serial #
			break;
		default:
			rc = f_chdrive("0:/");  // Make device 0:/ the default drive
	}
	if(rc) die("Change Drive ",rc); 

	SERIALX.printf("Volume Label: %s\n", label);
	SERIALX.printf("Volume Serial Number: %lu\n\n",vsn);
	// Show stats for Mass Storage Device
	if(drive == 0) {
		diskCapacity = getDriveCapacity();
		SERIALX.printf("Removable Device: ");
		diskInquiry = getDriveInquiry();
		if(diskInquiry->Removable == 1)
			SERIALX.printf("YES\n");
		else
			SERIALX.printf("NO\n");
		SERIALX.printf("        VendorID: %8.8s\n",diskInquiry->VendorID);
		SERIALX.printf("       ProductID: %16.16s\n",diskInquiry->ProductID);
		SERIALX.printf("      RevisionID: %4.4s\n\n",diskInquiry->RevisionID);
		SERIALX.printf("    Sector Count: %ld\n",diskCapacity->Blocks);
		SERIALX.printf("     Sector size: %ld\n",diskCapacity->BlockSize);
		SERIALX.printf("   Disk Capacity: %ld * %ld Bytes\n",diskCapacity->Blocks, diskCapacity->BlockSize);
		SERIALX.printf("      Free Space: %.Lf Kilo Bytes\n",(double_t)getDfree(0)); 
	}
  start = millis();
  SERIALX.println("\nCreate a new file (hello10.txt).");
  rc = f_open(&fil, (TCHAR*)_T("HELLO10.TXT"), FA_WRITE | FA_CREATE_ALWAYS);
  if (rc) die("Open",rc);

  SERIALX.println("Write some text lines. (Hello world!)");
  bw = f_puts((TCHAR*)_T("Hello world!\n"), &fil);
  bw = f_puts((TCHAR*)_T("Second Line\n"), &fil);
  bw = f_puts((TCHAR*)_T("Third Line\n"), &fil);
  bw = f_puts((TCHAR*)_T("Fourth Line\n"), &fil);
  bw = f_puts((TCHAR*)_T("Habe keine Phantasie\n"), &fil);
  if (rc) die("Puts",rc);
  
  SERIALX.println("Close the file.");
  rc = f_close(&fil);
  if (rc) die("Close",rc);

  SERIALX.println("\nOpen same file (hello10.txt).");
  rc = f_open(&fil, (TCHAR*) _T("HELLO10.TXT"), FA_READ);
  if (rc) die("Open",rc);

  SERIALX.println("Get the file content.");
  for (;;) 
  {
      if(!f_gets(buff, sizeof(buff), &fil)) break; /* Read a string from file */
      SERIALX.printf("%s",tchar2char(buff,80,text));
  }
  if (rc) die("Gets",rc);

  SERIALX.println("Close the file.");
  rc = f_close(&fil);
  if (rc) die("Close",rc);
  
  //-----------------------------------------------------------
  SERIALX.println("\nopen binary file");
  // open new file
  rc = f_open(&fil, (TCHAR*)_T("test00.bin"), FA_WRITE | FA_CREATE_ALWAYS);
  if (rc) die("Open",rc);
  SERIALX.println("write file");
       
  // fill buffer
  for(int ii=0;ii<BUFFSIZE;ii++) buffer[ii]='0';
    //write data to file 
    rc = f_write(&fil, buffer, BUFFSIZE, &wr);
    if (rc) die("Write1",rc);  
      // fill buffer
      for(int ii=0;ii<BUFFSIZE;ii++) buffer[ii]='1';
        //write data to file 
        rc = f_write(&fil, buffer, BUFFSIZE, &wr);
        if (rc) die("Write2",rc);  
       
      //close file
  SERIALX.println("close file");
      rc = f_close(&fil);
      if (rc) die("Close",rc);
  SERIALX.println("Binary test done");

  //-----------------------------------------------------------
  SERIALX.println("\nOpen root directory.");
  rc = f_opendir(&dir, (TCHAR*)_T(""));
  if (rc) die("Dir",rc);

   SERIALX.println("Directory listing...");
  for (;;) 
  {
      rc = f_readdir(&dir, &fno);   /* Read a directory item */
      if (rc || !fno.fname[0]) break; /* Error or end of dir */
      if (fno.fattrib & AM_DIR)
           SERIALX.printf("   <dir>  %s\r\n", tchar2char(fno.fname,80,text));
      else
           SERIALX.printf("%8d  %s\r\n", (int)fno.fsize, tchar2char(fno.fname,80,text)); // fsize is QWORD for exFAT
	  delay(10);
  }
  if (rc) die("Listing",rc);
  end = millis()-start;
  SERIALX.printf("\nDone in %dms\n",end);
	
}
 
/****************** setup ******************************/
void setup() {
  // put your setup code here, to run once:

  while(!SERIALX);
  #ifndef USB_SERIAL
  	SERIALX.begin(115200);
  #endif
  SERIALX.printf("Mass Storage Contoller with uSDFS\n");
  myusb.begin();
 SERIALX.printf("-----------------------------------------\n");
  SERIALX.printf("If USB device is not connected, please connect now.\n");
  SERIALX.printf("Note: USB HD and some flash drives\n");
  SERIALX.printf("      can take several seconds to\n");
  SERIALX.printf("      come online.\n");
  SERIALX.printf("Please Wait...\n");
  WaitDriveReady(); // Wait for device to appear

  SERIALX.println("Mounting MSC Device 0:/");
 // 0: is first disk (configured in uSDconfig.h)
  TCHAR *device0 = (TCHAR *)_T("0:/");
  rc = f_mount(&fatfs0, device0, 0);			// Mount the File System
  if(rc != 0) {
	SERIALX.printf("Filesystem Mount Failed with code: %d\n",rc);
  }
  SERIALX.println("Mounting SDHC Device 1:/");
  // 1: is second disk (configured in uSDconfig.h)
  TCHAR *device1 = (TCHAR *)_T("1:/");
  rc = f_mount(&fatfs1, device1, 0);			// Mount the File System
  if(rc != 0) {
	SERIALX.printf("Filesystem Mount Failed with code: %d\n",rc);
  }
}
/****************** loop ******************************/
void loop() {
  // put your main code here, to run repeatedly:
	SERIALX.printf("\n-----------------------------------------\n");
	SERIALX.printf("Press a key to test Mass Storage Drive...\n");
	while(SERIALX.available() <= 0);
	SERIALX.read();
	if(!deviceAvailable()) {
		SERIALX.printf("USB drive is not connected!\n");
		SERIALX.printf("Please Connect USB drive\n");
		WaitDriveReady(); // Wait for drive connection then Init.
  }
    rwDirTest(0); // MSC drive
	SERIALX.printf("\n-----------------------------------------\n");
	SERIALX.printf("Press a key to test SDHC Drive...\n");
	while(SERIALX.available() <= 0);
	SERIALX.read();
	rwDirTest(1); // SDIO drive
}
