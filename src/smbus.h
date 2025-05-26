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
#include <dirent.h>

#include <linux/types.h>
#include <asm/io.h>

// i2c drivers
#ifdef SBS_ENABLE_I2C
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#endif

// ACPI
#include <acpi/acpi.h>
#include <acpi/acpi_bus.h>

#include "src/structs.h"

// linux/drivers/acpi/sbshc.h
// ==========================

enum acpi_sbs_device_addr {
	ACPI_SBS_CHARGER = 0x9,
	ACPI_SBS_MANAGER = 0xa,
	ACPI_SBS_BATTERY = 0xb,
};

enum acpi_smb_offset {
	ACPI_SMB_PROTOCOL = 0,	/* protocol, PEC */
	ACPI_SMB_STATUS = 1,	/* status */
	ACPI_SMB_ADDRESS = 2,	/* address */
	ACPI_SMB_COMMAND = 3,	/* command */
	ACPI_SMB_DATA = 4,	/* 32 data registers */
	ACPI_SMB_BLOCK_COUNT = 0x24,	/* number of data bytes */
	ACPI_SMB_ALARM_ADDRESS = 0x25,	/* alarm address */
	ACPI_SMB_ALARM_DATA = 0x26,	/* 2 bytes alarm data */
};


// the device may appear with the following ids
char* acpi_sbs_device_hid[] = {"ACPI0001", "ACPI0005", ""}; // "" is an end of array marker


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


int probe_acpi_tables(struct args* c)
{
	
	// https://lwn.net/Articles/367630/
	// linux/tools/power/acpi/tools/acpidump/apdump.c

	struct acpi_table_header* table;
	acpi_physical_address addr;
	acpi_status status;
	int table_status;
	
	for (int instance = 0; instance < AP_MAX_ACPI_FILES, instance++)
	{
		status = acpi_os_get_table_by_name(ACPI_SIG_DSDT, instance, &table, &address);
		if (ACPI_FAILURE(status))
		{
			if (status == AE_LIMIT)
			{
				if (instance == 0)
				{
					printf("probe_acpi_tables() : no DSDT ACPI table found\n");
					return -1;
				}
				// else ok
			}
			else
			{
				printf("probe_acpi_tables() : could not get DSDT ACPI table : %s\n", local_signature, acpi_format_exception(status));
				return -1;
			}

		}
		// table->length includes the header
		// we now can now access ASCII ASL 

		ACPI_FREE(table);
	}
}

int probe_acpi_device(struct args* c)
{
	struct acpi_device* dev = NULL;
	// Iterate over each device which matches the criteria
	// Check which devices are present on the system bus
	/*DIR *dir;
	struct dirent* dir_e;
	d = opendir("/sys/bus/acpi/devices");
	if (!d)
	{
		printf("probe_acpi_tables() : could not access /sys/bus/acpi/devices : %s\n", strerror(errno));
		return -1;
	}

	while ((dir_e = opendir()) != NULL)
	{
		char* fname_hid = dir_e->d_name;
		strcat(fname_hid, "/hid"); // path to hid file

		int fd_hid = open(fname_hid, O_RDONLY);
		if (fd_hid < 0)
		{
			printf("probe_acpi_device() : could not open file %s : %s\n", fname_hid, strerror(errno));
			return -1;
		}

		char hid[32]; // supposed to be 8
		ssize_t res = read(fd_hid, hid, 32);
		if (res < 0 || res == 32)
		{
			if (res == 32)
				printf("probe_acpi_device() : %s has bad hid format\n");
			else
				printf("probe_acpi_device() : could not read file %s : %s\n", fname_hid, strerror(errno));
			return -1;
		}
		close(fd_hid);
	}
	closedir(dir);*/
	unsigned long long val;
	int device_hid = -1;
	int device_num = 0;

	int i = 0;
	while(*acpi_sbs_device_hid + i != "")
	{
		// linux/include/acpi/acpi_bus.h
		for_each_acpi_dev_match(dev, acpi_sbs_device_hid[i], NULL, -1)
		{
			// Each match appears here
		}
		// dev is now set
		if (!dev)
		{
			printf("probe_acpi_device() : could not find SBS controller : %s\n", strerror(ENODEV));
			return -1;
		}
		
		// Now we need to fetch _EC
		// linux/drivers/acpi/utils.c
		acpi_status status = AE_OK;
		union acpi_object element;
		struct acpi_buffer buffer = {0, NULL};
		buffer.length = sizeof(union acpi_object);
		buffer.pointer = &element;

		status = acpi_evaluate_object(dev->handle, "_EC", NULL, &buffer);
		if (ACPI_FAILURE(status))
		{
			// damn this is bad
			printf("probe_acpi_device() : could not evaluate object %s\n", acpi_sbs_device_hid[i]);
		}
		else
		{
			// ok
			if (element.type != ACPI_TYPE_INTEGER)
			{
				printf("probe_acpi_device() : expected int, got a different type\n");
			}
			else
			{
				// found
				
				printf("probe_acpi_device() : match %d\n", i);
				printf("  device hid : %s\n, _EC : %u\n", acpi_sbs_device_hid[i], val);
				device_hid = i;
				device_num++;
			}
		}

		// Need to put the device
		acpi_dev_put(dev);

		i++;
	}
	
	if (device_num > 1)
	{
		printf("probe_acpi_device() : multiple devices found, aborting...\n");
		return -1;
	}

	if (device_num == 0)
	{
		printf("probe_acpi_device() : no device found\n");
		return -1;
	}

	// linux/drivers/acpi/sbshc.c
	c->dev.offset = (val >> 8) & 0xff;
	c->dev.hid_index = device_hid;
	printf("found device !!");
	printf("  hid : %s, offset: %.2x\n", acpi_sbs_device_hid[i], c->dev.offset);
	return 1;
}

int device_open(struct args* c)
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
			printf("device_open() : could not open i2c device : %s\n", strerror(errno));
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
		if (probe_acpi_device(c) < 0)
			return -1;
		// https://tldp.org/HOWTO/IO-Port-Programming-2.html
		if (ioperm(c->dev.offset, ACPI_SMB_ALARM_DATA + 1, 1) < 0)
		{
			printf("device_open() : could not grant I/O ports permission : %s\n", strerror(errno));
			return -1;
		}
		return 0;
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
