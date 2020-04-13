#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "MassStorage.h"
#include "uSDFS.h"
#include "mscfs.h"
#include "time.h"

FATFS	FatFs0, *fs0 = &FatFs0;   // Work area (file system object) for logical drive 0.
FATFS	FatFs1, *fs1 = &FatFs1;   // Work area (file system object) for logical drive 1.
FATFS	FatFs2, *fs2 = &FatFs2;   // Work area (file system object) for logical drive 2.
FATFS	FatFs3, *fs3 = &FatFs3;   // Work area (file system object) for logical drive 3.
FATFS *fstemp; // Used for mounting drives.
FRESULT m_res;
FILINFO fno;
DIR		dir;
uint32_t fre_clust, fre_sect, tot_sect;  // Used with getDfree().
//TCHAR label[64];	 /* Pointer to a buffer to return the volume label */
//DWORD vsn = 0;	 /* Pointer to a variable to return the volume serial number */

const char driveName[4][4] = {"0:/","1:/","2:/","3:/"}; // 

TCHAR driveLabel[4][64]; // Pointer to a buffer to return the volume driveLabel
DWORD serialNum[4];		// Pointer to a variable to return the volume serial number

extern "C" struct tm seconds2tm(uint32_t tt);
extern "C" struct tm decode_fattime (uint16_t td, uint16_t tt);

msSCSICapacity_t *dskCapacity;
msInquiryResponse_t *dskInquiry;
msDriveInfo_t *usbDriveInfo;
bool driveOnline = false;
uint8_t localDrive = 0; //To keep track of default drive.
static uint8_t usbDrvCnt = 0;

// Find and initialize all available volumes.
void MSCFS_init(void) {
	Serial.printf("Waiting for USB drives to become available.\n");
	usbDrvCnt = mscInit();
	
	if(usbDrvCnt == 0)
		Serial.printf("No USB Drives Available\n");
	else	
		Serial.printf("%d USB Drives Available\n",usbDrvCnt);

	// Init drive label arrays
	for(uint8_t i = 1; i < 4; i++) {
		strcpy(driveLabel[i],"UnAvalialble");
		serialNum[i] = 0;
	}

	Serial.printf("Mounting Drive %s\n",driveName[1]);
	mountDrive(fs1,driveName[1], 1); // Try to mount drive 1

	if(driveAvailable(2)) {	// USB drive 0
		Serial.printf("Mounting Drive %s\n",driveName[2]);
		mountDrive(fs2,driveName[2], 2); // Try to mount drive 2
	}
	if(driveAvailable(3)) {	// USB drive 1
		Serial.printf("Mounting Drive %s\n",driveName[3]);
		mountDrive(fs3,driveName[3], 3); // Try to mount drive 3
	}
	if(usbDrvCnt != 0) {
		changeDrive(2); // Set default drive to 2:/
	} else {
		changeDrive(1); // Set default drive to 1:/
	}
}

// Mount a drive
FRESULT mountDrive(FATFS *fs, const char *driveName, uint8_t deviceNumber) {

	m_res = f_mount(fs, driveName, 1);	// Mount the File System 
	if(m_res != 0) {
		Serial.printf("Filesystem Mount Failed with code: %d, drive: %s\n",m_res, driveName);
		return m_res;
	}
	if(deviceNumber > 1) { //USB drives 2:/, 3:/
		usbDriveInfo = getDriveInfo(mapUSBDrive(deviceNumber));
		usbDriveInfo->mounted = true; // Drive successfully mounted
    }
	return m_res;
}

// UnMount a drive
FRESULT uNmountDrive(const char *driveName) {
	uint8_t drvnum = getDriveIndex(driveName);
	m_res = f_mount(0, driveName, 0);	// Mount the File System 
	if(m_res != 0) {
		Serial.printf("Filesystem UnMount Failed with code: %d, drive: %s\n",m_res, driveName);
	}
	strcpy(driveLabel[drvnum],"UnAvalialble");
	serialNum[drvnum] = 0;
	return m_res;
}

