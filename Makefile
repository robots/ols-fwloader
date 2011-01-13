CC = gcc
CFLAGS = -g -O0 -I/usr/include/libusb-1.0
LDFLAGS = -lusb-1.0
APP = ols-fwloader
OBJS = main.o ols-boot.o data_file.o

all: $(APP)

$(APP):  $(OBJS)
	$(CC) $(CFLAGS) -o $(APP) $(OBJS) $(LFD_OBJS) $(LDFLAGS)

%.o:	%.c
	$(CC) $(CFLAGS) $(DEFS) -c $<


clean:
	rm -f $(APP) *.o

