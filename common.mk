
# for cross-building with MinGW
CROSS = x86_64-w64-mingw32-
INCLUDES = -I/usr/share/mingw-w64/include

INCLUDES += -I../include -I../shared
CPPFLAGS += $(INCLUDES) -O2
CFLAGS += $(INCLUDES) -O2
LDFLAGS += -municode

CC = $(CROSS)gcc
LD = $(CROSS)ld
CXX = $(CROSS)g++

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) $^

clean:
	rm -f $(EXE) $(OBJS) $(DELOBJS)
