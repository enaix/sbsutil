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


int bq40_get_chemid(struct chem_id* chem, int fd, struct args* config)
{
	__u8 command[2] = {0x06, 0x00}; // LITTLE_ENDIAN
	__u8 data[32] = {}; // zero-init
	if (sbs_exec_block_command(0x44, command, data, 2, fd, 31) != 0)
	{
		printf("bq40_get_chemid() : could not get ChemID data\n");
		return 1;
	}

	if (config->verbose)
	{
		printf("    block -> ");
		smbus_print_block(data);
	}

	if (sbs_block_check_mac(command, data, 2) != 0)
	{
		printf("bq40_get_chemid() : bad MAC command\n");
		return 1;
	}

	chem->id = smbus_block_LE_to_ui32(data + 2, 2);

	return 0;
}


int bq40_get_devicetype(struct device_type* dev, int fd, struct args* config)
{
	__u8 command[2] = {0x01, 0x00}; // LITTLE_ENDIAN
	__u8 data[32] = {}; // zero-init
	if (sbs_exec_block_command(0x44, command, data, 2, fd, 31) != 0)
	{
		printf("bq40_get_devicetype() : could not get DeviceType\n");
		return 1;
	}

	if (config->verbose)
	{
		printf("    block -> ");
		smbus_print_block(data);
	}

	if (sbs_block_check_mac(command, data, 2) != 0)
	{
		printf("bq40_get_devicetype() : bad MAC command\n");
		return 1;
	}

	dev->type = smbus_block_LE_to_ui32(data + 2, 2);

	return 0;
}


int bq40_get_firmware_v(struct firmware_version* fw, int fd, struct args* config)
{
	__u8 command[2] = {0x02, 0x00}; // LITTLE_ENDIAN
	__u8 data[32] = {}; // zero-init
	if (sbs_exec_block_command(0x44, command, data, 2, fd, 31) != 0)
	{
		printf("bq40_get_firmware_v() : could not get FirmwareVersion\n");
		return 1;
	}

	if (config->verbose)
	{
		printf("    block -> ");
		smbus_print_block(data);
	}

	if (sbs_block_check_mac(command, data, 2) != 0)
	{
		printf("bq40_get_firmware_v() : bad MAC command\n");
		return 1;
	}

	smbus_block_LE_to_u8s(data + 2, 11, (__u8*)fw->fw);
	fw->fw[11] = '\0';

	return 0;
}


int bq40_get_operation_status(struct operation_status* status, int fd, struct args* config)
{
	__u8 command[2] = {0x54, 0x00};
	__u8 data[32] = {};
	if (sbs_exec_block_command(0x44, command, data, 2, fd, 31) != 0)
	{
		printf("bq40_get_operation_status() : could not get OperationStatus\n");
		return 1;
	}

	if (config->verbose)
	{
		printf("    block -> ");
		smbus_print_block(data);
	}

	if (sbs_block_check_mac(command, data, 2) != 0)
	{
		printf("bq40_get_operation_status() : bad MAC command\n");
		return 1;
	}

	//uint32_t flags = *(uint32_t*)(data + 2); // easier to do flags & (1 << bits);
	uint32_t flags = smbus_block_LE_to_ui32(data + 2, 4);
	status->shutdown = ( ((flags >> 29) & 1) == 1 ? SHUTDN_EMERGENCY :
			( ((flags >> 10) & 1) == 1 ? SHUTDN_LOW_VOLTAGE :
			 ( ((flags >> 16) & 1) == 1 ? SHUTDN_MANUAL :
			  SHUTDN_NONE)));

	status->pf = ( (flags >> 12) & 1) == 1 ? PF_FAIL : PF_NONE;
	status->fuse = ( (flags >> 5) & 1) == 1 ? FUSE_DEPLOY : FUSE_NONE;
	int bit9 = (flags >> 9) & 1, bit8 = (flags >> 8) & 1;
	status->access = (bit9 & bit8) ? ACCESS_SEALED : (bit9 ? ACCESS_UNSEALED : (bit8 ? ACCESS_FULL : ACCESS_ERR));

	if (status->access == ACCESS_ERR)
	{
		printf("bq40_get_operation_status() : bad status code\n");
		return 1;
	}
	return 0;
}


