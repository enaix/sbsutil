#ifndef SBS_COMMANDS_H
#define SBS_COMMANDS_H

// ====================
// COMMANDS DEFINITIONS
// ====================

#include "src/structs.h"
#include "src/preflight.h"
#include "src/bq40.h"
#include "src/sbs.h"


void device_unlock_priviledges(int fd, struct args* config, const char* key)
{
	if (config->chip == AUTO)
	{
		printf("AUTO chip model fetch is not supported yet. Please specify the CHIP argument\n");
		quit(fd, 1);
	}

	if (!key)
	{
		printf("This command requires a KEY to elevate priviledges.\n");
		quit(fd, 1);
	}

	size_t len = strlen(key);
	uint32_t res;

	if (len == 8 || len == 10)
	{
		// AAaaBBbb
		res = strtol(key, NULL, 16);
	} else {
		printf("Wrong key format. KEY should be specified as AAaaBBbb or 0xAAaaBBbb\n");
		quit(fd, 1);
	}

	switch(config->chip)
	{
		case BQ40:
		{
			if (bq40_unlock_priviledges(res, fd) != 0)
			{
				printf("device_unlock_priviledges() : failed to write key\n");
				quit(fd, 1);
			}

			struct operation_status status;
			if (bq40_get_operation_status(&status, fd) != 0)
			{
				printf("device_unlock_priviledges() : failed to get status\n");
				quit(fd, 1);
			}

			switch(status.access)
			{
				case ACCESS_FULL:
					printf("[!] Successfully obtained FULL access. You are root\n");
					break;
				case ACCESS_UNSEALED:
					printf("[#] Unsealed\n");
					break;
				case ACCESS_SEALED:
					printf("[$] Sealed\n");
					break;
				default:
					printf("ERR while getting status\n");
					break;
			}
			
		}
		default:
			quit(fd, 1);
	}
}


#endif
