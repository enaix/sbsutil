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
// ===============================


int bq40_get_chemid(struct chem_id* chem, int fd)
{
	__u8 data[32] = {0x06, 0x00}; // LITTLE_ENDIAN

	// Set data to 0x0006 (ChemID) in LE
	__s32 res = i2c_smbus_write_block_data(fd, 0x44, 2, data);

	if (res < 0)
	{
		printf("bq40_get_chemid() : could not request ChemID data\n");
		return 1;
	}

	res = i2c_smbus_read_block_data(fd, 0x44, data);
	if (res < 0)
	{
		printf("bq40_get_chemid() : could not read ChemID data\n");
		return 1;
	}
#ifdef ENABLE_DEBUG
	printf("  block -> ");
	smbus_print_block(data);
#endif

	chem->mac[0] = data[0];
	chem->mac[1] = data[1];
	chem->id = smbus_block_LE_to_ui32(data + 2, 2);

	// Verify ChemID
	if (chem->mac[0] != 0x06 || chem->mac[1] != 0x00)
	{
		printf("bq40_get_chemid() : bad MAC command : expected 06 00, got %.2x %.2x", chem->mac[0], chem->mac[1]);
		return 1;
	}

	return 0;
}

int bq40_get_devicetype(struct device_type* dev, int fd)
{
	__u8 data[32] = {0x01, 0x00}; // LITTLE_ENDIAN

	// Set data to 0x0001 (DeviceType) in LE
	__s32 res = i2c_smbus_write_block_data(fd, 0x44, 2, data);

	if (res < 0)
	{
		printf("bq40_get_devicetype() : could not request DeviceType\n");
		return 1;
	}

	res = i2c_smbus_read_block_data(fd, 0x44, data);
	if (res < 0)
	{
		printf("bq40_get_chemid() : could not read DeviceType response\n");
		return 1;
	}

#ifdef ENABLE_DEBUG
	printf("block -> ");
	smbus_print_block(data);
#endif

	// LITTLE_ENDIAN
	dev->mac[0] = data[0];
	dev->mac[1] = data[1];
	dev->type = smbus_block_LE_to_ui32(data + 2, 2);

	if (dev->mac[0] != 0x01 || dev->mac[1] != 0x00)
	{
		printf("bq40_get_devicetype() : bad MAC command : expected 06 00, got %.2x %.2x", dev->mac[0], dev->mac[1]);
		return 1;
	}

	return 0;
}


int bq40_get_firmware_v(struct firmware_version* fw, int fd)
{
	__u8 data[32] = {0x02, 0x00}; // LITTLE_ENDIAN

	// Set data to 0x0002 (DeviceType) in LE
	__s32 res = i2c_smbus_write_block_data(fd, 0x44, 2, data);

	if (res < 0)
	{
		printf("bq40_get_firmware_v() : could not request FirmwareVersion\n");
		return 1;
	}

	res = i2c_smbus_read_block_data(fd, 0x44, data);
	if (res < 0)
	{
		printf("bq40_get_chemid() : could not read FirmwareVersion response\n");
		return 1;
	}

#ifdef ENABLE_DEBUG
	printf("block -> ");
	smbus_print_block(data);
#endif

	// LITTLE_ENDIAN
	fw->mac[0] = data[0];
	fw->mac[1] = data[1];
	smbus_block_LE_to_u8s(data + 2, 11, fw->fw);
	fw->fw[11] = '\0';

	if (fw->mac[0] != 0x02 || fw->mac[1] != 0x00)
	{
		printf("bq40_get_firmware_v() : bad MAC command : expected 06 00, got %.2x %.2x", fw->mac[0], fw->mac[1]);
		return 1;
	}

	return 0;
}


#endif
