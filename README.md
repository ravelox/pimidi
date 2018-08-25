The original reason for writing this was to send NoteOn/NoteOff MIDI events from
a self-built drum pad interface through a Raspberry Pi to Logic Pro X on OS X.

This has evolved into a proxy for Note, Control and Program events with ALSA support.

I'm doing this purely for fun and don't expect anyone else to use
it but I'm happy to accept suggestions if you ever come across
this code.

If you are using raveloxmidi, please drop me a line with some detail and I'll add your name to a list
with a link to your project.

This code is released under GPLv3.

Simple usage is:

1. On the Linux box, build the raveloxmidi binary and run it in the background.
2. Connect an Apple machine to raveloxmidi using Audio MIDI setup under Applications/Utilities.
3. Run Logic Pro X on the Apple machine and start recording a MIDI track.
4. On the Linux box, run python/note_send.py to send some test MIDI notes. Use python/control_send.py for Control Change events.
5. Those notes should register in the MIDI track.

For more information, read the README file in raveloxmidi/
