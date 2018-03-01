WARNING

THIS IS THE EXPERIMENTAL BRANCH

THE CODE IN THIS BRANCH SHOULD BE CONSIDERED UNSTABLE
IT IS PRONE TO SEGMENTATION VIOLATIONS AND SOME FEATURES WILL NOT BE FULLY-AVAILABLE

IF YOU WANT A STABLE BRANCH, SWITCH TO MASTER

END WARNING

See http://www.raveloxprojects.com/blog/?cat=5 for the reason I'm doing this.

The original reason for writing this was to send NoteOn/NoteOff MIDI events from
a self-built drum pad interface through a Raspberry Pi to Logic Pro X on OS X.

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
4. On the Linux box, run python/udp_client.py to send some test MIDI notes.
5. Those notes should register in the MIDI track.

For more information, read the README file in raveloxmidi/
