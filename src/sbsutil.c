#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

#include "src/structs.h"
#include "src/commands.h"


// Prints chip names to the chip_names variable
// Assume the max size of each chip name is no longer than 32 chars
#define GET_CHIP_NAMES  char chip_names[(sizeof(ControllerNames)/sizeof(char*) - 1) * 32] = {}; \
			for (size_t i = 0; ControllerNames[i]; i++) \
			{ \
				strcat(chip_names, ControllerNames[i]); \
				if (ControllerNames[i+1]) \
					strcat(chip_names, ","); \
			}



void print_help()
{
	GET_CHIP_NAMES

	printf(
			"Usage: sbsutil [OPTION]... [-f|--file=FILE] [COMMAND]\n"
			"Communicate with the Smart Battery System IC.\n\n"
			"Runs preflight checks if COMMAND is not given.\n"
			"If the FILE is not specified, communicates over the ACPI bus using the sbsctl module.\n"
			"NOTE: i2c commands can potentially cause harm when sent to a wrong device!\n\n"
			"  -h, --help     \tprint this help message and exit\n"
			"  -v, --verbose  \tprint out all data\n"
			"  -f, --file=FILE\tcommunicate over i2c device located in FILE\n"
			"  -c, --chip=CHIP\toverride SBS controller model. CHIP is one of: [%s]\n\n"
			"Commands:\n"
			"  preflight      \tRun non-destructive checks using standard SBS commands\n"
			"  status         \tFetch device-specific status registers\n"
			"  key            \tElevate priviledges with a KEY, which should be specified as AAaaBBbb or 0xAAaaBBbb\n\n"
			"Examples:\n"
			"  sbsutil preflight    \tRun preflight checks without executing ManufacturerAccess commands. Requires loaded sbsctl kernel module to perform ACPI calls.\n"
			"  sbsutil -f /dev/i2c-2\tRun preflight checks over the second i2c device.\n", chip_names);
}


int command_exec(int fd, const char* cmd, const char* cmd_arg, struct args* config)
{
	if (!cmd || strcmp(cmd, "preflight") == 0)
	{
		// Default: run preflight checks
		sbs_preflight(fd, config);
	}
	else if (strcmp(cmd, "status") == 0)
	{
		device_fetch_status(fd, config);
	}
	else if (strcmp(cmd, "key") == 0)
	{
		device_unlock_priviledges(fd, config, cmd_arg);
	}
	else
	{
		printf("Command not recognised: %s\n", cmd);
		print_help();
		return 1;
	}
	return 0;
}


int main(int argc, char** argv)
{
	
	struct args config = {.verbose=0, .i2c=0, .file=NULL, .chip=AUTO};
	const char* chip_name = NULL;

	static struct option opts[] = {
		{"help", no_argument, NULL, 'h'},
		{"verbose", no_argument, NULL, 'v'},
		{"file", required_argument, NULL, 'f'},
		{"chip", required_argument, NULL, 'c'},
		{NULL, 0, NULL, 0}
	};
	int opt = 0;

	while((opt = getopt_long(argc, argv, "hvf:c:", opts, NULL)) != -1)
	{
		switch (opt)
		{
			case 'h':
				print_help();
				return 0;

			case 'v':
				config.verbose = 1;
				break;
			case 'f':
				#ifndef SBS_ENABLE_I2C
				printf("sbsutil was compiled without I2C support.\n");
				return 1;
				#endif
				config.i2c = 1;
				config.file = optarg; // pointer to argv, safe to assign
				break;
			case 'c':
				chip_name = optarg;   // ditto
				break;
			case '?':
				printf("Argument not recognised: %s\n", argv[optind]);
				print_help();
				return 1;
		}
	}

	if (chip_name)
	{
		int found = 0;
		for (size_t i = 0; ControllerNames[i]; i++)
		{
			if (strcmp(chip_name, ControllerNames[i]) == 0)
			{
				config.chip = (enum ControllerDevice)i;
				found = 1;
				break;
			}
		}
		if (!found)
		{
			GET_CHIP_NAMES
			printf("Chip name not recognised: %s. CHIP should be one of: [%s]\n", chip_name, chip_names);
			print_help();
			return 1;
		}
	}

	const char* cmd = NULL;
	const char* cmd_arg = NULL;

	if (optind < argc)
	{
		// Save command
		cmd = argv[optind];
		if (optind + 1 < argc)
			cmd_arg = argv[optind + 1];

		// Other arguments
		if (optind + 2 < argc)
		{
			for (int i = optind + 1; i < argc; i++)
				printf("Argument not recognised: %s\n", argv[i]);
			print_help();
			return 1;
		}
	}

	int fd = device_open(&config);
	if (fd < 0)
		return 1;
	
	int res = command_exec(fd, cmd, cmd_arg, &config);

	quit(fd, res);
}
