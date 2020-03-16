TARGET = ./bin/lmsmonitor
TARGETSSE = ./bin/lmsmonitorx
LIBS =  -lpthread -lrt -L./lib -lwiringPi -lArduiPi_OLED
CC = g++
CFLAGS4 = -g -Wall -Ofast -lrt -mfpu=neon-vfpv4    -mfloat-abi=hard -march=armv7-a -mtune=cortex-a7 -ffast-math -pipe -I. -I./source
CFLAGS3 = -g -Wall -Ofast -lrt -mfpu=neon-fp-armv8 -mfloat-abi=hard -march=armv8-a+crc -mcpu=cortex-a53 -funsafe-math-optimizations -ffast-math -pipe -I. -I./source

CAPTURE_BMP = -DCAPTURE_BMP -I./capture
SSE_VIZDATA = -DSSE_VIZDATA -I./sse

bin:
	mkdir bin

.PHONY: default all allsse clean

default: $(TARGET)

digest-sse.c: ./sse/digest-sse.flex
	flex -I -o $@ $<

allsse: digest-sse.c $(TARGETSSE)

all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard ./source/*.c))
OBJECTSCC = $(patsubst %.cc, %.o, $(wildcard ./sse/*.cc))
HEADERS = $(wildcard ./source/*.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS3) -c $< -o $@

%.o: %.cc $(HEADERS)
	$(CC) $(CFLAGS3) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS) $(OBJECTSCC)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

$(TARGETSSE): $(OBJECTS) $(OBJECTSCC) $(SSE_VIZDATA)
	$(CC) $(OBJECTS) $(OBJECTSCC) $(SSE_VIZDATA) -Wall $(LIBS) -o $@

clean:
	-rm sse/digest-sse.c
	-rm -f *.o
	-rm -f $(TARGET)
	-rm -f $(TARGETSSE)
