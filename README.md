# strtt
Segger RTT client using ST-link debugger.
Options:

**-v** debugLevel where debugLevel from -3 to 4 when -3 is equal to silent output

**-ram** ramAmmount where ramAmmount is the range where the program is looking for RTT. 

**-tcp** if connecting to st-link gdb server (<https://www.st.com/en/development-tools/st-link-server.html>)

# Windows
Folder windows_bin_64 includes windows 64 bit executable. If the program returns immediately try to run it with -v 4 option. Return value (-4) indicates missing stlink drivers available as STSW-LINK009 from st.com.

**strtt_sysview_tcp.exe** is a version that let you connect SystemView using TCP.

# Using with STM32CubeIDE

Thanks to the **-tcp** option it is possible to communicate with target using RTT channel while debugging in STM32CubeIDE.
In debug options "Shared ST-LINK" must be checked.

<a href="http://www.youtube.com/watch?feature=player_embedded&v=MP6PS8l4fyE" target="_blank"><img src="http://img.youtube.com/vi/MP6PS8l4fyE/0.jpg" 
alt="RTT and STM32CubeIDE" width="480" height="360" border="10" /></a>

# Internals
Program is using a refactored driver from the openocd project.

# RTT in browser
You can also try to use web browser version:

https://phryniszak.github.io/jstlink/sample_rtt/

https://github.com/phryniszak/jstlink
