# strtt
Segger RTT client using ST-link debugger.
Options:
-v debugLevelwhere debugLevel from -3 to 4 when -3 is equal to silent output
-ram ramAmmountwhere ramAmmount is the range where the program is looking for RTT. 
-tcp if connecting to st-link gdb server https://www.st.com/en/development-tools/st-link-server.html

# Windows
Folder windows_bin_64 includes compiled windows 64 bit executable. If the program returns immediately try to run it with -v 4 option. Return value (-4) indicates missing stlink drivers avalilable as STSW-LINK009 from st.com.

# Internals
Program is using a refactored driver from the openocd project.

# RTT in browser
You can also try to use webbrowser version here:

https://phryniszak.github.io/jstlink/sample_rtt/

https://github.com/phryniszak/jstlink
