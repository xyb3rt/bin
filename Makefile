all: fned timely

fned timely: indispensbl/call.h indispensbl/core.h indispensbl/vec.h

.c:
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)
