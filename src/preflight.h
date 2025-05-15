#ifndef SBS_PREFLIGHT_H
#define SBS_PREFLIGHT_H

#include "src/structs.h"
#include "src/bq40.h"


void sbs_preflight_check_chemid(int fd, enum ControllerDevice device)
{
	struct chem_id chem;
	int res;

	switch (device)
	{
		case BQ40:
			res = bq40_get_chemid(&chem, fd);
			break;
	}
	if (res != 0)
	{
		printf("sbs_preflight_check_chemid() : PREFLIGHT ERROR\n");
		quit(fd, 1);
	}

	printf("   ChemID : %.4x\n", chem.id);
}

void sbs_preflight_check_devicetype(int fd, enum ControllerDevice device)
{
	struct device_type dev;
	int res;

	switch (device)
	{
		case BQ40:
			res = bq40_get_devicetype(&dev, fd);
			break;
	}
	if (res != 0)
	{
		printf("sbs_preflight_check_devicetype() : PREFLIGHT ERROR\n");
		quit(fd, 1);
	}

	printf("   DeviceType : %.4x\n", dev.type);
}

void sbs_preflight_check_firmware_v(int fd, enum ControllerDevice device)
{
	struct firmware_version fw;
	int res;

	switch (device)
	{
		case BQ40:
			res = bq40_get_firmware_v(&fw, fd);
			break;
	}
	if (res != 0)
	{
		printf("sbs_preflight_check_firmware_v() : PREFLIGHT ERROR\n");
		quit(fd, 1);
	}

	printf("   FirmwareVersion : %s\n", fw.fw);
}

void sbs_preflight(int fd, enum ControllerDevice device)
{
	printf("PREFLIGHT : \n");
	sbs_preflight_check_chemid(fd, device);
	sbs_preflight_check_devicetype(fd, device);
	sbs_preflight_check_firmware_v(fd, device);
}
#endif
