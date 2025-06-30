#ifndef SBS_VOLTAGE_CTRL
#define SBS_VOLTAGE_CTRL

#ifdef SBS_RPI

// RPI voltage contol
// ==================

#include <gpiod.h>
#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct gpiod_chip* gpio_chip = NULL;
struct gpiod_line* gpio_pin_voltage = NULL;


int rpi_gpio_init(int pin)
{
	gpio_chip = gpiod_chip_open("/dev/gpiochip0");
	if (!gpio_chip)
	{
		printf("rpi_gpio_init() : could not open /dev/gpiochip0\n");
		return 0;
	}

	gpio_pin_voltage = gpiod_chip_get_line(gpio_chip, pin);
	if (!gpio_pin_voltage)
	{
		printf("rpi_gpio_init() : could not get pin %d\n", pin);
		gpiod_chip_close(gpio_chip);
		gpio_chip = NULL;
		return 0;
	}

	int ret = gpiod_line_request_output(gpio_pin_voltage, "sbsutil", 0);
	if (ret < 0)
	{
		printf("rpi_gpio_init() : could not set pin %d to OUTPUT\n", pin);
		gpiod_line_release(gpio_pin_voltage);
		gpiod_chip_close(gpio_chip);
		gpio_chip = NULL;
		return 0;
	}

	return 1;
}


void rpi_gpio_close()
{
	if (gpio_chip)
	{
		gpiod_line_release(gpio_pin_voltage);
		gpiod_chip_close(gpio_chip);
		gpio_chip = NULL;
	}
}


int rpi_gpio_set(int voltage)
{
	int ret = gpiod_line_set_value(gpio_pin_voltage, voltage);
	if (ret < 0)
	{
		const char hi[] = "HIGH";
		const char lo[] = "LOW";
		printf("rpi_gpio_init() : could not set voltage to %s\n", voltage ? hi : lo);
		rpi_gpio_close();
		return 0;
	}
	return 1;
}


#else

// No other backend 

#endif // End of internal function definitions


// Voltage ctrl api
// ================


static const char* voltage_ctrl_modes[] = {
	"RPI GPIO", // 0
	"NONE",
};



int voltage_ctrl_supported()
{
#ifdef SBS_RPI
	return 1;
#else
	return 0;
#endif
}

const char* voltage_ctrl_mode()
{
#ifdef SBS_RPI
	return voltage_ctrl_modes[0];
#else
	return voltage_ctrl_modes[1];
#endif
}


int voltage_ctrl_init(int pin)
{
#ifdef SBS_RPI
	return rpi_gpio_init(pin);
#else
	return 0;
#endif
}


int voltage_ctrl_set(int voltage)
{
#ifdef SBS_RPI
	return rpi_gpio_set(voltage);
#else
	return 0;
#endif
}


void voltage_ctrl_close()
{
#ifdef SBS_RPI
	rpi_gpio_close();
#else

#endif
}


#endif
