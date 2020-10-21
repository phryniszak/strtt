# strtt

Segger RTT client using ST-link debugger.

Options:

-v debugLevel
where debugLevel from -3 to 4 when -3 is equal to silent output

-ram ramAmmount
where ramAmmount is range where program is looking for RTT. 

Folder windows_bin_64 includes compiled windows 64 bit executable.

Program is built using static libraries from OpenOCD

As for 14/01/2020 OpenOCD has a bug: https://sourceforge.net/p/openocd/tickets/259/
A quick and dirty workaround seams to change in src/jtag/drivers/stlink_usb.c the macro STLINKV3_MAX_RW8 from current 512 to 255.


21/10/2020 
Added possibilities to connect from Segger SystemView using "IP Recorder" at port 19111.
Windows binaries are not updated for now.