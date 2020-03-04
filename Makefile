TARGET = ./bin/lmsmonitor
# curl used for SSE impl.
LIBS = -lasound -lpthread -lcurl -lrt -L./lib -lwiringPi -lArduiPi_OLED
LIBSTATIC = -lasound -lpthread -lcurl -lrt -L./lib -lwiringPi_static -lArduiPi_OLED_static
CC = g++
CFLAGS = -g -Wall -Ofast -lrt -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv7-a -mtune=cortex-a7 -ffast-math -pipe -I. -O3

bin:
	mkdir bin

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

digest-sse.c: digest-sse.flex
	flex -I -o $@ $<
