#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>

#include "src/structs.h"
#include "src/preflight.h"

void print_help(char** argv)
{
	printf("%s: usage :\n", argv[0]);
	printf("%s path-to-i2c-device\n", argv[0]);
}

int main(int argc, char** argv)
{
	
	struct args config{.verbose=0, .i2c=0, .file=NULL};

	struct option opts[] = {
		{"help", no_argument, NULL, 'h'},
		{"verbose", no_argument, NULL, 'v'},
		{"file", optional_argument, NULL, 'f'},
		{NULL, 0, NULL, 0}
	};
	int opt = 0;

	while(getopt_long(argc, argv, "hvf::"))
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
			default:
				print_help(argv);
				return 1;
		}
	}

	if (optind < argc)
	{
		for (int i = optind; i < argc; i++)
			printf("Argument not recognised: %s\n", argv[index]);
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

	int fd = device_open(config);
	if (fd < 0)
		return 1;
	sbs_preflight(fd, BQ40);

	quit(fd, 0);
}
