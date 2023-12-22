CC := gcc
CFLAGS := -Wall -g -D__DS_DBG__ -D_FILE_OFFSET_BITS=64
INCLUDE := -I. -I./common
LIBS := -L/usr/local/lib -L/usr/lib -L. -L./common -pthread

COMMON_SRCS := ./common/ds_err.c ./common/ds_common.c 
COMMON_OBJS := $(subst .c,.o, $(COMMON_SRCS))

EXEC := OSSInodeCacheHold

all: $(EXEC)
	
ds_mount: ds_mount.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

OSSInodeCacheHold: OSSInodeCacheHold.o $(COMMON_OBJS) ./common/ds_syscall.o
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS) -lcrypto -lssl

t: t.o $(COMMON_OBJS) ./common/ds_syscall.o
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

testso: test.o
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS) -ldeepcommon

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@ 





.PHONY: all clean
clean:
	@rm -f *.o lib*.so lib*.a $(EXEC) $(COMMON_OBJS) ./common/*.o


## file dependency
./common/ds_err.o: ./common/ds_err.c ./common/ds_err.h
./common/ds_common.o: ./common/ds_common.c ./common/ds_common.h
./common/ds_syscall.o: ./common/ds_syscall.c ./common/ds_syscall.h
