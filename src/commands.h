#ifndef SBS_COMMANDS_H
#define SBS_COMMANDS_H

// ====================
// COMMANDS DEFINITIONS
// ====================

#include "src/structs.h"
#include "src/preflight.h"
#include "src/bq40.h"
#include "src/sbs.h"

#include <signal.h>


uint32_t _bruteforce_key;
int _bruteforce_fd;


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
			if (bq40_unlock_priviledges(res, fd, config) != 0)
			{
				printf("device_unlock_priviledges() : failed to write key\n");
				quit(fd, 1);
			}

			struct operation_status status;
			if (bq40_get_operation_status(&status, fd, config) != 0)
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


void bruteforce_graceful_shutdown(int fd)
{
	printf("Iteration stopped at %.4x\n", _bruteforce_key);
	quit(fd, 1);
}


void bruteforce_sigterm(int signo)
{
	bruteforce_graceful_shutdown(_bruteforce_fd);
}


void device_bruteforce(int fd, struct args* config, const char* key_start, const char* key_end)
{
	if (config->chip == AUTO)
	{
		printf("AUTO chip model fetch is not supported yet. Please specify the CHIP argument\n");
		quit(fd, 1);
	}

	if (!key_start)
		key_start = "00000000";
	if (!key_end)
		key_end = "ffffffff";

	size_t len_s = strlen(key_start), len_r = strlen(key_end);
	uint32_t res_s, res_r;

	if ((len_s == 8 || len_s == 10) && (len_r == 8 || len_r == 10))
	{
		// AAaaBBbb
		res_s = strtol(key_start, NULL, 16);
		res_r = strtol(key_end, NULL, 16);
	} else {
		printf("Wrong key format. START and END should be specified as AAaaBBbb or 0xAAaaBBbb\n");
		quit(fd, 1);
	}

	// Initialize the bruteforce
	// =========================
	
	if (signal(SIGINT, bruteforce_sigterm) == SIG_ERR)
		printf("device_bruteforce() : cannot create sigint handler\n");

	struct operation_status status_old, status;
	switch(config->chip)
	{
		case BQ40:
			if (bq40_get_operation_status(&status_old, fd, config) != 0)
			{
				printf("device_bruteforce() : failed to get initial status\n");
				bruteforce_graceful_shutdown(fd);
			}
			break;
		default:
			quit(fd, 1);
	}
	
	config->verbose = 0;
	_bruteforce_fd = fd;

	uint64_t keys_total = (res_r - res_s);
	uint64_t perc_last = UINT64_MAX;
	uint32_t key_last = res_s;
	int retries_total = 3, retries = 0, errors_total = 0;
	printf("Initializing bruteforce from %.8x to %.8x with %ld keys total...\n", res_s, res_r, keys_total);

	for (uint32_t key = res_s; key <= res_r; key++)
	{
		_bruteforce_key = key;

		uint32_t iter = key - res_s;
		uint64_t perc = (iter * 10000) / keys_total; // in 100.00%
		switch(config->chip)
		{
			case BQ40:
			{
				if (bq40_unlock_priviledges(key, fd, config) != 0)
				{
					printf("[%.8x] device_bruteforce() : failed to write key\n", key);
					fflush(stdout);
					errors_total++;
					if (retries > retries_total || iter == 0)
						bruteforce_graceful_shutdown(fd);
					// else retry
					sleep(5); // We need to clear the current state
					retries++;
					key--; // Retry the same key
					continue;
				}
				break;
			}
			default:
				quit(fd, 1);
		}
		
		if (perc_last != perc || iter % 250 == 0)
		{
			switch(config->chip)
			{
				case BQ40:
					if (bq40_get_operation_status(&status, fd, config) != 0)
					{
						printf("[%.8x] device_bruteforce) : failed to get status\n", key);
						fflush(stdout);
						errors_total++;
						bruteforce_graceful_shutdown(fd);
						if (retries > retries_total || iter == 0)
							bruteforce_graceful_shutdown(fd);
						// else retry
						sleep(5); // We need to clear the current state
						retries++;
						key--; // Retry the same key
						continue;
					}
					break;
				default:
					quit(fd, 1);
			}

			if (status.access != status_old.access)
			{
				// Found
				printf("  Found the key in range [%.8d, %.8d]\n", key_last, key);
				switch(status.access)
				{
					case ACCESS_FULL:
						printf("  [!] Successfully obtained FULL access. You are root\n");
						break;
					case ACCESS_UNSEALED:
						printf("  [#] Unsealed\n");
						break;
					case ACCESS_SEALED:
						printf("  [$] Sealed\n");
						break;
					default:
						printf("  ERR while getting status\n");
						break;
				}
				fflush(stdout);
				break;
			}
			printf("\r[%.8x] Bruteforce : %.2ld.%.2ld%% done...", key, perc / 100, perc % 100);
			fflush(stdout);

			perc_last = perc;
			key_last = key;
		}
		retries = 0;
	}

	

	printf("  Stats: %d errors out of %ld keys total\n", errors_total, keys_total);
	fflush(stdout);
}


#endif
