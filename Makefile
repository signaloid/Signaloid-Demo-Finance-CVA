#
#	Makefile for CVA Monte Carlo Simulation with Signaloid UxHw Support
#
#
#	Copyright (c) 2026, Signaloid.
#
#	Targets:
#	  build-local        - Build for native execution with GSL compat layer
#	  run-local          - Build and run Monte Carlo for the CVA output
#	  clean              - Remove build artifacts
#

CC = gcc
CFLAGS = -std=c11 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2 -Isrc
LDFLAGS = -lm -lgsl -lgslcblas

#
#	Platform-specific paths. On macOS, GSL is not on the default search
#	path, so locate it under MacPorts (/opt/local), Homebrew on Apple
#	Silicon (/opt/homebrew) or Homebrew on Intel (/usr/local).
#
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    ifneq ($(wildcard /opt/local/include/gsl),)
        CFLAGS  += -I/opt/local/include
        LDFLAGS += -L/opt/local/lib
    else ifneq ($(wildcard /opt/homebrew/include/gsl),)
        CFLAGS  += -I/opt/homebrew/include
        LDFLAGS += -L/opt/homebrew/lib
    else ifneq ($(wildcard /usr/local/include/gsl),)
        CFLAGS  += -I/usr/local/include
        LDFLAGS += -L/usr/local/lib
    endif
endif

SRCDIR = src
BUILDDIR = build
TARGET = demo-native-mc

SOURCES = \
	$(SRCDIR)/main.c \
	$(SRCDIR)/kernel.c \
	$(SRCDIR)/common.c \
	$(SRCDIR)/utilities.c \
	$(SRCDIR)/uxhw.c \
	$(SRCDIR)/cva-hw.c \
	$(SRCDIR)/cva-pricing.c \
	$(SRCDIR)/cva-cva.c \
	$(SRCDIR)/cva-uxhw.c \
	$(SRCDIR)/cva-monte-carlo.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

.PHONY: all build-local local-build run-local local-run clean

all: build-local

build-local: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run-local: build-local
	@echo "=== CVA (S=0, σ=0.0075, 1500 paths to match notebook) ==="
	./$(TARGET) -M 1500 -S 0 -x 0.0075 -T

local-build: build-local

local-run: run-local

clean:
	rm -rf $(BUILDDIR) $(TARGET)

