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
	int16_t current; // mA
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



const char* BatteryAlarmName[] = {"Overcharged Alarm", "Terminate Charge Alarm", "Overtemperature Alarm", "Terminate Discharge Alarm", "Remaining Capacity Alarm", "Remaining Time Alarm"};

enum BatteryAlarm
{
	ALARM_OVERCHARGE,
	ALARM_TERMINATE_CHARGE,
	ALARM_TEMP,
	ALARM_TERMINATE_DISCHARGE,
	ALARM_REMAINING_CAP,
	ALARM_REMAINING_TIME,
	ALARM_NONE
};

const char* BatteryErrorName[] = {"OK", "Busy", "Reserved Command", "Unsupported Command", "AccessDenied", "Overflow/Underflow", "BadSize", "UnknownError"};

enum BatteryError
{
	SBS_ERROR_NONE = 0x0,
	SBS_ERROR_BUSY = 0x1,
	SBS_COMMAND_RESERVED = 0x2,
	SBS_COMMAND_UNSUPPORTED = 0x3,
	SBS_ACCESS_DENIED = 0x4,
	SBS_OU_FLOW = 0x5, // over/underflow
	SBS_BAD_SIZE = 0x6,
	SBS_ERROR_UNKNOWN = 0x7
};

struct battery_status
{
	enum BatteryAlarm alarms[ALARM_NONE];
	int alarms_num;
	enum BatteryError error;
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
