# MSC-non-blocking
Mass Storage Controller for Teensy 3.6 and Teensy 4.x

This the latest and possibly last version of MSC that I am submitting. It should function the same way as the original MSC did but
also has the ability to do USB disk transfers in a non-blocking mode. Using the non-blocking mode is probably only usefull with a
an RTOS or modified version of FatFS or SDFat. I tried modifying FatFS but that failed due to the fact that it had to wait for the
transfers to complete before processing. There will have to be a mechanisim to signal completion of data to or from the USB drive.
I guess file locking and semaphores.
Some improvements have been made. It still retains the blocking mode of MSC. It remains compatible with my updated version of uSDFS.
My newer version of uSDFS will have to be used. 

To really see what is happening in the background with this non-blocking version I have set up two pins on the T4.1 to run a red
and green LED. Red for writes and green for reads. Using MSC-non-blocking.ino will give you several options to test with.
Pin defines are in the MassStorage.h file so change to suit. I do not have the blocking version of this driver using LED's.

*********** BIG WARNING ************: Because MSC-non-blocking.ino is using direct sector reads and writes any OS formatting to the
USB mass storage device will be overwritten with a write to the device requiring it to be refomatted. The start sector address of the reads and writes are set high but still can destroy formatting. Use on USB drives with no volatile data!!!!

Key things you can play with to get various results.

1) MassStorage.h in this section:

Define queue sizes

#define MAX_TRANSFERS  1024.

read/write queue sizes (must be powers of 2!!)
As the queue sizes get Smaller and depending on the size of the read or write buffer the bigger the chance of MSC going
into  blocking. If the queues are full it will block until space is avilable in the queues. This can be a memory eater:)
  
Setup debugging LED pin defs.
Used mainly to see  the activity of non-blocking reads and writes.

"#define WRITE_PIN  33" Pin number of drive read activity led (RED LED).
"#define READ_PIN   34" Pin number of drive read activity led (GREEN LED).
  

UnComment the following define for verbose debug output.
//#define DBGprint 1

UnComment to watch read/write queue activity.
//#define DBGqueue 1

Define  USE_EXTERNAL_INIT to Initialize MSC Externally when not using uSDFS.
If defined, MSC needs the following in the setup section of the ino sketch:
USBHost myusb;
myusb.begin();
mscHost.mscHostInit();

//#define USE_EXTERNAL_INIT

2) MSC-non-blocking.ino:

#define FILLCHAR 0xff
Change this to write a different value to sectors.
Also used for verifying multi sector writes.
   
Uncomment "#define BLOCKING" to wait for the transfer to complete (blocking) and the time it took. Reads and writes will block until transfer is complete. Will return to loop() when complete.

//#define BLOCKING
  
This will read and write sectors in blocking mode and display read and write times. Setting MSC to non-blocking mode will show how long it takes to queue reads or writes.
  
MSC in non-blocking mode is using tonton81's CircularBuffer and luni64's attachYieldFunc libraries. As such, using attachYieldFunc requires any while statements to include the yield() function to keep it from blocking.
I have tried to comment the source code heavily to explain what is going on.
  
  THIS IS JUST PROOf OF CONCEPT:)
  
  
