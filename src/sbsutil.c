#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

#include "src/structs.h"
#include "src/preflight.h"

void print_help(char** argv)
{
	printf(
			"Usage: sbsutil [OPTION]... [-f|--file=FILE]\n"
			"Communicate with the Smart Battery System IC.\n\n"
			"Performs non-destructive checks by default.\n"
			"If the FILE is not specified, communicates over the ACPI bus using the sbsctl module.\n"
			"NOTE: i2c commands can potentially cause harm when sent to a wrong device!\n\n"
			"  -h, --help     \tprint this help message and exit\n"
			"  -v, --verbose  \tprint out all data\n"
			"  -f, --file=FILE\tcommunicate over i2c device located in FILE\n\n"
			"Examples:\n"
			"  sbsutil              \tRun preflight checks without executing ManufacturerAccess commands. Requires loaded sbsctl kernel module to perform ACPI calls.\n"
			"  sbsutil -f /dev/i2c-2\tRun preflight checks over the second i2c device.\n");
}

int main(int argc, char** argv)
{
	
	struct args config = {.verbose=0, .i2c=0, .file=NULL};

	static struct option opts[] = {
		{"help", no_argument, NULL, 'h'},
		{"verbose", no_argument, NULL, 'v'},
		{"file", optional_argument, NULL, 'f'},
		{NULL, 0, NULL, 0}
	};
	int opt = 0;

	while((opt = getopt_long(argc, argv, "hvf::", opts, NULL)) != -1)
	{
		switch (opt)
		{
			case 'h':
				print_help(argv);
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
			case '?':
				printf("Argument not recognised: %s\n", argv[optind]);
				print_help(argv);
				return 1;
		}
	}

	if (optind < argc)
	{
		for (int i = optind; i < argc; i++)
			printf("Argument not recognised: %s\n", argv[i]);
		print_help(argv);
		return 1;
	}

	//char* adapter = argv[1];
	/*errno = 0;
	int adapter = (int)strtol(argv[1], NULL, 10);
	if (errno != 0)
	{
		print_help(argv);
		return 1;
	}*/

	int fd = device_open(&config);
	if (fd < 0)
		return 1;
	sbs_preflight(fd, BQ40);

	quit(fd, 0);
}
