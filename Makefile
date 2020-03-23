TARGET = ./bin/lmsmonitor
TARGETSSE = ./bin/lmsmonitor-sse
LIBS =  -lpthread -lrt -L./lib -lwiringPi -lArduiPi_OLED
CC = g++
CFLAGS4 = -g -Wall -Ofast -lrt -fPIC -fno-rtti -mfpu=neon-vfpv4    -mfloat-abi=hard -march=armv7-a -mtune=cortex-a7 -ffast-math -pipe -I. -I./source
##CFLAGS3 = -g -Wall -Ofast -lrt -mfpu=neon-fp-armv8 -mfloat-abi=hard -march=armv8-a+crc -mcpu=cortex-a53 -funsafe-math-optimizations -ffast-math -pipe -I. -I./source
CFLAGS3 = -g -Wall -Ofast -lrt -fPIC -fno-rtti -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -funsafe-math-optimizations -ffast-math -pipe -I. -I./source

CAPTURE_BMP = -DCAPTURE_BMP -I./source -I./capture
SSE_VIZDATA = -DSSE_VIZDATA -I./source -I./sse

bin:
	mkdir bin

.PHONY: default all allsse clean

default: $(TARGET)

allsse: cleansse arggh ./sse/digest-sse.c arggh $(TARGETSSE)

all: std default

OBJECTS = $(patsubst %.c, %.o, $(wildcard ./source/*.c))
OBJECTSCC = $(OBJECTS) $(patsubst %.cc, %.o, $(wildcard ./sse/*.cc)) $(patsubst %.c, %.o, $(wildcard ./sse/*.c))
HEADERS = $(wildcard ./source/*.h)
HEADERSCC = $(HEADERS) $(wildcard ./sse/*.h)

%.o: %cc $(HEADERSCC)
	$(CC) $(SSE_VIZDATA) $(CFLAGS3) -c $< -o $@

arggh:
    CC += -DSSE_VIZDATA -I./sse
    %.o: %cc %c $(HEADERSCC)
	    $(CC) $(SSE_VIZDATA) $(CFLAGS3) -c $< -o $@

sse/digest-sse.c: ./sse/digest-sse.flex
    # CC += -DSSE_VIZDATA -I./sse -I./source
	flex -I -o $@ $<

std:
    %.o: %.c $(HEADERS)
	$(CC) $(CFLAGS3) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS) $(OBJECTSCC) arggh

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

$(TARGETSSE): arggh $(HEADERSCC) $(OBJECTSCC)
	$(CC) $(SSE_VIZDATA) $(OBJECTSCC) -Wall $(LIBS) -o $@

clean:
	-rm -f capture/*.o
	-rm -f source/*.o
	-rm -f *.o
	-rm -f $(TARGET)

cleansse:
	-rm -f sse/digest-sse.c
	-rm -f source/*.o
	-rm -f sse/*.o
	-rm -f *.o
	-rm -f $(TARGETSSE)
