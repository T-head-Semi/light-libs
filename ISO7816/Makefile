##
 # Copyright (C) 2021 Alibaba Group Holding Limited
##

CC=$(CROSS_COMPILE)gcc
SOURCE:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o,$(SOURCE))
OUTDIR=./output
TARGET_ELF:=iso7816


all:$(OBJS)
	echo $(OBJS)
	mkdir -p $(OUTDIR)
	$(CC) -o $(OUTDIR)/$(TARGET_ELF) $(OBJS)

%.o:%.c
	@echo Compiling $< ...
	$(CC) -c $< -o $*.o

.PHONY: clean

clean:
	rm -rf $(OUTDIR)/$(TARGET_ELF) *.o
