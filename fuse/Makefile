# Make file for KFS kfs

CFLAGS = -Wall
CFLAGS += -g -D__USE_MISC -D_GNU_SOURCE
#CFLAGS += -O2
LIBS = -lpthread
LIBS += `pkg-config fuse --cflags --libs`
INCLUDE = -I../includes
CC = gcc

all: clean kfs
libs := utils super blockgroup inode dentry locks
objs := $(libs:%=%.o)

kfs.o: kfs.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -c kfs.c

$(libs):
	$(CC) $(CFLAGS) $(INCLUDE) -c ../libs/$@.c -o $@.o

kfs: kfs.o $(libs)
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -o kfs kfs.o $(objs)
	#strip kfs

install:
	cp -f kfs /sbin/

clean:
	rm -f kfs *.o
