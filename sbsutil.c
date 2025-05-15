#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
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

void sbs_preflight(int fd)
{
	// Use userspace kernel api for smbus I/O
	
	__u8 data[32] = {0x06, 0x00}; // LITTLE_ENDIAN

	// Set data to 0x0006 (ChemID) in LE
	__s32 res = i2c_smbus_write_block_data(fd, 0x44, 2, data);
	if (res < 0)
	{
		printf("sbs_preflight() : PREFLIGHT ERROR : could not request ChemID data\n");
		quit(fd, 1);
	}

	res = i2c_smbus_read_block_data(fd, 0x44, data);
	if (res < 0)
	{
		printf("sbs_preflight() : PREFLIGHT ERROR : could not read ChemID data\n");
		quit(fd, 1);
	}

	printf("PREFLIGHT : \n");
	printf("   ChemID : %d %d %d %d\n", data[0], data[1], data[2], data[3]);
}


void print_help(char** argv)
{
	printf("%s: usage :\n", argv[0]);
	printf("%s <i2c-adapter-num>\n", argv[0]);
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		print_help(argv);
		return 1;
	}

	errno = 0;
	int adapter = (int)strtol(argv[1], NULL, 10);
	if (errno != 0)
	{
		print_help(argv);
		return 1;
	}

	int fd = device_open(adapter);
	sbs_preflight(fd);

	quit(fd, 0);
}