struct bq40_pf_status
{
	int thermistors_fail[4]; // TS1-TS4 fail
	int data_flash_wearout;  // DFW
	int open_cell_conn_fail; // OPNCELL
	int flash_checksum_fail; // IFC
	int ptc_fail;
	int second_lvl_protec_fail;
	int afe_conn_fail;
	int afe_register_fail;
	int chem_fuse_fail;
	int dis_fet_fail;
	int chg_fet_fail;
	int vol_imbal_act_fail;
	int vol_imbal_rest_fail;
	int cap_degradation;
	int impedance_fail;
	int cell_bal_fail;
	int qmax_imbal_fail;
	int overtemp_fet_fail;

	int overtemp_cell_fail;
	int overcurr_discharge;
	int overcurr_charge;
	int overvolt_fail;
	int undervolt_fail;
};


void bq40_log_pf_status(const struct bq40_pf_status* status, void (*logger)(const char*))
{
	for (int i = 0; i < 4; i++)
	{
		if (status->thermistors_fail[i])
		{
			char desc[32];
			snprintf(desc, 32, "Open Thermistor-TS%d Failure", i+1);
			logger(desc);
		}
	}
	if (status->data_flash_wearout)
		logger("Data Flash Wearout Failure");
	if (status->open_cell_conn_fail)
		logger("Open Cell Tab Connection Failure");
	if (status->flash_checksum_fail)
		logger("Instruction Flash Checksum Failure");
	if (status->ptc_fail)
		logger("PTC (Positive Temperature Coefficient) resistor Failure");
	if (status->second_lvl_protec_fail)
		logger("Second Level Protector Failure");
	if (status->afe_conn_fail)
		logger("AFE (Analog Front End) Communication Failure");
	if (status->afe_register_fail)
		logger("AFE (Analog Front End) Register Failure");
	if (status->chem_fuse_fail)
		logger("Chemical Fuse Failure");
	if (status->dis_fet_fail)
		logger("Discharge FET Failure");
	if (status->chg_fet_fail)
		logger("Charge FET Failure");
	if (status->vol_imbal_act_fail)
		logger("Voltage Imbalance while Pack is Active Failure");
	if (status->vol_imbal_rest_fail)
		logger("Voltage Imbalance while Pack At Rest Failure");
	if (status->cap_degradation)
		logger("Capacity Degradation Failure");
	if (status->impedance_fail)
		logger("Impedance Failure");
	if (status->cell_bal_fail)
		logger("Cell Balancing Failure");
	if (status->qmax_imbal_fail)
		logger("QMax Imbalance Failure");
	if (status->overtemp_fet_fail)
		logger("Safety Overtemperature FET Failure");

	if (status->overtemp_cell_fail)
		logger("Safety Overtemperature Cell Failure");
	if (status->overcurr_discharge)
		logger("Safety Overcurrent in Discharge");
	if (status->overcurr_charge)
		logger("Safety Overcurrent in Charge");
	if (status->overvolt_fail)
		logger("Safety Cell Overvoltage Failure");
	if (status->undervolt_fail)
		logger("Safety Cell Overvoltage Failure");
}


