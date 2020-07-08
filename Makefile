TARGET=baash
CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -Wbad-function-cast -Wstrict-prototypes\
		-Wmissing-declarations -Wmissing-prototypes -Wno-unused-parameter -Werror\
		-pedantic -g

# Propagar entorno a make en tests/
export CC CFLAGS

# Agregar bstring/ a los directorios de busqueda
BSTRING=bstrlib.c
vpath $(BSTRING) bstring

SOURCES=$(shell echo *.c) $(BSTRING)
OBJECTS=$(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) $(OBJECTS) .depend *~
	make -C tests clean

test: $(OBJECTS)
	make -C tests test

test-command: command.o bstrlib.o
	make -C tests test-command

memtest: $(OBJECTS)
	make -C tests memtest

.depend: $(SOURCES)
	$(CC) -MM $^ > $@

-include .depend

.PHONY: clean all
