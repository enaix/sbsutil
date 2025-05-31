#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/acpi.h>
#include <acpi/acpi.h>


MODULE_AUTHOR("enaix")
MODULE_DESCRIPTION("Advanced Smart Battery System interface")
MODULE_LICENSE("GPL")

// the device may appear with the following ids
char* acpi_sbs_device_hid[] = {"ACPI0001", "ACPI0005", ""}; // "" is an end of array marker


// Forward defs
static int acpi_sbs_add(struct acpi_device *device);
static void acpi_sbs_remove(struct acpi_device *device);

static struct acpi_driver acpi_sbsctl_driver = {
	.name = "sbsctl_driver",
	.class = ACPI_SMB_HC_CLASS,
	.ids = acpi_sbs_device_hid,
	.ops = {
		.add = acpi_sbs_add,
		.remove = acpi_sbs_remove,
	},
};


int probe_acpi_device()
{
	unsigned long long val;
	int device_hid = -1;
	int device_num = 0;

	int i = 0;
	while(*acpi_sbs_device_hid + i != "")
	{
		// linux/include/acpi/acpi_bus.h
		for_each_acpi_dev_match(dev, acpi_sbs_device_hid[i], NULL, -1)
		{
			// Each match appears here
		}
		// dev is now set
		if (!dev)
		{
			printf("probe_acpi_device() : could not find SBS controller : %s\n", strerror(ENODEV));
			return -1;
		}

		// Now we need to fetch _EC
		// linux/drivers/acpi/utils.c
		acpi_status status = AE_OK;
		union acpi_object element;
		struct acpi_buffer buffer = {0, NULL};
		buffer.length = sizeof(union acpi_object);
		buffer.pointer = &element;

		status = acpi_evaluate_object(dev->handle, "_EC", NULL, &buffer);
		if (ACPI_FAILURE(status))
		{
			// damn this is bad
			printf("probe_acpi_device() : could not evaluate object %s\n", acpi_sbs_device_hid[i]);
		}
		else
		{
			// ok
			if (element.type != ACPI_TYPE_INTEGER)
			{
				printf("probe_acpi_device() : expected int, got a different type\n");
			}
			else
			{
				// found

				printf("probe_acpi_device() : match %d\n", i);
				printf("  device hid : %s\n, _EC : %u\n", acpi_sbs_device_hid[i], val);
				device_hid = i;
				device_num++;
			}
		}

		// Need to put the device
		acpi_dev_put(dev);

		i++;
	}

	if (device_num > 1)
	{
		printf("probe_acpi_device() : multiple devices found, aborting...\n");
		return -1;
	}

	if (device_num == 0)
	{
		printf("probe_acpi_device() : no device found\n");
		return -1;
	}

	// linux/drivers/acpi/sbshc.c
	c->dev.offset = (val >> 8) & 0xff;
	c->dev.hid_index = device_hid;
	printf("found device !!");
	printf("  hid : %s, offset: %.2x\n", acpi_sbs_device_hid[i], c->dev.offset);
	return 1;
}