// Show USB drive information for the selected USB drive.
int showUSBDriveInfo(uint8_t drive) {
	if(drive < 2) { // Make sure drive is a USB drive (> 1).
		Serial.printf("Error: Drive %s is not a USB drive\n",driveName[drive]);
		return -1;
	}
	Serial.printf("USB DRIVE %s INFO  ",driveName[drive]);
	usbDriveInfo = getDriveInfo(mapUSBDrive(drive)); // Get a pointer to the info.
	if(usbDriveInfo->mounted == true) {// Make sure it's mounted.
		Serial.printf("Mounted\n\n");
	} else {
		Serial.printf("NOT Mounted\n\n");
		return -1;
	}
	// Now we will print out the information.
	getDriveLabels(drive);
	Serial.printf("Volume Label: %s\n", driveLabel[drive]);
	Serial.printf("Volume Serial Number: %lu\n\n",serialNum[drive]);
	Serial.printf("   USB Vendor ID: %4.4x\n",usbDriveInfo->idVendor);
	Serial.printf("  USB Product ID: %4.4x\n",usbDriveInfo->idProduct);
	Serial.printf("      HUB Number: %d\n",usbDriveInfo->hubNumber);
	Serial.printf("        HUB Port: %d\n",usbDriveInfo->hubPort);
	Serial.printf("  Device Address: %d\n",usbDriveInfo->deviceAddress);
	Serial.printf("Removable Device: ");
	if(usbDriveInfo->inquiry.Removable == 1)
		Serial.printf("YES\n");
	else
		Serial.printf("NO\n");
	Serial.printf("        VendorID: %8.8s\n",usbDriveInfo->inquiry.VendorID);
	Serial.printf("       ProductID: %16.16s\n",usbDriveInfo->inquiry.ProductID);
	Serial.printf("      RevisionID: %4.4s\n",usbDriveInfo->inquiry.RevisionID);
	Serial.printf("         Version: %d\n",usbDriveInfo->inquiry.Version);
	Serial.printf("    Sector Count: %ld\n",usbDriveInfo->capacity.Blocks);
	Serial.printf("     Sector size: %ld\n",usbDriveInfo->capacity.BlockSize);
	Serial.printf("   Disk Capacity: %.f Bytes\n",(double_t)usbDriveInfo->capacity.Blocks *
										(double_t)usbDriveInfo->capacity.BlockSize);
	Serial.printf("      Free Space: %.f Kilo Bytes\n\n",(double_t)getDfree(drive)); 
	return 0;
}

// Drive Available and mounted.
// If not mounted and it is available then attempt to mount the drive.
bool driveAvailable(uint8_t drive) {
	fstemp = assignVolume(drive); // Get the right FATFS work area pointer.
	if(drive > 1) {
		if(!checkDeviceConnected(mapUSBDrive(drive))) { // If it has been unplugged
			if(fstemp->fs_type != 0) // Is it still mounted?
				uNmountDrive(driveName[drive]); // Yes, unmount the drive.
			return false;
		} else { // The drive is available but is it mounted?
			usbDriveInfo = getDriveInfo(mapUSBDrive(drive));
			if(!usbDriveInfo->mounted) {  // No, try to mount it.
				if(mountDrive(fstemp,driveName[drive], drive) != 0)
					return false; // Failed to mount the drive. 
			}
		}
	}
	return true; // Drive is available and mounted.
}

// Set Default Device
void selectDevice(uint8_t device) {
	localDrive = device;
}

// Return the current default drive number.
uint8_t getCurrentDrive(void) {
	return localDrive;
}

// Get drive number from device string name '0:/, 1:/, 2:/ or 3:/'
int getDriveIndex(const char * drive) {
	for(uint8_t i = 0; i < 4; i++) {
		if(strcmp(driveName[i], drive) == 0)
			return i;
	}	
	return -1; // Failed: Not found.
}

// Return drive string name
const char *getDriveName(uint8_t drive){
	return driveName[drive];
}

// USB devices index starts at 0. (low level)
// Convert drive number to device number 2-->0 and 3-->1.
uint8_t mapUSBDrive(uint8_t drive) {
	if(drive > 1) drive -= 2;
	return drive;
}

// Return a pointer to the proper FATFS work area for a selected drive.
FATFS *assignVolume(uint8_t drive) {
	// Make sure drive number is between 0 and 3.
	if((drive < 0) || (drive > 3))
		return NULL;
	switch(drive) {
		case 0:
			return fs0; // SPI
			break;
		case 1:
			return fs1; // SDHC
			break;
		case 2:
			return fs2; // USB0
			break;
		case 3:
			return fs3; // USB1
			break;
		default:
			return NULL; 
			break;
	}
}

