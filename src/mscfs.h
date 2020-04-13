#ifndef MSCFS_H__
#define MSCFS_H__

#include "Arduino.h"
#include "uSDFS.h"
#include "ff.h"

#ifdef __cplusplus
extern "C" {
#endif
void MSCFS_init(void);
int showUSBDriveInfo(uint8_t drive);
void selectDevice(uint8_t device);
int getDriveIndex(const char * drive);
const char *getDriveName(uint8_t drive);
uint8_t getCurrentDrive(void);
uint8_t mapUSBDrive(uint8_t drive);
int changeDrive(uint8_t drive);
void getDriveLabels(uint8_t drives);
bool driveAvailable(uint8_t drive);
FATFS *assignVolume(uint8_t drive);
FRESULT mountDrive(FATFS *fs, const char *driveName, uint8_t deviceNumber);
FRESULT uNmountDrive(const char *driveName);
void lsdir(char *directory);
FRESULT	scan_files (char  *path);
int get_wild_cards(char *specs, char *pattern);
uint64_t getDfree(uint8_t drive);
boolean f_exists(char *fname);
boolean f_isDir(FILINFO fileinfo);
FRESULT fileCopy(const char *src, const char *dest, bool stats);
int fileSeek(FIL *fp, uint32_t position, int whence);
FRESULT delete_node (
    TCHAR* path,    /* Path name buffer with the sub-directory to delete */
    UINT sz_buff,   /* Size of path name buffer (items) */
    FILINFO* fno    /* Name read buffer */
);

#ifdef __cplusplus
}
#endif

void MSCFS_init(void);
int showUSBDriveInfo(uint8_t drive);
void selectDevice(uint8_t device);
int getDriveIndex(const char * drive);
const char *getDriveName(uint8_t drive);
uint8_t mapUSBDrive(uint8_t drive);
uint8_t getCurrentDrive(void);
int changeDrive(uint8_t drive);
void getDriveLabels(uint8_t drives);
bool driveAvailable(uint8_t drive);
FATFS *assignVolume(uint8_t drive);
FRESULT mountDrive(FATFS *fs, const char *driveName, uint8_t deviceNumber);
FRESULT uNmountDrive(const char *driveName);
void lsdir(char *directory);
FRESULT	scan_files (char  *path);
int get_wild_cards(char *specs, char *pattern);
uint64_t getDfree(uint8_t drive);
boolean f_exists(char *fname);
boolean f_isDir(FILINFO fileinfo);
FRESULT fileCopy(const char *src, const char *dest, bool stats);
int fileSeek(FIL *fp, uint32_t position, int whence);
FRESULT delete_node (
    TCHAR* path,    /* Path name buffer with the sub-directory to delete */
    UINT sz_buff,   /* Size of path name buffer (items) */
    FILINFO* fno    /* Name read buffer */
);

#endif
