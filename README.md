# strtt
Segger RTT client using ST-link debugger.

Options:

**-v** debugLevel where debugLevel from -3 to 4 when -3 is equal to silent output

**-ramstart** Ram Start address where the program is looking for RTT( **hex**,dec model supported).

**-ramsize** ramAmmount where ramAmmount is the range where the program is looking for RTT.

**-tcp** use tcp connection to st-link gdb server (<https://www.st.com/en/development-tools/st-link-server.html>)

**-ap** select the AP number to use (default 0), some devices have multiple APs, for example STM32H5 and STM32H7 need set AP to 1.

**-serial** ST-LINK serial number to connect to. Useful when multiple ST-LINK probes are connected at the same time.

# Executable

Can be found [here](https://github.com/phryniszak/strtt/releases).


# Using with STM32CubeIDE

Thanks to the **-tcp** option it is possible to communicate with a target using the RTT channel while debugging in STM32CubeIDE.
In debug options "Shared ST-LINK" must be checked.

<a href="http://www.youtube.com/watch?feature=player_embedded&v=MP6PS8l4fyE" target="_blank"><img src="http://img.youtube.com/vi/MP6PS8l4fyE/0.jpg"
alt="RTT and STM32CubeIDE" width="480" height="360" border="10" /></a>

# Using with VSCode and cortex-debug

To share ST-LINK_gdbserver add **-t** option to serverArgs and start strtt with **-tcp**.

# Known Limitations

> **Cortex-M7 devices:** Due to unresolved cache handling issues inherited from the OpenOCD driver, the program may fail to work correctly with Cortex-M7 based devices (e.g. STM32F7, STM32H7).

# Internals

Program is using a refactored driver from the openocd project.

# Usage example

```bash

# in most cases default settings are OK

./strtt

# If you want to connect to RTT while debugging in your IDE share stlink and use tcp

./strtt -ramstart 0x30020000 -tcp

# If multiple ST-LINK probes are connected, select one by serial number

./strtt -serial 066EFF303435554157105544

```

# SYSTEMVIEW

One experimental option is to use this program with Segger SystemView using tcp connection. To use it, the program must be built with the SYSVIEW option.

`cmake -DSYSVIEW=1 ..`

# RTT in browser
You can also try to use web browser version:

https://phryniszak.github.io/jstlink/sample_rtt/

https://github.com/phryniszak/jstlink
