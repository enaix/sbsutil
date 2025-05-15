#ifndef SBS_PREFLIGHT_H
#define SBS_PREFLIGHT_H

#include "src/structs.h"
#include "src/bq40.h"


void sbs_preflight_check_chemid(int fd, enum ControllerDevice device)
{
	struct chem_id chem;
	int res;

	switch (device)
	{
		case BQ40:
			res = bq40_get_chemid(&chem, fd);
			break;
	}
	if (res != 0)
	{
		printf("sbs_preflight() : PREFLIGHT ERROR\n");
		quit(fd, 1);
	}

	printf("   ChemID : %.2x\n", chem.id);
}


void sbs_preflight(int fd, enum ControllerDevice device)
{
	printf("PREFLIGHT : \n");
	sbs_preflight_check_chemid(fd, device);
}
#endif
