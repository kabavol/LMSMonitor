TARGET = ./bin/lmsmonitor
# curl used for SSE impl.
#LIBS =  -lpthread -lcurl -lrt -L./lib -lwiringPi -lArduiPi_OLED
LIBS =  -lpthread -lrt -L./lib -lwiringPi -lArduiPi_OLED
LIBSTATIC =  -lpthread -lcurl -lrt -L./lib -lwiringPi_static -lArduiPi_OLED_static
CC = g++
CFLAGS4 = -g -Wall -Ofast -lrt -mfpu=neon-vfpv4    -mfloat-abi=hard -march=armv7-a -mtune=cortex-a7 -ffast-math -pipe -I. -O3
CFLAGS3 = -g -Wall -Ofast -lrt -mfpu=neon-fp-armv8 -mfloat-abi=hard -march=armv8-a+crc -mcpu=cortex-a53 -funsafe-math-optimizations -ffast-math -pipe -I. -O3

bin:
	mkdir bin

.PHONY: default all clean

default: $(TARGET)

digest-sse.c: digest-sse.flex
	flex -I -o $@ $<

all: digest-sse.c default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
OBJECTSCC = $(patsubst %.cc, %.o, $(wildcard *.cc))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS3) -c $< -o $@

%.o: %.cc $(HEADERS)
	$(CC) $(CFLAGS3) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS) $(OBJECTSCC)

$(TARGET): $(OBJECTS) $(OBJECTSCC)
	$(CC) $(OBJECTS) $(OBJECTSCC) -Wall $(LIBS) -o $@

clean:
	-rm digest-sse.c
	-rm -f *.o
	-rm -f $(TARGET)