// Change drive 0:/, 1:/, 2:/, 3:/. It becomes the default drive.
int changeDrive(uint8_t drive) {
	// Check if valid drive number
	if(drive > 3) {
		Serial.printf("Invalid drive number: %d...\n", drive);
		return -1;
	}
	// Skip over non-USB and SD cards (SPI and SDHC).
	// USB drives[2,3] to check if online.
	if(drive > 1) {
		driveOnline = driveAvailable(drive); // Drive available?
		if(driveOnline)	{ // Available and mounted?
			usbDriveInfo = getDriveInfo(mapUSBDrive(drive)); // Get pointer to info struct.
		} else { // Nope, not available.
			Serial.printf("Drive not found: %s\n", driveName[drive]);
			return -1;
		}
	}
	fstemp = assignVolume(drive); // Get a pointer to the drives FATFS work area.
	if(!usbDriveInfo->mounted) {  // if drive not mounted...
		if(mountDrive(fstemp,driveName[drive], drive) != 0) return -1; // Try to remount drive 
	}
	if(f_chdrive(driveName[drive]) != 0) { // Change FatFs default drive.
		Serial.printf("Drive not found: %s\n", driveName[drive]);
		return -1;
	}
	else
		selectDevice(drive); // Set drive number as default drive.
	return 0;
}

// Get drive labels and serial numbers for a particular or all drive(S).
// If drives = 255 scan all drives.
void getDriveLabels(uint8_t drives) {
	if(drives == 255) { // Get all 4 drive labels
		for(uint8_t i = 1; i < 4; i++) {
			if((m_res = f_getlabel(driveName[i], driveLabel[i], &serialNum[i])) != FR_OK) {
				strcpy(driveLabel[i],"UnAvalialble");
				serialNum[i] = 0;
			}
		}
	} else { // Get specific drive label
		if((m_res = f_getlabel(driveName[drives], driveLabel[drives], &serialNum[drives])) != FR_OK) {
			strcpy(driveLabel[drives],"UnAvalialble");
			serialNum[drives] = 0;
		}
	}
}

// Scan files  search given directory for files/subdirectories
// Adapted from ChaN's example code for f_readdir().
FRESULT  scan_files (char  *path)
{
	int					i;
    FRESULT				res;
    DIR					dir;
    char				*fn;   // This function is assuming non-Unicode cfg.
	//  First, display all directories in order.
	res = f_opendir(&dir, path); // Open the directory.
	if (res == FR_OK) {
		for (;;) {
			res = f_readdir(&dir, &fno); // Read a directory item.
			if ((res != FR_OK) || (fno.fname[0] == 0))
			   break;  // Break on error or end of dir.
			fn = fno.fname;
			if (fno.fattrib & AM_DIR) { // It is a directory.
				Serial.printf("%s ", fn);
				i = 20-strlen(fn);
				while (i-- > 0)  Serial.printf(" ");
				Serial.printf("<DIR>\n");
			}
		}
		f_closedir(&dir);
	}
	// Now, reopen the directory and display all the files in order.
	res = f_opendir(&dir, path); /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno); // Read a directory item
            if ((res != FR_OK) || (fno.fname[0] == 0))
			    break;  // Break on error or end of dir
            fn = fno.fname;
            if ((fno.fattrib & AM_DIR) == 0) { // If this is a file...
                Serial.printf("%s ", fn);
				i = 20-strlen(fn);
				while (i-- > 0)  Serial.printf(" ");
				Serial.printf("%10.ld    ", (uint32_t)fno.fsize);
				Serial.printf("%c%c%c%c%c  ",
               (fno.fattrib & AM_DIR) ? 'D' : ' ',
               (fno.fattrib & AM_RDO) ? 'R' : ' ',
               (fno.fattrib & AM_HID) ? 'H' : ' ',
               (fno.fattrib & AM_SYS) ? 'S' : ' ',
               (fno.fattrib & AM_ARC) ? 'A' : ' ');
			  struct tm tx = decode_fattime (fno.fdate, fno.ftime);
			  Serial.printf("%4d-%02d-%02d %02d:%02d:%02d\n",
							tx.tm_year,tx.tm_mon,tx.tm_mday, 
							tx.tm_hour,tx.tm_min,tx.tm_sec);
            }
		}
		f_closedir(&dir);
	}
	return res;
}

// Get file spec and wildcard pattern
int get_wild_cards(char *specs, char *pattern)
{
	int index, count, len, i;
	
	len = strlen(specs);
	if(!len || !strchr(specs,'.'))
		return 0;
	index = len;
	while((specs[index] != '/') && (index != 0))
		index--;
	count = len - index;
	for(i = 0; i < count; i++)
		if(index)
			pattern[i] = specs[i+index+1];
		else
			pattern[i] = specs[i+index];
	pattern[i] = '\0';
	specs[index] ='\0';
	if(pattern[0])
		return 1;
	else
		return 0;
}

