# MSC3
Mass Storage Controller for Teensy T3.6 and T4.0

This is a USB Mass Storage driver based on PJRC's USBHost_t36 library.

This version is almost a complete rewrite of MSC (MSC2 Branch) and (MSC3 branch)
is still WIP.

- You will need Arduino version 1.89 up to version 1.8.12.
- Teensyduino version 1.47 Beta 2 or better from PJRC.
- You will need my forked version of WXMZ's uSDFS. Modifications
  were made for handling multiple Mass Storage devices.
- There are now some of my example programs in the examples folder of MSC3.

Teensyduino found here: https://www.pjrc.com/teensy/td_download.html.      
uSDFS found here: https://github.com/wwatson4506/uSDFS

If you are cloning MSC instead of downloading the zip file you will need to switch to the MSC3 branch using:
- git checkout MSC3
- git pull origin MSC3

PJRC forum thread for MSC: https://forum.pjrc.com/threads/55821-USBHost_t36-USB-Mass-Storage-Driver-Experiments

To use my MSC3 example sketches there is a define that needs to be set in the MassStorage.h file.

Uncomment 'Define USE_EXTENAL_INIT' to Initialize MSC3 Externally.

If defined, MSC3 uses the following in the *.ino sketches:
- USBHost myusb;
- USBHub hub1(myusb);
- USBHub hub2(myusb);
- USBHub hub3(myusb);
- USBHub hub4(myusb);
- myusb.begin(); (In setup())
- MSCFS_init();   (In setup() now calls mscInit())

You will need to comment out 'define USE_EXTENAL_INIT' when using uSDFS examples.

A new file has been added, 'mscfs.cpp'. It supplys some higher level functions needed by the new example sketches.
This version of MSC trys to initialize and auto mount the USB device when the 'driveAvailable()' function is called.

The 'uNmountDrive()' function should be called before unplugging a USB drive to avoid data loss and/or file system corruption.

The example sketch 'CopyFiles.ino' uses a file named '32MEGfile.dat'. You can find this file in the extras folder.

It is used for testing the copy function between different devices. Just copy it to all of your USB mass storage devices and/or SDHC cards or use the CopyFiles sketch to

copy it from one device to another.

The example sketch 'HotPlug.ino' tests MSC3's hot plugging capabilities of multiple USB drives. It also demonstrates use of available drive information for attached drive.

The example sketch 'test_RawWrite.ino' is gone.

Again this work in progress and is still in development.

New Sketches:

The example sketch 'ReadWrite.ino' opens a file 'test.dat' for writing. It writes 32768000 bytes to the file and closes it. It reopens the file for reading and reads it back.

It shows the speed of the read and writes.

The example sketch 'ListDirectory.ino' is an example of listing the directory in several ways including using wildcards and patterns.

Again this work in progress and is still in development.

TODO: Finish buffering MSC3 reads and writes. MSC3 is initially setup to do it. WIP.
