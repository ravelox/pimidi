See http://projects.ravelox.com/blog for the reason I'm doing this.

This is a simple RTP midi implementation for NoteOn/NoteOff events from
a $150 ION ION Sound Sessions drum kit through a Raspberry Pi to
Logic Pro X on an Apple machine.

I'm doing this purely for fun and don't expect anyone else to use
it but I'm happy to accept suggestions if you ever come across
this code.

This code is released under GPLv3.

Simple usage is:

1. On the Linux box, buld the raveloxmidi binary and run it in the background.
2. Connect an Apple machine to raveloxmidi using Audio MIDI setup under Applications/Utilities.
3. Run Logic Pro X on the Apple machine and start recording a MIDI track.
4. On the Linux box, run python/udp_client.py to send some test MIDI notes.
5. Those notes should register in the MIDI track.
