all: darkmon fned timely xyterm yargs

darkmon fned timely xyterm yargs: Makefile
fned timely yargs: base.h vec.h
fned yargs: io.h

CFLAGS += -O3
CFLAGS_darkmon != pkg-config --cflags gio-2.0
LDLIBS_darkmon != pkg-config --libs gio-2.0
CFLAGS_xyterm != pkg-config --cflags gtk4 vte-2.91-gtk4
LDLIBS_xyterm != pkg-config --libs gtk4 vte-2.91-gtk4

.c:
	$(CC) $(CFLAGS) $(CFLAGS_$@) $(CPPFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS) $(LDLIBS_$@)
