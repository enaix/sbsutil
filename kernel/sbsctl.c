#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/acpi.h>
#include <acpi/acpi.h>

// Enable prints
#define ENABLE_DEBUG


MODULE_AUTHOR("enaix")
MODULE_DESCRIPTION("Advanced Smart Battery System interface")
MODULE_LICENSE("GPL")

/*
 * We are not declaring a device driver
 *
 
// the device may appear with the following ids
char* acpi_sbs_device_hid[] = {"ACPI0001", "ACPI0005", ""}; // "" is an end of array marker

MODULE_DEVICE_TABLE(acpi_sbs_device_hid);

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
*/


struct sbs_device
{
	int hid_index;
	unsigned char offset;
};

static sbs_device dev;


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
			printk(KERN_WARNING "probe_acpi_device() : could not find SBS controller : %s\n", strerror(ENODEV));
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
			printk(KERN_WARNING "probe_acpi_device() : could not evaluate object %s\n", acpi_sbs_device_hid[i]);
		}
		else
		{
			// ok
			if (element.type != ACPI_TYPE_INTEGER)
			{
				printk(KERN_WARNING "probe_acpi_device() : expected int, got a different type\n");
			}
			else
			{
				// found

				printk(KERN_INFO "probe_acpi_device() : match %d\n", i);
				printk(KERN_INFO "  device hid : %s\n, _EC : %u\n", acpi_sbs_device_hid[i], val);
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
		printk(KERN_WARNING "probe_acpi_device() : multiple devices found, aborting...\n");
		return -1;
	}

	if (device_num == 0)
	{
		printk(KERN_WARNING "probe_acpi_device() : no device found\n");
		return -1;
	}

	// linux/drivers/acpi/sbshc.c
	dev.offset = (val >> 8) & 0xff;
	dev.hid_index = device_hid;
	printk(KERN_INFO "found device !!");
	printk(KERN_INFO "  hid : %s, offset: %.2x\n", acpi_sbs_device_hid[i], c->dev.offset);
	return 1;
}

static int __init init_sbs_interface(void)
{
	if (probe_acpi_device() < 0)
	{
		// ignore
	}
	return 0;
}


static void __exit unload_sbs_interface(void)
{
	// nothing to do
}

module_init(init_sbs_interface);
module_exit(unload_sbs_interface);

