#ifndef SBS_PREFLIGHT_H
#define SBS_PREFLIGHT_H
// ================
// Preflight checks
// ================

#include "src/structs.h"
#include "src/bq40.h"
#include "src/sbs.h"


void sbs_status_get_chemid(int fd, enum ControllerDevice device)
{
	struct chem_id chem;
	int res;

	switch (device)
	{
		case BQ40:
			res = bq40_get_chemid(&chem, fd);
			break;
		default:
			quit(fd, 1);
	}
	if (res != 0)
	{
		printf("sbs_status_get_chemid() : PREFLIGHT ERROR\n");
		quit(fd, 1);
	}

	printf("    ChemID : %.4x\n", chem.id);
}

void sbs_status_get_devicetype(int fd, enum ControllerDevice device)
{
	struct device_type dev;
	int res;

	switch (device)
	{
		case BQ40:
			res = bq40_get_devicetype(&dev, fd);
			break;
		default:
			quit(fd, 1);
	}
	if (res != 0)
	{
		printf("sbs_status_get_devicetype() : PREFLIGHT ERROR\n");
		quit(fd, 1);
	}

	printf("    DeviceType : %.4x\n", dev.type);
}

void sbs_status_get_fw_v(int fd, enum ControllerDevice device)
{
	struct firmware_version fw;
	int res;

	switch (device)
	{
		case BQ40:
			res = bq40_get_firmware_v(&fw, fd);
			break;
		default:
			quit(fd, 1);
	}
	if (res != 0)
	{
		printf("sbs_status_get_fw_v() : PREFLIGHT ERROR\n");
		quit(fd, 1);
	}

	printf("    FirmwareVersion : %s\n", fw.fw);
}

void sbs_status_get_op_status(int fd, enum ControllerDevice device)
{
	struct operation_status status;
	int res;

	printf("\n============\n");
	switch (device)
	{
		case BQ40:
			res = bq40_get_operation_status(&status, fd);
			break;
		default:
			quit(fd, 1);
	}

	if (res != 0)
	{
		printf("sbs_status_get_op_status() : PREFLIGHT ERROR\n");
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
	printf("      Access: ");
	switch(status.access)
	{
		case ACCESS_FULL:
			printf("[!] FULL\n");
			break;
		case ACCESS_UNSEALED:
			printf("[#] Unsealed\n");
			break;
		case ACCESS_SEALED:
			printf("[$] Sealed\n");
			break;
		default:
			printf("ERR\n");
			break;
	}
	printf("\n============\n");
}


// SBS-COMPLIANT PREFLIGHT COMMANDS
// ================================


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


	struct device_metadata meta;
	printf("    Device metadata : \n");
	if (sbs_get_device_metadata(&meta, fd) != 0)
	{
		printf("sbs_preflight_check_sanity() : PREFLIGHT ERROR\n");
		quit(fd, 1);
	}

	printf("      Manufacturing date: %s\n", meta.date);
	printf("      Serial number: %.4x\n", meta.serial);
	printf("      Vendor name: %s\n", meta.vendor);
	printf("      Device name: %s\n", meta.device);
	printf("      Device chem: %s\n", meta.chemistry);
}

void sbs_preflight_check_status(int fd)
{
	struct battery_status status;
	printf("    SBS Status : \n");
	if (sbs_get_status(&status, fd) != 0)
	{
		printf("sbs_preflight_check_status() : PREFLIGHT ERROR\n");
		quit(fd, 1);
	}

	printf("      Alarms: ");
	for (int i = 0; i < status.alarms_num; i++)
	{
		printf("[%s] ", BatteryAlarmName[i]);
	}
	printf("\n");

	printf("      Error code: [%s]\n", BatteryErrorName[status.error]);
}

void sbs_preflight(int fd)
{
	printf("PREFLIGHT : \n");
	sbs_preflight_check_sanity(fd);
	sbs_preflight_check_status(fd);
}

void device_fetch_status(int fd, struct args* config)
{
	printf("DEVICE STATUS : \n");
	if (config->chip == AUTO)
	{
		printf("AUTO chip model fetch is not supported yet. Please specify the CHIP argument\n");
		quit(fd, 1);
	}

	sbs_status_get_chemid(fd, config->chip);
	sbs_status_get_devicetype(fd, config->chip);
	sbs_status_get_fw_v(fd, config->chip);
	sbs_status_get_op_status(fd, config->chip);
}
#endif
