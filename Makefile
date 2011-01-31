
CROSS =

CC = $(CROSS)gcc
CFLAGS =
LDFLAGS =
APP = ols-fwloader
OBJS = main.o ols-boot.o data_file.o serial.o ols.o

ifeq ($(CROSS), mingw32-)
CFLAGS += -DWIN32
LDFLAGS += -lhid -lsetupapi
APP = ols-fwloader.exe
else
LDFLAGS += -lusb-1.0
CFLAGS += -I/usr/include/libusb-1.0
APP = ols-fwloader
endif


all: $(APP)

$(APP):  $(OBJS)
	$(CC) $(CFLAGS) -o $(APP) $(OBJS) $(LFD_OBJS) $(LDFLAGS)

%.o:	%.c
	$(CC) $(CFLAGS) $(DEFS) -c $<


clean:
	rm -f $(APP) *.o

