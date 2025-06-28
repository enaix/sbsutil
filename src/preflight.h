#ifndef SBS_PREFLIGHT_H
#define SBS_PREFLIGHT_H
// ================
// Preflight checks
// ================

#include "src/structs.h"
#include "src/bq40.h"
#include "src/sbs.h"


void sbs_status_get_chemid(int fd, struct args* config)
{
	struct chem_id chem;
	int res;

	switch (config->chip)
	{
		case BQ40:
			res = bq40_get_chemid(&chem, fd, config);
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

void sbs_status_get_devicetype(int fd, struct args* config)
{
	struct device_type dev;
	int res;

	switch (config->chip)
	{
		case BQ40:
			res = bq40_get_devicetype(&dev, fd, config);
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

void sbs_status_get_fw_v(int fd, struct args* config)
{
	struct firmware_version fw;
	int res;

	switch (config->chip)
	{
		case BQ40:
			res = bq40_get_firmware_v(&fw, fd, config);
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

void sbs_status_get_op_status(int fd, struct args* config)
{
	struct operation_status status;
	int res;

	printf("\n============\n");
	switch (config->chip)
	{
		case BQ40:
			res = bq40_get_operation_status(&status, fd, config);
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

void sbs_pf_logger(const char* desc)
{
	printf("      [!] %s\n", desc);
}

void sbs_status_get_pf_status(int fd, struct args* config)
{
	int res;
	int ok;

	printf("\n============\n");
	printf("\n    Permanent Failure status :\n      ");
	
	switch (config->chip)
	{
		case BQ40:
		{
			struct bq40_pf_status status;
			res = bq40_get_pf_status(&status, &ok, fd, config);
			if (res != 0)
				break;

			if (!ok)
				bq40_log_pf_status(&status, sbs_pf_logger);
			break;
		}
		default:
			quit(fd, 1);
	}

	if (res != 0)
	{
		printf("sbs_status_get_pf_status() : PREFLIGHT ERROR\n");
		quit(fd, 1);
	}

	if (ok)
	{
		printf("      No error detected\n");
	}
	printf("\n============\n");
}


void sbs_status_get_lifetime_data(int fd, struct args* config)
{
	int res;

	printf("\n============\n");
	printf("\n    Lifetime Data Block :\n      ");
	
	switch (config->chip)
	{
		case BQ40:
		{
			struct bq40_lifetime_block data;
			res = bq40_get_lifetime_data(&data, fd, config);
			if (res != 0)
				break;
			for (int i = 0; i < 5; i++)
			{
				printf("      block %d: ", i+1);
				//for (int s = 0; s < 32; i++)
				//{
					//printf("%02x", (__u8)data.blocks[i][s]);
				//}
				printf("%s", data.blocks[i]);
				printf("\n");
			}
			break;
		}
		default:
			quit(fd, 1);
	}

	if (res != 0)
	{
		printf("sbs_status_get_lifetime_data() : PREFLIGHT ERROR\n");
		quit(fd, 1);
	}
	printf("\n============\n");
}


// SBS-COMPLIANT PREFLIGHT COMMANDS
// ================================


void sbs_preflight_check_sanity(int fd, struct args* config)
{
	struct battery_stats stats;
	printf("    Battery stats : \n");
	if (sbs_get_basic_stats(&stats, fd, config) != 0)
	{
		printf("sbs_preflight_check_sanity() : PREFLIGHT ERROR\n");
		quit(fd, 1);
	}

	printf("      Temperature: %uºK\n", (stats.temp));
	printf("      Voltage: %u mV\n", stats.voltage);
	printf("      Current: %u mA\n", stats.current);


	struct device_metadata meta;
	printf("    Device metadata : \n");
	if (sbs_get_device_metadata(&meta, fd, config) != 0)
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

void sbs_preflight_check_status(int fd, struct args* config)
{
	struct battery_status status;
	printf("    SBS Status : \n");
	if (sbs_get_status(&status, fd, config) != 0)
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

void sbs_preflight(int fd, struct args* config)
{
	printf("PREFLIGHT : \n");
	sbs_preflight_check_sanity(fd, config);
	sbs_preflight_check_status(fd, config);
}

void device_fetch_status(int fd, struct args* config)
{
	printf("DEVICE STATUS : \n");
	if (config->chip == AUTO)
	{
		printf("AUTO chip model fetch is not supported yet. Please specify the CHIP argument\n");
		quit(fd, 1);
	}

	sbs_status_get_chemid(fd, config);
	sbs_status_get_devicetype(fd, config);
	sbs_status_get_fw_v(fd, config);
	sbs_status_get_op_status(fd, config);
	sbs_status_get_pf_status(fd, config);
	sbs_status_get_lifetime_data(fd, config);
}
#endif
