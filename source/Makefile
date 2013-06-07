CC = gcc
DEBUG= -g
CFLAGS = $(DEBUG) -Wall
LDFLAGS =

LISTENER_BINARY=net_midi
LISTENER_OBJS=net_midi.o net_applemidi.o net_listener.o midi_journal.o rtp_packet.o net_connection.o cmd_inv_handler.o cmd_sync_handler.o cmd_end_handler.o midi_note_packet.o midi_payload.o utils.o 
LISTENER_LIBS=

AVAHI_BINARY=avahi_test
AVAHI_OBJS=avahi_test.o
AVAHI_LIBS=-lavahi-client -lavahi-common

all: $(LISTENER_BINARY) $(AVAHI_BINARY)
listener: $(LISTENER_BINARY)

$(LISTENER_BINARY): $(LISTENER_OBJS)
	$(CC) -o $(LISTENER_BINARY) $(DEBUG) $(LISTENER_OBJS) $(LDFLAGS) $(LISTENER_LIBS)

$(AVAHI_BINARY): $(AVAHI_OBJS)
	$(CC) -o $(AVAHI_BINARY) $(DEBUG) $(AVAHI_OBJS) $(LDFLAGS) $(AVAHI_LIBS)

.c.o:
	$(CC) -c $(DEBUG) $(CFLAGS) $<

clean:
	rm -f $(AVAHI_BINARY) $(AVAHI_OBJS) $(LISTENER_BINARY) $(LISTENER_OBJS)
