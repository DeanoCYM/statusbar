# This file is part of statusbar - a status bar for dwm
# Copyright (C) 2020  Ellis Rhys Thomas <e.rhys.thomas@gmail.com>
# See COPYING for licence details.

TARGET:=statusbar
SRC=$(wildcard ./src/*.c)
LIBS:=-lX11 -lpulse

CC:=cc
CFLAGS:= -Wall -Wextra -Wfatal-errors -g3 -o0 -DDEBUG
OBJ=$(SRC:%.c=%.o)
INCLUDE:=-I./src

PREFIX:=/usr/local
SYSTEMDPATH:=/etc/systemd/user/

.PHONY: all tags clean build install uninstall

all: tags $(TARGET)
$(TARGET): $(OBJ) 
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@ $(LIBS)
%.o: ./src/%.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $^ -o $@ $(LIBS)

clean:
	$(RM) $(OBJ) $(TARGET) $(DEP)

build: CFLAGS:=-Wall -Wextra -Werror -Wfatal-errors -DNDEBUG
build: clean all
install: build
	install -d $(PREFIX)/bin
	install -m 755 $(TARGET) $(PREFIX)/bin
	install -d $(SYSTEMDPATH)
	install -m 644 $(TARGET).service $(SYSTEMDPATH)
uninstall:
	rm -i $(PREFIX)/bin/$(TARGET)
