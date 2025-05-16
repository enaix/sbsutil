#ifndef SBS_SMBUS_H
#define SBS_SMBUS_H
// ======================
// Common SMBus functions
// ======================


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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

// SBS I/O functions
// =================


int sbs_exec_block_command(__u8 command, const __u8* data, __u8* result, int length, int fd)
{
	__s32 res = i2c_smbus_write_block_data(fd, command, length, data);
	if (res < 0)
	{
		printf("sbs_exec_block_command() : command execution failed\n");
		return 1;
	}

	res = i2c_smbus_read_block_data(fd, command, result);
	if (res < 0)
	{
		printf("sbs_exec_block_command() : could not read command result\n");
		return 1;
	}

	return 0;
}

int sbs_block_check_mac(const __u8* data, const __u8* result, int length)
{
	if (memcmp(data, result, length) != 0)
	{
		printf("sbs_block_check_mac() : bad MAC command : expected");
		for (int i = 0; i < length; i++)
			printf(" %.2x", data[i]);
		printf(", got");
		for (int i = 0; i < length; i++)
			printf(" %.2x", result[i]);
		printf("\n");
		return 1;
	}
	return 0;
}


// Conversion functions
// ====================


void smbus_print_block(const unsigned char data[32])
{
	printf("[");
	for (int i = 0; i < 31; i++)
		printf("%.2x ", data[i]);
	printf("%.2x]\n", data[31]);
}


// Convert SMBus data in hex (little endian) to unsigned char*
void smbus_block_LE_to_u8s(const unsigned char* data, int length, unsigned char* dest)
{
	for (int i = length - 1; i >= 0; i--)
	{
		dest[i] = data[i];
	}
}

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
