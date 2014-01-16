CC = gcc

FLAGS        = -std=gnu99
CFLAGS       = -Wall -Wextra
DEBUGFLAGS   = -O0 -g
RELEASEFLAGS = -O2 -combine

TARGET  = nes
SOURCES = $(shell echo *.c)
COMMON  =
HEADERS = $(shell echo *.h)
OBJECTS = $(SOURCES:.c=.o)

PREFIX = $(DESTDIR)/usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET)

$(TARGET): $(OBJECTS) $(COMMON)
	$(CC) $(FLAGS) $(CFLAGS) $(DEBUGFLAGS) -o $(TARGET) $(OBJECTS)

release: $(SOURCES) $(HEADERS) $(COMMON)
	$(CC) $(FLAGS) $(CFLAGS) $(RELEASEFLAGS) -o $(TARGET) $(SOURCES)

profile: CFLAGS += -pg
profile: $(TARGET)

install: release
	install -D $(TARGET) $(BINDIR)/$(TARGET)

install-strip: release
	install -D -s $(TARGET) $(BINDIR)/$(TARGET)

uninstall:
	-rm $(BINDIR)/$(TARGET)

clean:
	-rm -f $(OBJECTS)
	-rm -f gmon.out

distclean: clean
	-rm -f $(TARGET)

.SECONDEXPANSION:

$(foreach OBJ,$(OBJECTS),$(eval $(OBJ)_DEPS = $(shell gcc -MM $(OBJ:.o=.c) | sed s/.*://)))
%.o: %.c $$($$@_DEPS)
	$(CC) $(FLAGS) $(CFLAGS) $(DEBUGFLAGS) -c -o $@ $<


.PHONY : all profile release install install-strip uninstall clean distclean
