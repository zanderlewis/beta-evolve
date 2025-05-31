# Beta Evolve Cross-Platform Makefile

# Detect operating system
UNAME_S := $(shell uname -s)

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude $(shell pkg-config --cflags libcjson)
LDFLAGS = $(shell pkg-config --libs libcjson) -lm

# Directory structure
SRCDIR = src
OBJDIR = obj
INCDIR = include

# Source and object files (including subdirectories)
SOURCES = $(wildcard $(SRCDIR)/*.c) \
          $(wildcard $(SRCDIR)/core/*.c) \
          $(wildcard $(SRCDIR)/utils/*.c) \
          $(wildcard $(SRCDIR)/api/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET = beta_evolve


# Windows (MSYS2/MinGW)
ifeq ($(OS),Windows_NT)
    TARGET = beta_evolve.exe
endif

# Default target
all: $(TARGET)

# Create object directory and subdirectories
$(OBJDIR):
	mkdir -p $(OBJDIR)
	mkdir -p $(OBJDIR)/core
	mkdir -p $(OBJDIR)/utils
	mkdir -p $(OBJDIR)/api

# Build target
$(TARGET): $(OBJDIR) $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Build object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(OBJDIR) $(TARGET) $(TARGET).exe

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

.PHONY: all clean debug
