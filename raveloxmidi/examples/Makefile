CC ?= cc
PKG_CONFIG ?= pkg-config

CFLAGS += -Wall -Wextra -g $(shell $(PKG_CONFIG) --cflags raveloxmidi)
LDLIBS += $(shell $(PKG_CONFIG) --libs raveloxmidi)

EXAMPLES = \
	minimal_sdk \
	midi_event_callback \
	send_note_and_stop

.PHONY: all clean

all: $(EXAMPLES)

minimal_sdk: minimal_sdk.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

midi_event_callback: midi_event_callback.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

send_note_and_stop: send_note_and_stop.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(EXAMPLES)
