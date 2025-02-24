SHELL := /bin/bash

# Compiler and flag settings
CC=gcc			# GCC Compiler is the default
CFLAGS=-g -w		# Build with debugging enabled by default

# HELP
.PHONY: help
help:
	@echo "Usage make <TARGET>"
	@echo ""
	@echo "  Targets:"
	@echo "	   build				Build the decoder executable"
	@echo "	   run					Run the decoder program"

# Build target
.PHONY: build
build: decoder

decoder: decoder.o nethelper.o
	$(CC) $(CFLAGS) -o decoder decoder.o nethelper.o

decoder.o: decoder.c decoder.h packet.h nethelper.h testframes.h
	$(CC) $(CFLAGS) -c decoder.c

nethelper.o: nethelper.c nethelper.h packet.h
	$(CC) $(CFLAGS) -c nethelper.c

# Run target
.PHONY: run
run: decoder
	./decoder