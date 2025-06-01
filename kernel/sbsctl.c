#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/acpi.h>
#include <linux/errno.h>
#include <acpi/acpi.h>


// Enable prints
#define ENABLE_DEBUG


MODULE_AUTHOR("enaix");
MODULE_DESCRIPTION("Advanced Smart Battery System interface");
MODULE_LICENSE("GPL");

// the device may appear with the following ids
char* acpi_sbs_device_hid[] = {"ACPI0001", "ACPI0005", "PNP0C09", "PNP0C0A", ""}; // "" is an end of array marker
// not needed: ec.h already does this for us


// EC declarations
// ===============


/* linux/drivers/acpi/internal.h
 * ~~ Is not supposed to be used in drivers, but this is only used for debugging purposes ~~
 */

enum acpi_ec_event_state {
	EC_EVENT_READY = 0,	/* Event work can be submitted */
	EC_EVENT_IN_PROGRESS,	/* Event work is pending or being processed */
	EC_EVENT_COMPLETE,	/* Event work processing has completed */
};

struct acpi_ec {
	acpi_handle handle;
	int gpe;
	int irq;
	unsigned long command_addr;
	unsigned long data_addr;
	bool global_lock;
	unsigned long flags;
	unsigned long reference_count;
	struct mutex mutex;
	wait_queue_head_t wait;
	struct list_head list;
	struct transaction *curr;
	spinlock_t lock;
	struct work_struct work;
	unsigned long timestamp;
	enum acpi_ec_event_state event_state;
	unsigned int events_to_process;
	unsigned int events_in_progress;
	unsigned int queries_in_progress;
	bool busy_polling;
	unsigned int polling_guard;
};


extern struct acpi_ec* first_ec;



// ==============

struct sbs_device
{
	int hid_index;
	unsigned char offset;
};

static struct sbs_device dev;



static int find_ec(void)
{
	// We suppose that the ec.h driver is loaded
	if (first_ec == NULL)
	{
		printk(KERN_ERR "sbsctl : find_ec() : embedded controller not loaded by linux/drivers/acpi/ec.h\n");
		return -1;
	}

	dev.offset = 0; // Do not set offset
	printk(KERN_INFO "sbsctl : find_ec() : ec found !!   cmd addr: 0x%lx, data addr: 0x%lx\n", first_ec->command_addr, first_ec->data_addr);
	return 1;
}



static int probe_acpi_device(void)
{
	unsigned long long val;
	int device_hid = -1;
	int device_num = 0;
	struct acpi_device* acpi_dev;

	int i = 0;
	while(acpi_sbs_device_hid[i][0] != '\0')
	{
		// linux/include/acpi/acpi_bus.h
		for_each_acpi_dev_match(acpi_dev, acpi_sbs_device_hid[i], NULL, -1)
		{
			// Each match appears here
			// dev is now set
			if (!acpi_dev)
			{
				printk(KERN_WARNING "probe_acpi_device() : could not find SBS controller : ENODEV\n");
				return -1;
			}

			// Now we need to fetch _EC
			// linux/drivers/acpi/utils.c
			/*acpi_status status = AE_OK;
			union acpi_object element;
			struct acpi_buffer buffer = {0, NULL};
			buffer.length = sizeof(union acpi_object);
			buffer.pointer = &element;

			status = acpi_evaluate_object(acpi_dev->handle, "_EC", NULL, &buffer);
			*/
			acpi_status status = acpi_evaluate_integer(acpi_dev->handle, "_EC", NULL, &val);
			if (ACPI_FAILURE(status))
			{
				// damn this is bad
				printk(KERN_WARNING "probe_acpi_device() : could not evaluate object %s\n", acpi_sbs_device_hid[i]);
			}
			else
			{
				// ok
				/*if (element.type != ACPI_TYPE_INTEGER)
				{
					printk(KERN_WARNING "probe_acpi_device() : expected int, got a different type\n");
				}
				else
				{*/
					// found

					printk(KERN_INFO "probe_acpi_device() : match %d\n", i);
					printk(KERN_INFO "  device hid : %s\n, _EC : %u\n", acpi_sbs_device_hid[i], val);
					device_hid = i;
					device_num++;
				//}
			}
		}

		// Need to put the device
		acpi_dev_put(acpi_dev);

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
	printk(KERN_INFO "  hid : %s, offset: %.2x\n", acpi_sbs_device_hid[i], dev.offset);
	return 1;
}

static int __init init_sbs_interface(void)
{
	// Print debug info about ec
	find_ec(); // ignore status

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

