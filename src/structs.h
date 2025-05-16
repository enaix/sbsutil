#ifndef SBS_STRUCTS_H
#define SBS_STRUCTS_H
#include <stdint.h>

enum ControllerDevice
{
	BQ40,
};


// SBS structures
// ==============

struct battery_stats
{
	uint16_t temp;    // K
	uint16_t voltage; // mV
	uint16_t current; // mA
};


struct device_metadata
{
	uint16_t date_packed; // Manufactuing date
	char date[9]; // Manufacturing date in the format DDMMYYYY
	uint16_t serial;
	char vendor[32];
	char device[32];
	char chemistry[32];
};



// Platform-independent ManufacturerAccess structures
// ==================================================

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


struct operation_status
{
	enum ShutdownMode { SHUTDN_NONE, SHUTDN_MANUAL, SHUTDN_LOW_VOLTAGE, SHUTDN_EMERGENCY, } shutdown;
	enum PermanentFailure { PF_NONE, PF_FAIL } pf;
	enum FuseStatus { FUSE_NONE, FUSE_DEPLOY } fuse;
};

#endif
