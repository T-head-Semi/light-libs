CC=$(CROSS)gcc
CFLAGS:=-fpic
LDFLAGS:=-shared -fpic
SOURCE:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o,$(SOURCE))
OUTDIR=./output
TARGET_LIB:=libiopmp.so


all:$(OBJS)
	echo $(OBJS)
	mkdir -p $(OUTDIR)
	$(CC) $(LDFLAGS) -o $(OUTDIR)/$(TARGET_LIB) $(OBJS)

%.o:%.c
	@echo Compiling $< ...
	$(CC) -c $(CFLAGS) $< -o $*.o

.PHONY: clean

clean:
	rm -rf $(OUTDIR)/$(TARGET_LIB) *.o
