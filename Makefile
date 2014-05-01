CC = clang

FLAGS        = -std=gnu99
CFLAGS       = -Wall -Wextra
DEBUGFLAGS   = -O0 -g
RELEASEFLAGS = -O2 -combine
LINKFLAGS    =

TARGET  = nes
SOURCES = $(shell echo *.c)
COMMON  =
HEADERS = $(shell echo *.h)
OBJECTS = $(SOURCES:.c=.o)

PREFIX = $(DESTDIR)/usr/local
BINDIR = $(PREFIX)/bin

# SDL2
CFLAGS += $(shell sdl2-config --cflags)
LINKFLAGS += $(shell sdl2-config --static-libs)

all: $(TARGET)

$(TARGET): $(OBJECTS) $(COMMON)
	$(CC) $(DEBUGFLAGS) -o $(TARGET) $(OBJECTS) $(LINKFLAGS)

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
