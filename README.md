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
