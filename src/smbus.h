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
#include <ctype.h>

#include <linux/types.h>
//#include <sys/io.h>

// i2c drivers
#ifdef SBS_ENABLE_I2C
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#endif

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
char* acpi_sbs_device_hid[] = {"ACPI0001", "ACPI0005", "PNP0C09", "PNP0C0A", ""}; // "" is an end of array marker


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


int call_ec_method(struct args* c, const char* device, int use_acpi_call)
{
	if (use_acpi_call)
	{
		char buf[256];
		char path[] = "/proc/acpi/call";
		unsigned long long val;

		snprintf(buf, sizeof(buf), "%s._EC", device);
		if (c->verbose)
			printf("call_ec_method() : acpi_call : %s\n", buf);

		int fd = open(path, O_RDWR);
		if (fd < 0)
		{
			if (errno == ENOENT)
			{
				printf("call_ec_method() : acpi_call kernel module is not loaded\n");
				return -1;
			}
			printf("call_ec_method() : failed to open %s : %s\n", path, strerror(errno));
			return -1;
		}

		ssize_t bytes = write(fd, buf, strlen(buf));
	
		if (bytes < 0)
		{
			printf("call_ec_method() : failed to execute %s call : %s\n", buf, strerror(errno));
			close(fd);
			return -1;
		}
	
		char res[32];
		bytes = read(fd, res, 32);
	
		if (bytes < 0)
		{
			printf("call_ec_method() : failed to read %s method result : %s\n", buf, strerror(errno));
			close(fd);
			return -1;
		}

		close(fd);
		
		// Check the result

		if (res[0] == '0' && res[1] == 'x')
		{
			// success
			#ifdef ENABLE_DEBUG
			printf("call_ec_method() : success : acpi_call returned %s\n", res);
			#endif
			
			val = atoi(res + 2);
			// linux/drivers/acpi/sbshc.c
			c->dev.offset = (val >> 8) & 0xff;
		}
		else if (strcmp(res, "not called") == 0)
		{
			printf("call_ec_method() : acpi_call returned nothing\n");
			return -1;
		}
		else if (!strstr(res, "Error:"))
		{
			printf("call_ec_method() : acpi_call : %s\n", res);
			return -1;
		}
		else
		{
			printf("call_ec_method() : acpi_call returned unsupported type : %s\n", res);
			return -1;
		}
		return 1;
	}
	else
	{
		char path[256];
		snprintf(path, sizeof(path), "/sys/kernel/debug/acpi/%s/call", device);
		int fd = open(path, O_RDWR);
		if (fd < 0)
		{
			printf("call_ec_method() : failed to open %s : %s\n", path, strerror(errno));
			return -1;
		}

		char cmd[] = "EC";
		ssize_t bytes = write(fd, cmd, sizeof(cmd) / sizeof(char));
	
		if (bytes < 0)
		{
			printf("call_ec_method() : failed to execute %s method : %s\n", cmd, strerror(errno));
			close(fd);
			return -1;
		}
	
		char buf[32];
		bytes = read(fd, buf, 32);
	
		if (bytes < 0)
		{
			printf("call_ec_method() : failed to read %s method result : %s\n", cmd, strerror(errno));
			close(fd);
			return -1;
		}
		// TODO fetch result
		printf("call_ec_method() : result : %s\n", buf);

	
		close(fd);
		return 1;
	}
}

char* strtrim_end(char* str)
{
	int end = strlen(str) - 1;
	while (end > 0 && isspace(str[end])) str[end] = '\0';

	return str;
}