// Do directory listing.
// Processes drive names, directories, filenames and wildcards. Check FATFS docs.
void lsdir(char *directory) {
	int i;
	FRESULT fr;
	FILINFO fno;
	DIR dir;			// Directory object
	uint8_t drive = 2;  // Default drive number if no drive spec is given.
	TCHAR path[256];
	TCHAR pattern[256];
    char *fn;   		// This function is assuming non-Unicode cfg.
	TCHAR drv[4];
	path[0] = 0;
	pattern[0] = 0;
	int wildcards = 0;

	// Get drive '0:/-3:/'
	sprintf(path, "%s", directory);
	strncpy(drv,directory,3); // Copy drive spec into drv array for validating.
	drv[3] = 0; // Terminate drive spec string.
	// Check for a valid drive number 0-4.
	if(((drv[0] == '0') || (drv[0] == '1')) || ((drv[0] == '2') || (drv[0] == '3'))) {
		if(drv[1] == ':') { // Is there a valid drive spec?
			drive = (uint8_t)(drv[0]-48); // Convert ascii character to a uint8_t number.
			// Check for connected USB drives and mounting.
			if(drive > 1 && !driveAvailable(drive)) { // Drive connected?
				Serial.printf("drive %s not connected.\n",drv); // No drive.
				return; // Nope, give up and return.
			}
		}
	}
	if(drv[0] == ':') { // ':' must be prceeded by a drive number.
		Serial.printf("Failed: Bad Drive Spec %s\n",drv); // Bad drive spec given.
		return; // Go home.
	}
	// Get and show volume label and serial number.
	getDriveLabels(drive);
	Serial.printf("Volume Label: %s\n", driveLabel[drive]);
	Serial.printf("Volume Serial Number: %lu\n\n",serialNum[drive]);

	// First Check if using wilcards. '?*.?*'
	wildcards = get_wild_cards(path,pattern);
	if(wildcards) { // Find and display all filenames that match.
		fr = f_findfirst(&dir, &fno, path, pattern);  // Start to search for files with the started by "dsc".
		while (fr == FR_OK && fno.fname[0]) {  // Repeat while an item is found.
           fn = fno.fname;
            if ((fno.fattrib & AM_DIR) == 0) { // if this is a file...
                Serial.printf("%s ", fn);
				i = 20-strlen(fn);
				while (i-- > 0)  Serial.printf(" ");
				Serial.printf("%10.ld    ", (uint32_t)fno.fsize); // Display file size.
				Serial.printf("%c%c%c%c%c  ", // Display attributes.
              (fno.fattrib & AM_DIR) ? 'D' : ' ',
               (fno.fattrib & AM_RDO) ? 'R' : ' ',
               (fno.fattrib & AM_HID) ? 'H' : ' ',
               (fno.fattrib & AM_SYS) ? 'S' : ' ',
               (fno.fattrib & AM_ARC) ? 'A' : ' ');
			  struct tm tx = decode_fattime (fno.fdate, fno.ftime);
			  Serial.printf("%4d-%02d-%02d %02d:%02d:%02d\n",
							tx.tm_year,tx.tm_mon,tx.tm_mday, 
							tx.tm_hour,tx.tm_min,tx.tm_sec);
			}
			fr = f_findnext(&dir, &fno); // Search for next item.
		}
		f_closedir(&dir);
	}
	else // Not using wildards.
	{
		scan_files(path); // Do a complete directory/file listing.
	}
}

/*------------------------------------------------------------/
/ Delete a sub-directory even if it contains any file
/-------------------------------------------------------------/
/ The delete_node() function is for R0.12+.
/ It works regardless of FF_FS_RPATH.
*/
FRESULT delete_node (
    TCHAR* path,    /* Path name buffer with the sub-directory to delete */
    UINT sz_buff,   /* Size of path name buffer (items) */
    FILINFO* fno    /* Name read buffer */
) {
    UINT i, j;
    FRESULT fr;
    DIR dir;

    fr = f_opendir(&dir, path); /* Open the directory */
    if (fr != FR_OK) return fr;

    for (i = 0; path[i]; i++) ; /* Get current path length */
    path[i++] = _T('/');

    for (;;) {
        fr = f_readdir(&dir, fno);  /* Get a directory item */
        if (fr != FR_OK || !fno->fname[0]) break;   /* End of directory? */
        j = 0;
        do {    /* Make a path name */
            if (i + j >= sz_buff) { /* Buffer over flow? */
                fr = (FRESULT)100; break;    /* Fails with 100 when buffer overflow */
            }
            path[i + j] = fno->fname[j];
        } while (fno->fname[j++]);
        if (fno->fattrib & AM_DIR) {    /* Item is a directory */
            fr = delete_node(path, sz_buff, fno);
        } else {                        /* Item is a file */
            fr = f_unlink(path);
        }
        if (fr != FR_OK) break;
    }

    path[--i] = 0;  /* Restore the path name */
    f_closedir(&dir);

    if (fr == FR_OK) fr = f_unlink(path);  /* Delete the empty directory */
    return fr;
}

