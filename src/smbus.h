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

#include <linux/types.h>

#ifdef SBS_ENABLE_I2C
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#endif

#include "src/structs.h"

// linux/drivers/acpi/sbshc.h
enum acpi_sbs_device_addr {
	ACPI_SBS_CHARGER = 0x9,
	ACPI_SBS_MANAGER = 0xa,
	ACPI_SBS_BATTERY = 0xb,
};


void check_device_capabilities(int fd)
{
	#ifdef SBS_ENABLE_I2C
	unsigned long funcs;
	if (ioctl(fd, I2C_FUNCS, &funcs))
	{
		printf("check_device_capabilities() : failed to get supported functions\n");
		close(fd);
		exit(1);
	}

	// linux/i2c.h
	const unsigned long flags[] = {I2C_FUNC_I2C, I2C_FUNC_10BIT_ADDR, I2C_FUNC_PROTOCOL_MANGLING, I2C_FUNC_SMBUS_PEC, I2C_FUNC_NOSTART, I2C_FUNC_SLAVE,
				I2C_FUNC_SMBUS_BLOCK_PROC_CALL, I2C_FUNC_SMBUS_QUICK, I2C_FUNC_SMBUS_BYTE, I2C_FUNC_SMBUS_BYTE_DATA,
				I2C_FUNC_SMBUS_WORD_DATA, I2C_FUNC_SMBUS_PROC_CALL, I2C_FUNC_SMBUS_BLOCK_DATA, I2C_FUNC_SMBUS_I2C_BLOCK,
				I2C_FUNC_SMBUS_HOST_NOTIFY};

	const char* c_flags[] = {"I2C_FUNC_I2C", "I2C_FUNC_10BIT_ADDR", "I2C_FUNC_PROTOCOL_MANGLING", "I2C_FUNC_SMBUS_PEC", "I2C_FUNC_NOSTART", "I2C_FUNC_SLAVE",
				"I2C_FUNC_SMBUS_BLOCK_PROC_CALL", "I2C_FUNC_SMBUS_QUICK", "I2C_FUNC_SMBUS_BYTE", "I2C_FUNC_SMBUS_BYTE_DATA",
				"I2C_FUNC_SMBUS_WORD_DATA", "I2C_FUNC_SMBUS_PROC_CALL", "I2C_FUNC_SMBUS_BLOCK_DATA", "I2C_FUNC_SMBUS_I2C_BLOCK",
				"I2C_FUNC_SMBUS_HOST_NOTIFY"};

	const unsigned long flags_required[] = {/*I2C_FUNC_I2C, I2C_FUNC_SLAVE,*/
				I2C_FUNC_SMBUS_BYTE, I2C_FUNC_SMBUS_BYTE_DATA,
				I2C_FUNC_SMBUS_WORD_DATA, I2C_FUNC_SMBUS_BLOCK_DATA};

	const char* c_flags_required[] = {/*"I2C_FUNC_I2C", "I2C_FUNC_SLAVE",*/
				"I2C_FUNC_SMBUS_BYTE", "I2C_FUNC_SMBUS_BYTE_DATA",
				"I2C_FUNC_SMBUS_WORD_DATA", "I2C_FUNC_SMBUS_BLOCK_DATA"};
	
	static const int flags_n = sizeof(flags) / sizeof(const unsigned long), flags_required_n = sizeof(flags_required) / sizeof(const unsigned long);

#ifdef ENABLE_DEBUG
	printf("device capabilities : ");
	for (int i = 0; i < flags_n; i++)
	{
		if ((funcs & flags[i]) > 0)
			printf("%s ", c_flags[i]);
	}
	printf("\n");
#endif

	for (int i = 0; i < flags_required_n; i++)
	{
		if ((funcs & flags_required[i]) == 0)
		{
			printf("check_device_capabilities() : device does not support %s\n", c_flags_required[i]);
			close(fd);
			exit(1);
		}
	}
	#endif
}


int device_open(const struct args* c)
{
	//char fname[32];
	//snprintf(fname, 31, "/dev/i2c-%d", adapter);
	
	// Read from the i2c bus
	if (c->i2c)
	{
		#ifdef SBS_ENABLE_I2C
		int fd = open(c->file, O_RDWR);
		if (fd < 0)
		{
			printf("device_open() : could not open device\n");
			exit(1);
		}

		check_device_capabilities(fd);
		if (ioctl(fd, I2C_SLAVE, ACPI_SBS_BATTERY) < 0)
		{
			printf("device_open() : could not change device address\n");
			close(fd);
			exit(1);
		}

		return fd;
		#else
		return -1;
		#endif
	}
	else
	{
		// Access EC
		return -1;
	}
}


void quit(int fd, int status)
{
	close(fd);
	exit(status);
}

// SBS I/O functions
// =================


void sbs_log_error(__s32 res)
{
	int err = (int)(-res);
	printf("SMBus I/O error : %s\n", strerror(err));
}


__s32 sbs_read_word(int fd, __u8 command)
{
	#ifdef SBS_ENABLE_I2C
	__s32 res = i2c_smbus_read_word_data(fd, command);
	if (res < 0)
	{
		printf("sbs_read_word() : command execution failed\n");
		sbs_log_error(res);
	}
	return res;
	#else
	return -1;
	#endif
}


__s32 sbs_read_block(int fd, __u8 command, __u8* result)
{
	#ifdef SBS_ENABLE_I2C
	__s32 res = i2c_smbus_read_block_data(fd, command, result);
	if (res < 0)
	{
		int err = (int)(-res);
		if (err == EPROTO)
		{
			printf("sbs_read_block() : slave returned non-standard block size\n");
			return 0;
		}
		printf("sbs_read_block() : command execution failed\n");
		sbs_log_error(res);
	}
	return res;
	#else
	return -1;
	#endif
}


int sbs_exec_block_command(__u8 command, const __u8* data, __u8* result, int length, int fd)
{
	#ifdef SBS_ENABLE_I2C
	__s32 res = i2c_smbus_write_block_data(fd, command, length, data);
	if (res < 0)
	{
		printf("sbs_exec_block_command() : command execution failed\n");
		sbs_log_error(res);
		return 1;
	}

	res = i2c_smbus_read_block_data(fd, command, result);
	if (res < 0)
	{
		printf("sbs_exec_block_command() : could not read command result\n");
		sbs_log_error(res);
		return 1;
	}

	return 0;
	#else
	return -1;
	#endif
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
