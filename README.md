# MSC2
Mass Storage Controller for Teensy T3.6 and T4.0

This is a USB Mass Storage driver based on PJRC's USBHost_t36 library.

This version is almost a complete rewrite of MSC and is still WIP.

- You will need Arduino version 1.89 up to version 1.8.12.
- Teensyduino version 1.47 Beta 2 or better from PJRC.
- You will need my forked version of WXMZ's uSDFS. Modifications
  were made for handling multiple Mass Storage devices.
- There are now some example programs in the examples folder.

Teensyduino found here: https://www.pjrc.com/teensy/td_download.html

uSDFS found here: https://github.com/wwatson4506/MSC/tree/MSC2

PJRC forum thread for MSC: https://forum.pjrc.com/threads/55821-USBHost_t36-USB-Mass-Storage-Driver-Experiments

To use my example sketches there is a define that needs to be set in the MassStorage.h file.
Define  USE_EXTENAL_INIT to Initialize MSC Externally.
If defined, MSC needs the following setup in the ino sketch:
- USBHost myusb;
- USBHub hub1(myusb);
- USBHub hub2(myusb);
- USBHub hub3(myusb);
- USBHub hub4(myusb);
- myusb.begin();
- mscInit();