// A more standard file seek function.
int fileSeek(FIL *fp, uint32_t position, int whence)
{
	FRESULT	result;
	switch (whence) {
		case SEEK_SET:
			result = f_lseek(fp, position);
			if (result != FR_OK)
				return -1;
			return fp->fptr;
			break;
		case SEEK_CUR:
					result = f_lseek(fp,(fp->fptr + position));
			if (result != FR_OK)
				return -1;
			return fp->fptr;
			break;
		case SEEK_END:
			result = f_lseek(fp,(fp->obj.objsize + position));
			if (result != FR_OK)
				return -1;
			return fp->fptr;
			break;
	}
	return -1;
}

// Copy a file from one drive to another. Uses standard path names.
// Set 'stats' the third parameter to true to display a progress bar,
// copy speed and copy time. 
FRESULT fileCopy(const char *src, const char *dest, bool stats) {
    FIL fsrc, fdst;      // File objects
    FRESULT fr;          // FatFs function common result code
    UINT br, bw;         // File read/write count
	UINT bufferSize;
	bufferSize = 32*1024; // Buffer size can be adjusted as needed.
	uint32_t cntr = 0;
	uint32_t start = 0, finish = 0;
	uint32_t buffer[bufferSize];   /* File copy buffer */
	uint32_t flSize = 0;

    /* Open source file */
    fr = f_open(&fsrc, src, FA_READ);
    if (fr) return fr;
	flSize = f_size(&fsrc); // Size of source file.
    /* Create destination file on the drive 0 */
    fr = f_open(&fdst, dest, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr)	return fr;

    /* Copy source to destination */
	start = micros();
    for (;;) {
		if(stats) { // If true, display progress bar.
			cntr++;
			if(!(cntr%10)) Serial.printf("*");
			if(!(cntr%640)) Serial.printf("\n");
		}
        fr = f_read(&fsrc, buffer, bufferSize, &br);  /* Read a chunk of source file */
        if (fr || br == 0) break; /* error or eof */
        fr = f_write(&fdst, buffer, br, &bw);            /* Write it to the destination file */
        if (fr || bw < br) break; /* error or disk full */
    }
	f_sync(&fdst); // Flush write buffer.
    /* Close open files */
    f_close(&fsrc);
    f_close(&fdst);
	finish = (micros() - start);
    float MegaBytes = (flSize*1.0f)/(1.0f*finish);
	if(stats) // If true, display time stats.
		Serial.printf("\nCopied %u bytes in %f seconds. Speed: %f MB/s\n",flSize,(1.0*finish)/1000000.0,MegaBytes);
	return fr; // Return any errors or success.
}

// Check if file exists.
boolean f_exists(char *fname) {
	m_res = f_stat(fname, &fno);
	if(m_res == FR_NO_FILE)
		return false;
	else
		return true;
}
	
// Check if a directory entry is directory. 
boolean f_isDir(FILINFO fileinfo) {
	if(fileinfo.fattrib & AM_DIR)
		return true;
	else
		return false;
}

// Return the free space on the selected drive index.
uint64_t getDfree(uint8_t drive) {
	switch(drive)
	{
		case 3: // USB1
			f_getfree(driveName[drive], &fre_clust, &fs3);
			tot_sect = (fs3->n_fatent - 2) * fs3->csize;
			fre_sect = fre_clust * fs3->csize;
			break;
		case 2: // USB0
			f_getfree(driveName[drive], &fre_clust, &fs2);
			tot_sect = (fs2->n_fatent - 2) * fs2->csize;
			fre_sect = fre_clust * fs2->csize;
			break;
		case 1: // SDHC
			f_getfree(driveName[drive], &fre_clust, &fs1);
			tot_sect = (fs1->n_fatent - 2) * fs1->csize;
			fre_sect = fre_clust * fs1->csize;
			break;
		case 0: // SPI
			tot_sect = (fs0->n_fatent - 2) * fs0->csize;
			fre_sect = fre_clust * fs0->csize;
			break;
		default:
			break;
	}
	return (uint64_t)(fre_sect / 2);
}
