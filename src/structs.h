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
	uint32_t id;
};


// DeviceType: IC part number
struct device_type
{
	uint32_t type;
};


struct firmware_version
{
	char fw[32];
};

#endif
