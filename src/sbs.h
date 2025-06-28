#ifndef SBS_SBS_H
#define SBS_SBS_H
// ==========================================
// Common platform-independent SBS operations
// ==========================================

#include "src/smbus.h"
#include "src/structs.h"


int sbs_get_basic_stats(struct battery_stats* stats, int fd)
{
	__s32 res = sbs_read_word(fd, 0x08);
	if (res < 0)
	{
		printf("sbs_get_basic_stats() : failed to get temperature\n");
		return 1;
	}
	stats->temp = (uint16_t)res;

#ifdef ENABLE_DEBUG
	printf("    word [temp] -> %.4x\n", res);
#endif

	res = sbs_read_word(fd, 0x09);
	if (res < 0)
	{
		printf("sbs_get_basic_stats() : failed to get voltage\n");
		return 1;
	}
	stats->voltage = (uint16_t)res;

#ifdef ENABLE_DEBUG
	printf("    word [volt] -> %.4x\n", res);
#endif

	res = sbs_read_word(fd, 0x0a);
	if (res < 0)
	{
		printf("sbs_get_basic_stats() : failed to get current\n");
		return 1;
	}
	stats->current = (int16_t)res;

#ifdef ENABLE_DEBUG
	printf("    word [curr] -> %.4x\n", res);
#endif

	return 0;
}


int sbs_get_device_metadata(struct device_metadata* meta, int fd)
{
	__s32 res = sbs_read_word(fd, 0x1b);
	if (res < 0)
	{
		printf("sbs_get_device_metadata() : failed to get manufacturing date\n");
		return 1;
	}
	meta->date_packed = (uint16_t)res;
	sprintf(meta->date, "%d", (meta->date_packed & 0b11111) + ((meta->date_packed >> 5) & 0b1111) * 100 + (((meta->date_packed >> 9) & 0b1111111) + 1980) * 10000);

#ifdef ENABLE_DEBUG
	printf("    word [date] -> %.4x\n", res);
#endif

	res = sbs_read_word(fd, 0x1c);
	if (res < 0)
	{
		printf("sbs_get_device_metadata() : failed to get serial number\n");
		return 1;
	}
	meta->serial = (uint16_t)res;

#ifdef ENABLE_DEBUG
	printf("    word [ser#] -> %.4x\n", res);
#endif

	__u8 data[33] = {}; // Reserve one for the \0
	res = sbs_read_block(fd, 0x20, data);
	if (res < 0)
	{
		printf("sbs_get_device_metadata() : failed to get manufacturer name\n");
		return 1;
	}

	memcpy(meta->vendor, (char*)data, 33);

#ifdef ENABLE_DEBUG
	printf("    block [vend] ->");
	smbus_print_block(data);
#endif

	res = sbs_read_block(fd, 0x21, data);
	if (res < 0)
	{
		printf("sbs_get_device_metadata() : failed to get device name\n");
		return 1;
	}

	memcpy(meta->device, (char*)data, 33);

#ifdef ENABLE_DEBUG
	printf("    block [dev ] ->");
	smbus_print_block(data);
#endif

	res = sbs_read_block(fd, 0x22, data);
	if (res < 0)
	{
		printf("sbs_get_device_metadata() : failed to get device chemistry\n");
		return 1;
	}

	memcpy(meta->chemistry, (char*)data, 33);

#ifdef ENABLE_DEBUG
	printf("    block [chem] ->");
	smbus_print_block(data);
#endif

	return 0;
}


int sbs_get_status(struct battery_status* status, int fd)
{
	__s32 res = sbs_read_word(fd, 0x16);
	if (res < 0)
	{
		printf("sbs_get_status() : failed to get status\n");
		return 1;
	}

#ifdef ENABLE_DEBUG
	printf("    word [stat] -> %.4x\n", res);
#endif

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
