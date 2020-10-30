# MSC-non-blocking
Mass Storage Controller for Teensy 3.6 and Teensy 4.x
This the latest and possibly last version of MSC that I am submitting. It should function the same way as the original MSC did but
also has the ability to do USB disk transfers in a non-blocking mode. Using the non-blocking mode is probably only usefull with a
an RTOS or modified version of FatFS or SDFat. I tried modifying FatFS but that failed due to the fact that it had to wait for the
transfers to complete before processing. There will have to be a mechanisim to signal completion of data to or from the USB drive.
I guess file locking and semaphores.
Some improvements have been made. It still retains the blocking mode of MSC. It remains compatible with my updated version of uSDFS.
The newer version of uSDFS will have to be used. This will be the next thing I have to update.

To really see what is happening in the background with this non-blocking version I have set up two pins on the T4.1 to run a red
and green LED. Red for writes and green for reads. Using MSC-non-blocking.ino will give you several options to test with.
Pin defines are in the MassStorage.h file so change to suit. I do not have the blocking version of this driver using LED's.

*********** BIG WARNING ************: Because MSC-non-blocking.ino is using direct sector reads and writes any OS formatting to the
USB mass storage device will be overwritten with a write to the device requiring it to be refomatted. The start sector address of the reads and writes are set high but still can destroy formatting. Use on USB drives with no volatile data!!!!

Key things you can play with to get various results.
1) MassStorage.h in this section:
  // Define queue sizes
  #define MAX_TRANSFERS		1024	// read/write queue sizes (must be powers of 2!!)

  // Setup debugging LED pin defs.
  // Used mainly to see  the activity of non-blocking reads and writes.
  #define WRITE_PIN			33		// Pin number of drive read activity led (RED LED).
  #define READ_PIN			34		// Pin number of drive read activity led (GREEN LED).
2) :
  

