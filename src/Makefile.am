bin_PROGRAMS = ols_fwloader

ols_fwloader_SOURCES = boot_if.h data_file.c data_file.h main.c ols-boot.c ols-boot.h ols.c ols.h serial.c serial.h

ols_fwloader_CFLAGS = @libusb_CFLAGS@
ols_fwloader_LDADD = @libusb_LIBS@ @win32_LIBS@

