#ifndef SBS_BQ_40_H
#define SBS_BQ_40_H
// ========================
// BQ40** controller family
// ========================
// 
// This file contains the SMBus logic for communicating with the BQ40* chip

#include "src/smbus.h"
#include "src/structs.h"


// ManufacturerAccess() operations
//
// Note: these are the SBS command codes for the following operations:
// ManufacturerAccess()      : 0x00
// ManufacturerBlockAccess() : 0x44
//
// ===============================


int bq40_get_chemid(struct chem_id* chem, int fd)
{
	__u8 command[2] = {0x06, 0x00}; // LITTLE_ENDIAN
	__u8 data[32] = {}; // zero-init
	if (sbs_exec_block_command(0x44, command, data, 2, fd) != 0)
	{
		printf("bq40_get_chemid() : could not get ChemID data\n");
		return 1;
	}

#ifdef ENABLE_DEBUG
	printf("    block -> ");
	smbus_print_block(data);
#endif

	if (sbs_block_check_mac(command, data, 2) != 0)
	{
		printf("bq40_get_chemid() : bad MAC command\n");
		return 1;
	}

	chem->id = smbus_block_LE_to_ui32(data + 2, 2);

	return 0;
}


int bq40_get_devicetype(struct device_type* dev, int fd)
{
	__u8 command[2] = {0x01, 0x00}; // LITTLE_ENDIAN
	__u8 data[32] = {}; // zero-init
	if (sbs_exec_block_command(0x44, command, data, 2, fd) != 0)
	{
		printf("bq40_get_devicetype() : could not get DeviceType\n");
		return 1;
	}

#ifdef ENABLE_DEBUG
	printf("    block -> ");
	smbus_print_block(data);
#endif

	if (sbs_block_check_mac(command, data, 2) != 0)
	{
		printf("bq40_get_devicetype() : bad MAC command\n");
		return 1;
	}

	dev->type = smbus_block_LE_to_ui32(data + 2, 2);

	return 0;
}


int bq40_get_firmware_v(struct firmware_version* fw, int fd)
{
	__u8 command[2] = {0x02, 0x00}; // LITTLE_ENDIAN
	__u8 data[32] = {}; // zero-init
	if (sbs_exec_block_command(0x44, command, data, 2, fd) != 0)
	{
		printf("bq40_get_firmware_v() : could not get FirmwareVersion\n");
		return 1;
	}

#ifdef ENABLE_DEBUG
	printf("    block -> ");
	smbus_print_block(data);
#endif

	if (sbs_block_check_mac(command, data, 2) != 0)
	{
		printf("bq40_get_firmware_v() : bad MAC command\n");
		return 1;
	}

	smbus_block_LE_to_u8s(data + 2, 11, (__u8*)fw->fw);
	fw->fw[11] = '\0';

	return 0;
}


int bq40_get_operation_status(struct operation_status* status, int fd)
{
	__u8 command[2] = {0x54, 0x00};
	__u8 data[32] = {};
	if (sbs_exec_block_command(0x44, command, data, 2, fd) != 0)
	{
		printf("bq40_get_operation_status() : could not get OperationStatus\n");
		return 1;
	}

#ifdef ENABLE_DEBUG
	printf("    block -> ");
	smbus_print_block(data);
#endif

	if (sbs_block_check_mac(command, data, 2) != 0)
	{
		printf("bq40_get_operation_status() : bad MAC command\n");
		return 1;
	}

	uint32_t flags = *(uint32_t*)data;
	status->shutdown = ( ((flags >> 29) & 1) == 1 ? SHUTDN_EMERGENCY :
			( ((flags >> 10) & 1) == 1 ? SHUTDN_LOW_VOLTAGE :
			 ( ((flags >> 16) & 1) == 1 ? SHUTDN_MANUAL :
			  SHUTDN_NONE)));

	status->pf = ( (flags >> 12) & 1) == 1 ? PF_FAIL : PF_NONE;
	status->fuse = ( (flags >> 5) & 1) == 1 ? FUSE_DEPLOY : FUSE_NONE;
	return 0;
}

#endif
