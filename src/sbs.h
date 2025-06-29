#ifndef SBS_SBS_H
#define SBS_SBS_H
// ==========================================
// Common platform-independent SBS operations
// ==========================================

#include "src/smbus.h"
#include "src/structs.h"


int sbs_get_basic_stats(struct battery_stats* stats, int fd, struct args* config)
{
	__s32 res = sbs_read_word(fd, 0x08);
	if (res < 0)
	{
		printf("sbs_get_basic_stats() : failed to get temperature\n");
		return 1;
	}
	stats->temp = (uint16_t)res;

	if (config->verbose)
		printf("    word [temp] -> %.4x\n", res);

	res = sbs_read_word(fd, 0x09);
	if (res < 0)
	{
		printf("sbs_get_basic_stats() : failed to get voltage\n");
		return 1;
	}
	stats->voltage = (uint16_t)res;

	if (config->verbose)
		printf("    word [volt] -> %.4x\n", res);

	res = sbs_read_word(fd, 0x0a);
	if (res < 0)
	{
		printf("sbs_get_basic_stats() : failed to get current\n");
		return 1;
	}
	stats->current = (int16_t)res;

	if (config->verbose)
		printf("    word [curr] -> %.4x\n", res);


	return 0;
}


int sbs_get_device_metadata(struct device_metadata* meta, int fd, struct args* config)
{
	__s32 res = sbs_read_word(fd, 0x1b);
	if (res < 0)
	{
		printf("sbs_get_device_metadata() : failed to get manufacturing date\n");
		return 1;
	}
	meta->date_packed = (uint16_t)res;
	sprintf(meta->date, "%d", (meta->date_packed & 0b11111) + ((meta->date_packed >> 5) & 0b1111) * 100 + (((meta->date_packed >> 9) & 0b1111111) + 1980) * 10000);

	if (config->verbose)
		printf("    word [date] -> %.4x\n", res);

	res = sbs_read_word(fd, 0x1c);
	if (res < 0)
	{
		printf("sbs_get_device_metadata() : failed to get serial number\n");
		return 1;
	}
	meta->serial = (uint16_t)res;

	if (config->verbose)
		printf("    word [ser#] -> %.4x\n", res);

	__u8 data[33] = {}; // Reserve one for the \0
	res = sbs_read_block(fd, 0x20, data);
	if (res < 0)
	{
		printf("sbs_get_device_metadata() : failed to get manufacturer name\n");
		return 1;
	}

	memcpy(meta->vendor, (char*)data, 32);
	meta->vendor[31] = '\0';

	if (config->verbose)
	{
		printf("    block [vend] ->");
		smbus_print_block(data);
	}

	res = sbs_read_block(fd, 0x21, data);
	if (res < 0)
	{
		printf("sbs_get_device_metadata() : failed to get device name\n");
		return 1;
	}

	memcpy(meta->device, (char*)data, 32);
	meta->device[31] = '\0';

	if (config->verbose)
	{
		printf("    block [dev ] ->");
		smbus_print_block(data);
	}

	res = sbs_read_block(fd, 0x22, data);
	if (res < 0)
	{
		printf("sbs_get_device_metadata() : failed to get device chemistry\n");
		return 1;
	}

	memcpy(meta->chemistry, (char*)data, 32);
	meta->chemistry[31] = '\0';

	if (config->verbose)
	{
		printf("    block [chem] ->");
		smbus_print_block(data);
	}

	__u8 data_l[64] = {};
	res = sbs_read_block_l(fd, 0x23, data_l, 33);
	memcpy(meta->manufacturer_info, (char*)data_l, 64);
	meta->manufacturer_info[32] = '\0';

	if (config->verbose)
	{
		printf("    block [minf] ->");
		smbus_print_block_l(data_l, 64);
	}

	return 0;
}


int sbs_get_status(struct battery_status* status, int fd, struct args* config)
{
	__s32 res = sbs_read_word(fd, 0x16);
	if (res < 0)
	{
		printf("sbs_get_status() : failed to get status\n");
		return 1;
	}

	if (config->verbose)
		printf("    word [stat] -> %.4x\n", res);


	int alarm_bits[] = {15, 14, 12, 11, 9, 8};
	status->alarms_num = 0;

	for (int a = ALARM_OVERCHARGE; a < ALARM_NONE; a++)
	{
		if (res & (1 << alarm_bits[a]))
		{
			status->alarms[status->alarms_num] = a;
			status->alarms_num++;
		}
	}

	status->error = res & 0b111;
	
	return 0;
}


#endif
