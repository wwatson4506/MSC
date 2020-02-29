# MSC2
Mass Storage Controller for Teensy T3.6 and T4.0

This is a USB Mass Storage driver based on PJRC's USBHost_t36 library.

This version is almost a complete rewrite of MSC (MSC2 Branch) and is still WIP.

- You will need Arduino version 1.89 up to version 1.8.12.
- Teensyduino version 1.47 Beta 2 or better from PJRC.
- You will need my forked version of WXMZ's uSDFS. Modifications
  were made for handling multiple Mass Storage devices.
- There are now some example programs in the examples folder of MSC.

Teensyduino found here: https://www.pjrc.com/teensy/td_download.html

uSDFS found here: https://github.com/wwatson4506/MSC/tree/MSC2

If you are cloning MSC instead of downloading the zip file you will need to switch to the MSC2 branch using:
- git checkout MSC2
- git pull origin MSC2

PJRC forum thread for MSC: https://forum.pjrc.com/threads/55821-USBHost_t36-USB-Mass-Storage-Driver-Experiments

To use my MSC2 example sketches there is a define that needs to be set in the MassStorage.h file.

Uncomment 'Define USE_EXTENAL_INIT' to Initialize MSC Externally.

If defined, MSC uses the following in the *.ino sketches:
- USBHost myusb;
- USBHub hub1(myusb);
- USBHub hub2(myusb);
- USBHub hub3(myusb);
- USBHub hub4(myusb);
- myusb.begin();
- mscInit();

You will need to comment out '//define USE_EXTENAL_INIT' when using uSDFS examples.

The example sketch 'CopyFiles.ino' uses a file named 'A_00001.dat' which is created by the 'logger_rawWrite.ino' sketch in uSDFS. This file was used for testing the copy function between different devices.

The example sketch 'HotPlug.ino' tests MSC2's hot plugging capabilities of multiple USB drives. It also demonstrates use of available drive information for attached drive.

The example sketch 'test_RawWrite.ino' tests the read and write speeds of USB drives. Change the define in line 57 to '#define WR_RD 0' for read speeds and '#define WR_RD 1' for write speeds.

Again this work in progress and is still in development.


