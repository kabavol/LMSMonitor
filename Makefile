HOSTTYPE:=$(shell uname -m)
ifeq ($(HOSTTYPE),armv6l)
    LIBS = -lasound -lpthread -L./lib -lwiringPi_static -lArduiPi_OLED_static
    CFLAGS = -g -Wall -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -I.
else
    LIBS = -lasound -lpthread
    CFLAGS = -g -Wall -I.
endif

TARGET = ./bin/lmsmonitor
CC = g++

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
