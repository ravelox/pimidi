# FAQ

## 0. Which other libraries are needed to build ravelomidi?

Please check raveloxmidi/required for the per-platform packages needed.

## 1. External software tries to connect to raveloxmidi over IPv6

If you are finding that the client connecting to raveloxmidi is reporting issues trying to connect to an IPv6 address and you do NOT have IPv6 networking, you need to edit /etc/avahi/avahi-daemon.conf and set:
```
use-ipv6=no
publish-aaaa-on-ipv4=no
```
Obviously, if you want to use IPv6, both those options should be set to yes
You will need to restart Avahi for those changes to be picked up.

## 2. How to configure and test ALSA Support

The autotools configure script will automatically detect the presence of ALSA libraries and will build the code for support.
raveloxmidi uses the rawmidi interface so the snd-virmidi module must be loaded.

The following steps can be taken to test everything is working:

1. Ensure the snd-virmidi module is loaded.

```modprobe snd-virmidi```

2. Verify the device names

```sudo amidi -l``` will give output like 
```
Dir Device Name
IO hw:0,0 ES1371
IO hw:1,0 Virtual Raw MIDI (16 subdevices)
IO hw:1,1 Virtual Raw MIDI (16 subdevices)
IO hw:1,2 Virtual Raw MIDI (16 subdevices)
IO hw:1,3 Virtual Raw MIDI (16 subdevices)
```

3. Install timidity and run it with the ALSA interface

```timidity -iA``` will output the available ports to connect to (for example):

```
Opening sequencer port: 128:0 128:1 128:2 128:3
```

4. In a raveloxmidi config file, add the option:

```alsa.output_device = hw:1,0,0```

The device name will vary depending on the setup.

5. Run raveloxmidi with the config file. In debug mode, the debug output should show lines like:

```
[1534193901]	DEBUG: raveloxmidi_alsa_init: ret=0 Success
[1534193901]	DEBUG: rawmidi: handle="hw:1,0,0" hw_id="VirMidi" hw_driver_name="Virtual Raw MIDI" flags=7 card=1 device=0
[1534193901]	DEBUG: rawmidi: handle="hw:1,0,0" hw_id="VirMidi" hw_driver_name="Virtual Raw MIDI" flags=7 card=1 device=0
```
6. Determine the port number for hw:1,0,0 using aconnect

```aconnect -l```

This will show output like:
```
client 0: 'System' [type=kernel]
0 'Timer '
1 'Announce '
client 14: 'Midi Through' [type=kernel]
0 'Midi Through Port-0'
client 16: 'Ensoniq AudioPCI' [type=kernel,card=0]
0 'ES1371 '
client 20: 'Virtual Raw MIDI 1-0' [type=kernel,card=1]
0 'VirMIDI 1-0 '
```

This shows that ```hw:1,0,0``` is port ```20:0```

7. Connected the port to timidty:

```aconnect 20:0 128:0```

8. On the remote machine, make a connection to raveloxmidi. I have tested this with OS X.
9. (For example) In Logic Pro X, create a new external MIDI track and use the raveloxmidi connection.
10. Using the keyboard GUI in Logic Pro X, tap a few notes. The notes are played through Timidity.


For input support:

1. Repeat steps 1 and 2 above if the module isn't loaded.

2. In a raveloxmidi config file, add the option:

```alsa.input_device = hw:1,1,0```

The device name will vary depending on the setup but it MUST be different from the device configuired as the output device.

3. Run raveloxmidi with the config file. In debug mode, the debug output should show lines like:
```
[1534193901]    DEBUG: raveloxmidi_alsa_init: ret=0 Success 
[1534193901]    DEBUG: rawmidi: handle="hw:1,0,0" hw_id="VirMidi" hw_driver_name="Virtual Raw MIDI" flags=7 card=1 device=0
[1534193901]    DEBUG: rawmidi: handle="hw:1,1,0" hw_id="VirMidi" hw_driver_name="Virtual Raw MIDI" flags=7 card=1 device=0
```
4. Determine the port number for ```hw:1,1,0``` using aconnect

```aconnect -l``` will show output like:
```
client 0: 'System' [type=kernel]
0 'Timer '
1 'Announce '
client 14: 'Midi Through' [type=kernel]
0 'Midi Through Port-0' 
client 16: 'Ensoniq AudioPCI' [type=kernel,card=0]
0 'ES1371 '
client 20: 'Virtual Raw MIDI 1-0' [type=kernel,card=1]
0 'VirMIDI 1-0 '
client 21: 'Virtual Raw MIDI 1-0' [type=kernel,card=1]
0 'VirMIDI 1-0 '
```
This shows that ```hw:1,1,0``` is port ```21:0```

5. On the remote machine make a connection raveloxmidi.
6. Run your favourite music making software that will make MIDI connections.
7. On the local machine, Using aplaymidi, take a .mid file and run:

```aplaymidi -p 21:0 name-of-midi-file.mid```

The MIDI events should be processed through the remote software.
