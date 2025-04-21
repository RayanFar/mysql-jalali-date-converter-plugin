TARGET = gregorian_to_jalali
SRC = gregorian_to_jalali.c

OS := linux
ARCH := $(shell uname -m)
OUTDIR = build/$(OS)/$(ARCH)
OUTFILE = $(OUTDIR)/$(TARGET).so

MYSQL_INC := $(shell mysql_config --include 2>/dev/null)

CFLAGS = -Wall -fPIC $(MYSQL_INC)
LDFLAGS = -shared

all: $(OUTFILE)

$(OUTFILE): $(SRC)
	mkdir -p $(OUTDIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf build
