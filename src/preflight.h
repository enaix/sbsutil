#ifndef SBS_PREFLIGHT_H
#define SBS_PREFLIGHT_H
// ================
// Preflight checks
// ================

#include "src/structs.h"
#include "src/bq40.h"
#include "src/sbs.h"


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

	printf("    ChemID : %.4x\n", chem.id);
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

	printf("    DeviceType : %.4x\n", dev.type);
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

	printf("    FirmwareVersion : %s\n", fw.fw);
}

void sbs_preflight_check_operation_status(int fd, enum ControllerDevice device)
{
	struct operation_status status;
	int res;

	printf("\n============\n");
	switch (device)
	{
		case BQ40:
			res = bq40_get_operation_status(&status, fd);
			break;
	}

	if (res != 0)
	{
		printf("sbs_preflight_check_operation_status() : PREFLIGHT ERROR\n");
		quit(fd, 1);
	}

	printf("\n    OperationStatus :\n      ");
	switch(status.shutdown)
	{
		case SHUTDN_EMERGENCY:
			printf("[!] EMERGENCY SHUTDOWN\n");
			break;
		case SHUTDN_LOW_VOLTAGE:
			printf("LOW VOLTAGE SHUTDOWN\n");
			break;
		case SHUTDN_MANUAL:
			printf("MANUAL SHUTDOWN\n");
			break;
		default:
			printf("No shutdown status\n");
			break;
	}

	printf("      ");
	switch(status.pf)
	{
		case PF_FAIL:
			printf("[!] PERMANENT FAILURE\n");
			break;
		default:
			printf("No permanent failure\n");
			break;
	}

	printf("      ");
	switch(status.fuse)
	{
		case FUSE_DEPLOY:
			printf("[!] FUSE DEPLOYED\n");
			break;
		default:
			printf("Fuse disarmed\n");
			break;
	}
}

void sbs_preflight_check_sanity(int fd)
{
	struct battery_stats stats;
	printf("    Battery stats : \n");
	if (sbs_get_basic_stats(&stats, fd) != 0)
	{
		printf("sbs_preflight_check_sanity() : PREFLIGHT ERROR\n");
		quit(fd, 1);
	}

	printf("      Temperature: %uºK\n", (stats.temp));
	printf("      Voltage: %u mV\n", stats.voltage);
	printf("      Current: %u mA\n", stats.current);
}

void sbs_preflight(int fd, enum ControllerDevice device)
{
	printf("PREFLIGHT : \n");
	sbs_preflight_check_chemid(fd, device);
	sbs_preflight_check_devicetype(fd, device);
	sbs_preflight_check_firmware_v(fd, device);
	sbs_preflight_check_operation_status(fd, device);
	sbs_preflight_check_sanity(fd);
}
#endif
