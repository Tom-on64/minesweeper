# Configuration file

VERSION = "1.0.0"

PREFIX = /usr/local
MANFIX = $(PREFIX)/share/man

CC = cc
LD = cc
CCFLAGS = -Wall -Wextra -Wpedantic -std=c17
LDFLAGS = -lSDL2

