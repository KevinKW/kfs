# Make file for KFS mkfs

CFLAGS = -Wall
CFLAGS += -g -D__USE_MISC -D_GNU_SOURCE
#CFLAGS += -O2
LIBS = -lpthread
LIBS += `pkg-config fuse --cflags --libs`
INCLUDE = -I../includes
CC = gcc

all: clean mkfs
libs := utils super blockgroup inode locks
objs := $(libs:%=%.o)

mkfs.o: mkfs.c
	$(CC) $(CFLAGS) $(INCLUDE) -c mkfs.c

$(libs):
	$(CC) $(CFLAGS) $(INCLUDE) -c ../libs/$@.c -o $@.o

mkfs: mkfs.o $(libs)
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -o mkfs mkfs.o $(objs)
	#strip mkfs

install:
	cp -f mkfs /sbin/

clean:
	rm -f mkfs *.o
