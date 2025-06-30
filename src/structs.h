#ifndef SBS_STRUCTS_H
#define SBS_STRUCTS_H
#include <stdint.h>


// Defines
// =======

#ifndef likely
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#endif


// Application structures
// ======================



static char* ControllerNames[] = {
	"bq40", // 0
	"auto", // 1
	NULL
};

enum ControllerDevice
{
	BQ40, // 0
	AUTO, // 1
};


struct sbs_device
{
	int hid_index; // index of the hid device
	unsigned char offset;     // device address
};


struct args
{
	int verbose; // print verbose info, including dumps
	int i2c;     // read from the i2c device instead of EC
	char* file;  // i2c device path

	struct sbs_device dev;
	enum ControllerDevice chip; // may be AUTO
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
	char manufacturer_info[64];
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
	enum SecurityMode { ACCESS_FULL, ACCESS_UNSEALED, ACCESS_SEALED, ACCESS_ERR } access;
};

#endif
