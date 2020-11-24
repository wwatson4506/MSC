# MSC
Mass Storage Controller for Teensy T3.6 and T4.x

This is a USB Mass Storage driver based on PJRC's USBHost_t36 library.

***********************************************************************************************
THIS VERSION IS NOW OUTDATED!!

The latest version is MSC-non-blocking. It is the most recent version. Unfortunatley it
is not completely backwards compatible with the other three version. Itis compatible with
the latest TeensyDuino 1.54 and Arduino 1.8.13.
You will also need my version of WMXZ's uSDFS if using uSDFS located here:
https://github.com/wwatson4506/uSDFS/tree/uSDFS-non-blocking.

My modfied fork of the SdFat-beta and SD libraries are useable with MSC-non-blocking located here
https://github.com/wwatson4506/SdFat-beta.
and here
https://github.com/wwatson4506/SD.

These two libraries are forked from Paul Stoffregen's github site. (WIP).
Sdfat-beta must be renamed to Sdfat and placed in "arduino-1.8.13/hardware/teensy/avr/libraries"
folder along with SD. Make sure to backup both of these libraries!

There is a sketch in the examples folder of MSC-non-blocking that demonstrates async access to
a MSC USB drive. ***READ THE WARNING***!! The async write function will overwrite any disk data or
formatting!!
Requires device to be reformatted.
***************************************************************************************************
It is still very much WIP.
You will need Arduino version 1.89 and Teensyduino version 1.47 Beta 2 from PJRC.
Currently there is no example programs within MSC itself. For examples of the use of MSC
you need to use example programs found in WMXZ's uSDFS library which implements MSC.

Teensyduino 1.47Beta2 found here in post #2:
https://forum.pjrc.com/threads/54711-Teensy-4-0-First-Beta-Test

WMXZ uSDFS found here: https://github.com/WMXZ-EU/uSDFS

PJRC forum thread for MSC: https://forum.pjrc.com/threads/55821-USBHost_t36-USB-Mass-Storage-Driver-Experiments

Updated 07/28/19: Added more complete error processing using sense codes.
