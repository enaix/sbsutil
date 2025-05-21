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
	if (argc != 2)
	{
		print_help(argv);
		return 1;
	}

	char* adapter = argv[1];
	/*errno = 0;
	int adapter = (int)strtol(argv[1], NULL, 10);
	if (errno != 0)
	{
		print_help(argv);
		return 1;
	}*/

	int fd = device_open(adapter);
	sbs_preflight(fd, BQ40);

	quit(fd, 0);
}
