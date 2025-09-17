all: darkmon fned timely xyterm yargs

fned yargs: base.h io.h vec.h
timely: base.h vec.h

CFLAGS += -O3

darkmon: CFLAGS += $(shell pkg-config --cflags gio-2.0)
darkmon: LDLIBS += $(shell pkg-config --libs gio-2.0)

xyterm: CFLAGS += $(shell pkg-config --cflags gtk4 vte-2.91-gtk4)
xyterm: LDLIBS += $(shell pkg-config --libs gtk4 vte-2.91-gtk4)

.c:
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)
