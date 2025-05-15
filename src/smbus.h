#ifndef SBS_SMBUS_H
#define SBS_SMBUS_H
// ======================
// Common SMBus functions
// ======================


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/i2c-dev.h>
#include <i2c/smbus.h>


// linux/drivers/acpi/sbshc.h
enum acpi_sbs_device_addr {
	ACPI_SBS_CHARGER = 0x9,
	ACPI_SBS_MANAGER = 0xa,
	ACPI_SBS_BATTERY = 0xb,
};


int device_open(int adapter)
{
	char fname[32];
	snprintf(fname, 31, "/dev/i2c-%d", adapter);
	int fd = open(fname, O_RDWR);
	if (fd < 0)
	{
		printf("device_open() : could not open device\n");
		exit(1);
	}

	if (ioctl(fd, I2C_SLAVE, ACPI_SBS_BATTERY) < 0)
	{
		printf("device_open() : could not change device address\n");
		close(fd);
		exit(1);
	}

	return fd;
}


void quit(int fd, int status)
{
	close(fd);
	exit(status);
}

// Conversion functions
// ====================


// Convert SMBus data in hex (little endian) to unsinged 32-bit integer
uint32_t smbus_block_LE_to_ui32(const unsigned char* data, int length)
{
	uint32_t res = 0;
	for (int i = length - 1; i >= 0; i--)
	{
		res *= 0x100; // Move the result
		res += data[i];
	}
	return res;
}


// Convert SMBus data in hex (big endian) to unsinged 32-bit integer
uint32_t smbus_block_BE_to_ui32(const unsigned char* data, int length)
{
	uint32_t res = 0;
	for (int i = 0; i < length; i++)
	{
		res *= 0x100; // Move the result
		res += data[i];
	}
	return res;
}
#endif
