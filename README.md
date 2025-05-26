# sbsutil

Smart Battery System utility for monitoring and setting smart battery flags. This tool supports controller-specific `ManufacturerAccess` commands

## Compilation

This project requires userspace Linux headers (`linux-api-headers`, aka `linux-libc-dev`) and i2c headers, which can be provided by `i2c-tools` package. I2C support can be disabled by running `ENABLE_I2C=0 make ...`.

`make debug`

## Usage

`sudo ./build/sbsutil path-to-i2c-device`

Right now, this utility attempts to dump battery information, early WIP

## Supported chips

- bq40zxy family : [datasheet](https://www.ti.com/lit/ds/symlink/bq40z50.pdf), [protocol](https://www.ti.com/lit/ug/sluua43a/sluua43a.pdf)
