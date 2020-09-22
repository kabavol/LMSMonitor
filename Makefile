# making leaner with overrides - 20200407

TARGET = ./bin/lmsmonitor
TARGETSSE = ./bin/lmsmonitor-sse

# need https support so ssl and crypto added
LIBS =  -pthread -lrt -L./lib -li2c -lssl -lcrypto

CC = g++

CFLAGS4 = -g -Wall -std=c11 -Ofast -lrt -fPIC -fno-rtti -mfpu=neon-vfpv4  \
-Wstringop-truncation -Wno-unused-variable -Wno-unused-parameter \
-fpermissive -Wunused-function -Wstringop-truncation -Wunused-but-set-variable -Wstringop-overflow \
-mfloat-abi=hard -march=armv7-a -mtune=cortex-a7 -ffast-math -pipe -I. -I./src

CFLAGS4 = -g -Wall -std=c11 -Ofast -lrt -fPIC -fno-rtti -mfpu=neon-vfpv4  \
-Wstringop-truncation -Wno-unused-variable -Wno-unused-parameter \
-fpermissive -Wunused-function -Wstringop-truncation -Wunused-but-set-variable -Wstringop-overflow \
-mfloat-abi=hard -ffast-math -pipe -I. -I./src

CFLAGS3 = -g -Wall -std=c11 -Ofast -lrt -fPIC -fno-rtti  -Wno-unused-variable \
-Wno-unused-parameter -Wstringop-truncation -mfpu=vfp -mfloat-abi=hard \
-fpermissive -Wunused-function -Wstringop-truncation -Wunused-but-set-variable -Wstringop-overflow \
-march=armv6zk -mtune=arm1176jzf-s -funsafe-math-optimizations -ffast-math -pipe -I. -I./src

CFLAGS3 = -g -Wall -std=c11 -Ofast -fPIC -fno-rtti  -Wno-unused-variable \
-Wno-unused-parameter -Wstringop-truncation -mfpu=vfp -mfloat-abi=hard \
-fpermissive -Wunused-function -Wstringop-truncation -Wunused-but-set-variable -Wstringop-overflow \
-funsafe-math-optimizations -ffast-math -pipe -I. -I./src

CAPTURE_BMP = -DCAPTURE_BMP -I./src -I./capture
SSE_VIZDATA = -DSSE_VIZDATA -I./src -I./sse


bin:
	mkdir bin

.PHONY: default all clean # allsse cleansse

default: $(TARGET)

#allsse: cleansse coresse ./sse/digest-sse.c coresse $(TARGETSSE)

all: bin std default

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard ./src/*.cpp)) $(patsubst %.cc, %.o, $(wildcard ./src/*.cc)) $(patsubst %.c, %.o, $(wildcard ./src/*.c))
OBJECTSCC = $(OBJECTS) $(patsubst %.cc, %.o, $(wildcard ./sse/*.cc)) $(patsubst %.c, %.o, $(wildcard ./sse/*.c))
HEADERS = $(wildcard ./src/*.h) $(wildcard ./font/*.h)
HEADERSCC = $(HEADERS) $(wildcard ./sse/*.h)

%.o: %cc $(HEADERSCC)
	$(CC) $(SSE_VIZDATA) $(CFLAGS3) -c $< -o $@

sse/digest-sse.c: ./sse/digest-sse.flex
	flex -I -o $@ $<

std:
    %.o: %.c $(HEADERS)
	$(CC) $(CFLAGS3) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS) $(OBJECTSCC) coresse

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

coresse:
#    CC += -DSSE_VIZDATA -I./sse
    %.o: %cc %c $(HEADERSCC)
	    $(CC) $(SSE_VIZDATA) $(CFLAGS3) -c $< -o $@

$(TARGETSSE): coresse $(HEADERSCC) $(OBJECTSCC)
	$(CC) $(SSE_VIZDATA) $(OBJECTSCC) -Wall $(LIBS) -o $@

clean:
	-rm -f capture/*.o
	-rm -f src/*.o
	-rm -f *.o
	-rm -f $(TARGET)

cleansse:
	-rm -f sse/digest-sse.c
	-rm -f src/*.o
	-rm -f sse/*.o
	-rm -f *.o
	-rm -f $(TARGETSSE)
