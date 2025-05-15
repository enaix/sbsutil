#ifndef SBS_STRUCTS_H
#define SBS_STRUCTS_H
#include <stdint.h>

enum ControllerDevice
{
	BQ40,
};


// ChemicalID
struct chem_id
{
	unsigned char mac[2]; // MAC command
	uint32_t id;
};

#endif
