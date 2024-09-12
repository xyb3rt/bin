all: fned timely xyterm

fned timely: base.h vec.h

xyterm: CFLAGS += $(shell pkg-config --cflags gtk4 vte-2.91-gtk4)
xyterm: LDLIBS += $(shell pkg-config --libs gtk4 vte-2.91-gtk4)

.c:
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)
