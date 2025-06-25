# sbsutil

Smart Battery System utility for monitoring and setting smart battery flags. This tool supports controller-specific `ManufacturerAccess` commands

## Compilation

This project requires userspace Linux headers (`linux-api-headers`, aka `linux-libc-dev`) and i2c headers, which can be provided by `i2c-tools` package. I2C support can be disabled by running `ENABLE_I2C=0 make ...`.

`make debug`

### Kernel module

To build the experimental kernel module, run `make kmod` to build, `sudo make install` to install and `sudo make load` to load the module. Right now it only tries to find the SBS chip address on the EC according to the ACPI specification and dumps this info to dmesg. 

## Usage

```
Usage: sbsutil [OPTION]... [-f|--file=FILE] [COMMAND]
Communicate with the Smart Battery System IC.

Runs preflight checks if COMMAND is not given.
If the FILE is not specified, communicates over the ACPI bus using the sbsctl module.
NOTE: i2c commands can potentially cause harm when sent to a wrong device!

  -h, --help     	print this help message and exit
  -v, --verbose  	print out all data
  -f, --file=FILE	communicate over i2c device located in FILE
  -c, --chip=CHIP	override SBS controller model. CHIP is one of: [bq40,auto]

Commands:
  preflight      	Run non-destructive checks using standard SBS commands
  status         	Fetch device-specific status registers

Examples:
  sbsutil preflight    	Run preflight checks without executing ManufacturerAccess commands. Requires loaded sbsctl kernel module to perform ACPI calls.
  sbsutil -f /dev/i2c-2	Run preflight checks over the second i2c device.
```

Right now, this utility attempts to dump battery information, early WIP

## Supported chips

- bq40zxy family : [datasheet](https://www.ti.com/lit/ds/symlink/bq40z50.pdf), [protocol](https://www.ti.com/lit/ug/sluua43a/sluua43a.pdf)
