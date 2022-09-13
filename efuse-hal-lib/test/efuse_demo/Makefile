CC=$(CROSS)gcc
CFLAGS=-DxDEBUG
LIBS=-L ../../lib/output -lefuse

BIN = efuse_demo
OUTDIR = ../output
SRCS:=$(wildcard *.c)
COBJS:=$(SRCS:.c=.o)

all:$(OUTDIR)/$(BIN)

$(OUTDIR)/$(BIN):$(COBJS)
	mkdir -p $(OUTDIR)
	$(CC) -o $(OUTDIR)/$(BIN) $(CFLAGS) $(COBJS) $(LIBS)

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -rf $(OUTDIR)/$(BIN) $(COBJS)
