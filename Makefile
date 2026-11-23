#
#	Makefile for CVA Monte Carlo Simulation with Signaloid UxHw Support
#
#
#	Copyright (c) 2026, Signaloid.
#
#	Targets:
#	  local-build        - Build for native execution with GSL compat layer
#	  build-local        - Alias of local-build
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

#
#	`src/config.mk` is the source list shared with the Signaloid cloud
#	build. It omits `uxhw.c`, because the cloud compiler provides the
#	UxHw API natively. The native build compiles the shim in explicitly.
#
include src/config.mk

SOURCES := $(addprefix $(SRCDIR)/,$(SOURCES)) $(SRCDIR)/uxhw.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

.PHONY: all build-local local-build run-local local-run clean

all: local-build

local-build: $(TARGET)

build-local: local-build

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run-local: local-build
	@echo "=== CVA (S=0, σ=0.0075, 1500 paths to match notebook) ==="
	./$(TARGET) -M 1500 -S 0 -x 0.0075 -T

local-run: run-local

clean:
	rm -rf $(BUILDDIR) $(TARGET)