int bq40_get_pf_status(struct bq40_pf_status* status, int* ok, int fd, struct args* config)
{
	__u8 command[2] = {0x53, 0x00};
	__u8 data[32] = {};
	if (sbs_exec_block_command(0x44, command, data, 2, fd, 31) != 0)
	{
		printf("bq40_get_pf_status() : could not get PFStatus\n");
		return 1;
	}

	if (config->verbose)
	{
		printf("    block -> ");
		smbus_print_block(data);
	}

	if (sbs_block_check_mac(command, data, 2) != 0)
	{
		printf("bq40_get_pf_status() : bad MAC command\n");
		return 1;
	}

	//uint32_t flags = *(uint32_t*)(data + 2); // easier to do flags & (1 << bits);
	uint32_t flags = smbus_block_LE_to_ui32(data + 2, 4);
	for (int bit = 28; bit <= 31; bit++)
		status->thermistors_fail[bit - 28] = (flags >> bit) & 1;

	status->data_flash_wearout = (flags >> 26) & 1;
	status->open_cell_conn_fail = (flags >> 25) & 1;
	status->flash_checksum_fail = (flags >> 24) & 1;
	status->ptc_fail = (flags >> 23) & 1;
	status->second_lvl_protec_fail = (flags >> 22) & 1;
	status->afe_conn_fail = (flags >> 21) & 1;
	status->afe_register_fail = (flags >> 20) & 1;
	status->chem_fuse_fail = (flags >> 19) & 1;
	status->dis_fet_fail = (flags >> 17) & 1;
	status->chg_fet_fail = (flags >> 16) & 1;
	status->vol_imbal_act_fail = (flags >> 12) & 1;
	status->vol_imbal_rest_fail = (flags >> 11) & 1;
	status->cap_degradation = (flags >> 10) & 1;
	status->impedance_fail = (flags >> 9) & 1;
	status->cell_bal_fail = (flags >> 8) & 1;
	status->qmax_imbal_fail = (flags >> 7) & 1;
	status->overtemp_fet_fail = (flags >> 6) & 1;

	status->overtemp_cell_fail = (flags >> 4) & 1;
	status->overcurr_discharge = (flags >> 3) & 1;
	status->overcurr_charge = (flags >> 2) & 1;
	status->overvolt_fail = (flags >> 1) & 1;
	status->undervolt_fail = flags & 1;

	*ok = (flags == 0);
	return 0;
}

struct bq40_lifetime_block
{
	char blocks[5][32];
};


int bq40_get_lifetime_data(struct bq40_lifetime_block* blocks, int fd, struct args* config)
{
	for (int i = 0; i < 5; i++)
	{
		__u8 command[2] = {0x60 + i, 0x00};
		__u8 data[64] = {};

		if (sbs_exec_block_command(0x44, command, data, 2, fd, 32) != 0)
		{
			printf("bq40_get_lifetime_data() : could not get Lifetime Data Block %d\n", i+1);
			return 1;
		}

		if (config->verbose)
		{
			printf("    block %d -> ", i+1);
			smbus_print_block(data);
		}

		if (sbs_block_check_mac(command, data, 2) != 0)
		{
			printf("bq40_get_lifetime_data() : bad MAC command\n");
			return 1;
		}

		memcpy(blocks->blocks[i], data, 32);
	}
	return 0;
}


int bq40_unlock_priviledges(uint32_t key, int fd, struct args* config)
{
	// Split the key in two parts
	__u8* key8 = (__u8*)(&key);
	__u8 key8_le[] = {key8[2], key8[3], key8[0], key8[1]}; // LITTLE ENDIAN
	//__u8 key8_le[] = {key8[3], key8[2], key8[1], key8[0]};
	__u16* key16 = (__u16*)(key8_le);

	__u16 key_lhs = key16[0];
	__u16 key_rhs = key16[1];

	if (config->verbose)
	{
		printf("    word -> ");
		smbus_print_block_l(key8_le, 4);
	}

	if (sbs_write_word(fd, 0x00, key_lhs) != 0)
	{
		printf("bq40_unlock_priviledges() : could not write first word of the key\n");
		return 1;
	}

	//usleep(1000*1);

	if (sbs_write_word(fd, 0x00, key_rhs) != 0)
	{
		printf("bq40_unlock_priviledges() : could not write second word of the key\n");
		return 1;
	}

	return 0;
}


#endif
