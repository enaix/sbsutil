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


// DeviceType: IC part number
struct device_type
{
	unsigned char mac[2];
	uint32_t type;
};


struct firmware_version
{
	unsigned char mac[2];
	char fw[32];
};

#endif
