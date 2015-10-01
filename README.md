#  OLS-fwloader

This is commandline update utility for OLS - OpenBench Logic Sniffer. Logic Analyzer created by Dangerous Prototypes (http://dangerousprototypes.com).

This tool uses parts from Diolan's fw_update tool to talk to the bootloader.  Serial port routines first written by Piotr Pawluczuk for pirate-loader, adapted and improved to better fit this project.

So far supported input file types are:
* Binary
* Intel Hex
* Xilinx bit file

All input files are autodetected.

## Requirements:

* linux:
  *  cdc_acm support in kernel
  *  libusb 1.0 (install libusb-1.0.0-dev)
  *  autoconf (to build from the GIT repository)
* Windows:
  *  cdc acm driver

## Building:

```
From git repository first run:
$ autoreconf --install

$ ./configure
$ make
$ make install
```

I suggest to use `--prefix` parameter for configure.

Git repository can be found here:

  https://github.com/robots/ols-fwloader

This tool is created and maintained by Michal Demin <michal.demin@gmail.com>
Feel free to send suggestions, improvements, patches.

## Usage Examples

To use BOOT mode (reading/writing the PIC), you will first need to press RESET while the PGC and PGD terminals are jumpered. The ACT and PWR LEDs will be lit. Note that this is different from resetting while pressing the UPDATE button, which is for FPGA firmware update.

In BOOT mode, the device will identify itself (see it with `lsusb` on linux) as 04d8:fc90 whereas in warm (sniffer operating) mode, the device will be 04d8:fc92.

You may need to run ols-fwloader as root (`sudo ols-fwloader ...`) to use the USB device directly in BOOT mode.

Read current OLS PIC firmware into a file:

```
ols-fwloader -f BOOT -t HEX -R -r current-pic-firmware.hex
```

Write PIC firmware with verification, enter bootloader first:

```
ols-fwloader -f BOOT -n -P /dev/ttyACM0 -V -W -w new-firmware.hex
```

Write FPGA bitstream (HEX):

```
ols-fwloader -f APP -P /dev/ttyACM0 -W -w bitstream.mcs
```

Write FPGA bitstream (BIN):

```
ols-fwloader -f APP -P /dev/ttyACM0 -W -w bitstream.bit -t BIN
```
