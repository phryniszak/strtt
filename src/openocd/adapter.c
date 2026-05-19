/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2005 by Dominic Rath <Dominic.Rath@gmx.de>
 * Copyright (C) 2007-2010 Øyvind Harboe <oyvind.harboe@zylin.com>
 * Copyright (C) 2009 SoftPLC Corporation, http://softplc.com, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2009 Zachary T Welch <zw@superlucidity.net>
 * Copyright (C) 2018 Pengutronix, Oleksij Rempel <kernel@pengutronix.de>
 */

#include "adapter.h"
#include "helper_types.h"
#include "helper_replacements.h"
#include "log.h"

enum adapter_clk_mode
{
	CLOCK_MODE_UNSELECTED = 0,
	CLOCK_MODE_KHZ,
	CLOCK_MODE_RCLK
};

/**
 * Adapter configuration
 */
static struct
{
	bool adapter_initialized;
	char *usb_location;
	char *serial;
	enum adapter_clk_mode clock_mode;
	int speed_khz;
	int rclk_fallback_speed_khz;
} adapter_config;

/*
 * 1 char: bus
 * 2 * 7 chars: max 7 ports
 * 1 char: test for overflow
 * ------
 * 16 chars
 */
#define USB_MAX_LOCATION_LENGTH 16

/*
@anchor{adapter_usb_location}
@deffn {Config Command} {adapter usb location} [<bus>-<port>[.<port>]...]
Displays or specifies the physical USB port of the adapter to use. The path
roots at @var{bus} and walks down the physical ports, with each
@var{port} option specifying a deeper level in the bus topology, the last
@var{port} denoting where the target adapter is actually plugged.
The USB bus topology can be queried with the command @emph{lsusb -t} or @emph{dmesg}.

This command is only available if your libusb1 is at least version 1.0.16.
@end deffn
*/

static void adapter_usb_set_location(const char *location)
{
	if (strnlen(location, USB_MAX_LOCATION_LENGTH) == USB_MAX_LOCATION_LENGTH)
		LOG_WARNING("usb location string is too long!!");

	free(adapter_config.usb_location);

	adapter_config.usb_location = strndup(location, USB_MAX_LOCATION_LENGTH);
}

const char *adapter_usb_get_location(void)
{
	return adapter_config.usb_location;
}

/*
@deffn {Config Command} {adapter serial} serial_string
Specifies the @var{serial_string} of the adapter to use.
If this command is not specified, serial strings are not checked.
Only the following adapter drivers use the serial string from this command:
aice (aice_usb), arm-jtag-ew, cmsis_dap, ft232r, ftdi, hla (stlink, ti-icdi), jlink, kitprog, opendus,
openjtag, osbdm, presto, rlink, st-link, usb_blaster (ublast2), usbprog, vsllink, xds110.
@end deffn
*/

void adapter_set_required_serial(const char *serial)
{
	free(adapter_config.serial);
	adapter_config.serial = strdup(serial);
}

const char *adapter_get_required_serial(void)
{
	return adapter_config.serial;
}

bool adapter_usb_location_equal(uint8_t dev_bus, uint8_t *port_path, size_t path_len)
{
	size_t path_step, string_length;
	char *ptr, *loc;
	bool equal = false;

	if (!adapter_usb_get_location())
		return equal;

	/* strtok need non const char */
	loc = strndup(adapter_usb_get_location(), USB_MAX_LOCATION_LENGTH);
	string_length = strnlen(loc, USB_MAX_LOCATION_LENGTH);

	ptr = strtok(loc, "-");
	if (!ptr)
	{
		LOG_WARNING("no '-' in usb path\n");
		goto done;
	}

	string_length -= strnlen(ptr, string_length);
	/* check bus mismatch */
	if (atoi(ptr) != dev_bus)
		goto done;

	path_step = 0;
	while (path_step < path_len)
	{
		ptr = strtok(NULL, ".");

		/* no more tokens in path */
		if (!ptr)
			break;

		/* path mismatch at some step */
		if (path_step < path_len && atoi(ptr) != port_path[path_step])
			break;

		path_step++;
		string_length -= strnlen(ptr, string_length) + 1;
	};

	/* walked the full path, all elements match */
	if (path_step == path_len && !string_length)
		equal = true;

done:
	free(loc);
	return equal;
}