int probe_acpi_device(struct args* c)
{
	// https://lwn.net/Articles/367630/
	
	// Iterate over each device which matches the criteria
	// Check which devices are present on the system bus
	DIR *dir;
	struct dirent* dir_e;
	static char d_path[] = "/sys/bus/acpi/devices";
	dir = opendir(d_path);
	if (!dir)
	{
		printf("probe_acpi_tables() : could not access /sys/bus/acpi/devices : %s\n", strerror(errno));
		return -1;
	}

	int device_num = 0;
	while ((dir_e = readdir(dir)) != NULL)
	{
		if (strcmp(dir_e->d_name, ".") == 0 || strcmp(dir_e->d_name, "..") == 0)
			continue;

		char fname[288];
		snprintf(fname, 288, "%s/%s/hid", d_path, dir_e->d_name); // path to hid file

		int fd_hid = open(fname, O_RDONLY);
		if (fd_hid < 0)
		{
			if (errno == ENOENT)
				continue; // No hid file

			printf("probe_acpi_device() : could not open hid file %s : %s\n", fname, strerror(errno));
			continue;
		}

		char hid[32] = ""; // supposed to be 8
		ssize_t res = read(fd_hid, hid, 32);
		if (res < 0 || res == 32)
		{
			if (res == 32)
				printf("probe_acpi_device() : %s has bad hid format\n", fname);
			else
				printf("probe_acpi_device() : could not read file %s : %s\n", fname, strerror(errno));
			return -1;
		}
		close(fd_hid); // Close the hid file
		strtrim_end(hid);

		// Check if the HID matches
		int i = 0;

		//printf("probe_acpi_device() : checking hid %s...\n", hid);
		while(acpi_sbs_device_hid[i][0] != '\0')
		{
			if (strncmp(hid, acpi_sbs_device_hid[i], strlen(acpi_sbs_device_hid[i])) == 0)
			{
				// hid matches

				// ---------------
				// We need to fetch the full device ACPI path
				if (c->verbose)
				{
					printf("probe_acpi_device() : device hid matches : %s\n", fname);
				}

				snprintf(fname, 288, "%s/%s/path", d_path, dir_e->d_name);
				int fd_p = open(fname, O_RDONLY);
				if (fd_hid < 0)
				{
					if (errno == ENOENT)
						continue; // No path file
					printf("probe_acpi_device() : could not open file %s : %s\n", fname, strerror(errno));
					break;
				}

				char path[128] = "";
				ssize_t res = read(fd_p, path, 128);
				if (res < 0)
				{
					printf("probe_acpi_device() : could not read file %s : %s\n", fname, strerror(errno));
					return -1;
				}
				close(fd_p);

				strtrim_end(path);
				
				if (call_ec_method(c, path, 1) < 0)
				{
					printf("probe_acpi_device() : could not execute _EC method for %s\n", hid);
				}
				else
				{
					// success
					printf("found device !!");
					printf("  hid : %s, offset: %.2x\n", acpi_sbs_device_hid[i], c->dev.offset);
					device_num++;
				}

			}
			i++;
		}
	}

	closedir(dir);
	
	if (device_num == 0)
	{
		printf("probe_acpi_device() : no acpi device found\n");
		return -1;
	}
	else if (device_num > 1)
	{
		printf("probe_acpi_device() : error : more than 1 device found\n");
		return -1;
	}
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
		/*if (ioperm(c->dev.offset, ACPI_SMB_ALARM_DATA + 1, 1) < 0)
		{
			printf("device_open() : could not grant I/O ports permission : %s\n", strerror(errno));
			return -1;
		}*/
		printf("device_open() : not implemented\n");
		return -1; // Not implemented
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
	__s32 res = i2c_smbus_read_i2c_block_data(fd, command, 32, result);
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


int sbs_exec_block_command(__u8 command, const __u8* data, __u8* result, __u8 length, int fd)
{
	#ifdef SBS_ENABLE_I2C
	__s32 res = i2c_smbus_write_block_data(fd, command, length, data);
	if (res < 0)
	{
		printf("sbs_exec_block_command() : command execution failed\n");
		sbs_log_error(res);
		return 1;
	}

	//usleep(50*1000);

	res = i2c_smbus_read_i2c_block_data(fd, command, 32, result);
	if (res < 0)
	{
		printf("sbs_exec_block_command() : could not read command result\n");
		sbs_log_error(res);
		return 1;
	}
	__u8 len = result[0];
	memmove(result, result + 1, 31);
	printf("sbs_exec_block_command() : read %d bytes\n", len);
	// We need to return the block size

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
