#ifndef SBS_SBS_H
#define SBS_SBS_H
// ==========================================
// Common platform-independent SBS operations
// ==========================================

#include "src/smbus.h"
#include "src/structs.h"


int sbs_get_basic_stats(struct battery_stats* stats, int fd)
{
	__s32 res = i2c_smbus_read_word_data(fd, 0x08);
	if (res < 0)
	{
		printf("sbs_get_basic_stats() : failed to get temperature\n");
		return 1;
	}
	stats->temp = (uint16_t)res;

#ifdef ENABLE_DEBUG
	printf("    word [temp] -> %.4x\n", res);
#endif

	res = i2c_smbus_read_word_data(fd, 0x09);
	if (res < 0)
	{
		printf("sbs_get_basic_stats() : failed to get voltage\n");
		return 1;
	}
	stats->voltage = (uint16_t)res;

#ifdef ENABLE_DEBUG
	printf("    word [volt] -> %.4x\n", res);
#endif

	res = i2c_smbus_read_word_data(fd, 0x0a);
	if (res < 0)
	{
		printf("sbs_get_basic_stats() : failed to get current\n");
		return 1;
	}
	stats->current = (uint16_t)res;

#ifdef ENABLE_DEBUG
	printf("    word [curr] -> %.4x\n", res);
#endif

	return 0;
}




#endif
