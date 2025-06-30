# sbsutil

Smart Battery System utility for monitoring and setting smart battery flags. This tool supports controller-specific `ManufacturerAccess` commands

## Compilation

This project requires userspace Linux headers (`linux-api-headers`, aka `linux-libc-dev`) and i2c headers, which can be provided by `i2c-tools` or `libi2c-dev` package. I2C support can be disabled by running `ENABLE_I2C=0 make ...`.

`make debug`

You may also enable RPI GPIO support for running voltage glitching exploits by setting `GPIO_RPI=1 make ...`. This flag requires the `libgpiod` library.

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
  key KEY        	Elevate priviledges with a KEY, which should be specified as AAaaBBbb or 0xAAaaBBbb
  brute START END	Bruteforce keys in an optional range [START, END]
  hack U_KEY FA_KEY	Execute password override exploit, which overwrites unseal and full access keys with new ones
  flash          	Dump contents of the flash memory

Examples:
  sbsutil preflight    	Run preflight checks without executing ManufacturerAccess commands. Requires loaded sbsctl kernel module to perform ACPI calls.
  sbsutil -f /dev/i2c-2	Run preflight checks over the second i2c device.
```

Right now, this utility attempts to dump battery information, early WIP

## Supported chips

- bq40zxy family : [datasheet](https://www.ti.com/lit/ds/symlink/bq40z50.pdf), [protocol](https://www.ti.com/lit/ug/sluua43a/sluua43a.pdf)

## Connect using the ch341 programmer

If the sbs device is not present on the i2c bus and you cannot access the EC chip, you may use a generic ch341 programmer for communication. [ch341-i2c-spi-gpio](https://github.com/frank-zago/ch341-i2c-spi-gpio) kernel driver exposes i2c pins as a generic `/dev/i2c-*` device, so it can be used natively by the sbsutil. A generic `ch341 pro` programmer has the following pins exposed:

```
 [pcb top]
   (USB)

 .  ...  .
 +---+---+
>|SDA|   |
 +---+---+
>|SCL|   |
 +---+---+
>|GND|   |
 +---+---+
 |3V3|   |
 +---+---+
 +=====+||
        ||
 (END)  OO
```
