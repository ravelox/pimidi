# RaveloxMIDI

Please read **FAQ.md**

raveloxmidi is a simple proxy to send MIDI NoteOn, NoteOff, Control Change (CC) and Program Change (PC) events from the local machine via RTP MIDI or to receive any MIDI events from a remote source via RTP MIDI and write them to a file.

The reason for writing this was to generate note events from a Raspberry Pi to send them to Logic Pro X. In particular, using the Raspberry Pi to handle input from drum pads. As some people have started to use this, there have been several requests for the ability to send Control Change and Program Change events too. I've included some very basic python scripts for testing.

Thanks to feedback from a couple of users, I've also tested this with rtpMIDI on Windows talking to FL Studio 11.

The build will auto-detect ALSA and build rawmidi support. Please read **FAQ.md** for ALSA requirements. Thanks to Daniel Collins (malacalypse) for being the guinea pig for this.

Except for the Avahi code, it's all mine but I have leaned heavily on the following references:

* RTP MIDI: An RTP Payload Format for MIDI ( http://www.eecs.berkeley.edu/~lazzaro/rtpmidi/index.html )
* The RTP MIDI dissector in wireshark. Written by Tobias Erichsen ( http://www.tobias-erichsen.de )

Note: Where possible, I've tried to use RTP MIDI to mean the protocol and rtpMIDI to mean the software written by Tobias Erichsen. Some mistakes may be present but,
in most cases, I am referring to the protocol

Some Apple documentation is available at https://developer.apple.com/library/archive/documentation/Audio/Conceptual/MIDINetworkDriverProtocol/MIDI/MIDI.html
I'm doing this purely for fun and don't expect anyone else to use
it but I'm happy to accept suggestions if you ever come across
this code.

If you are using raveloxmidi, please drop me a line with some detail and I'll add your name to a list
with a link to your project.

## Description
There are a number of stages in the general RTP MIDI protocol, SIP, MIDI and feedback. The lazzaro document talks about SIP and feedback in terms of RTP but Apple has their own packet format for that information. raveloxmidi makes assumptions that may or may not be correct but the Apple MIDI SIP process isn't documented.

### Stage 1 - SIP
Audio MIDI setup needs to know which server to connect to. In order to do that, the raveloxmidi announces the Apple MIDI service (_apple-midi._udp). By default, the announcement is for connections on port 5004. This also implies port 5005 is open for connections too.

The connecting server will sent 2 connection (INV) requests containing its name, its IP address and the port that connections can be made on.

raveloxmidi will accept the first INV request and store the connection information in a table ( limited to 8 entries ) for later use.

raveloxmidi sends back an OK response to both requests.

The connecting server will send SYNC packets to raveloxmidi. According to http://en.wikipedia.org/wiki/RTP_MIDI, the sender will send timestamp1 to indicate its local time. This timestamp isn't an actual time value but can be a delta from a local value, the receiver will send timestamp2 with its local delta and then the sender will send timestamp3 indicating the time it received the timestamp2 response from the receiver. This gives the system an idea of latency.

SYNC packets are sent from the connecting server on a regular basis.

### Stage 2 - MIDI
This stage uses the defined MIDI RTP payload. raveloxmidi opens an additional listening port (by default 5006) which will accept a simple data packet containing note information. Where Apple MIDI packets are prefixed with a single byte 0xff, note data packets are expected to be prefixed with 0xaa. The rest of data packet is expected to be a MIDI Note or ControlChange command.

```
Command: 4 bits ( NoteOn = 0x09, NoteOff = 0x08 , ControlChange = 0x0B )
Channel: 4 bits ( 0 - 15 )
```

Note events are defined as:
```
Note: 8 bits ( -127 to 127 )
Velocity: 8 bits ( 0 to 127 )
```

Control Change events are defined as:
```
Controller Number: 8 bits (0 to 127 with 120-127 as Channel Mode messages)
Controller Value : 8 bits (0 to 127).
```

When raveloxmidi receives the command packet, it will send that MIDI command to any  active connections in the connection table. 

The RTP MIDI payload specification also requires a recovery journal ( see section 4 of RFC6295 at http://www.rfc-editor.org/rfc/rfc6295.txt ). raveloxmidi will add the note and control change events to the journal and attach the journal in each RTP packet sent to the connecting server. As raveloxmidi is only concerned with NoteOn, NoteOff and ControlChange events, only Chapter N and Chapter C journal entries are stored.

### Stage 3 - Feedback
The Apple MIDI implementation sends a feedback packet (RS) from the connecting server. This packet contains a RTP sequence number to indicate that the connecting server is acknowledging that it has received packets with a sequence number up to and including that particular value.

This tells the receiving server that it doesn't need to send journal events for any packets with a sequence number lower than that value.

raveloxmidi keeps track of the sequence number in the connection table and, if the value in the feedback packet is greater than or equal to the value in the table, the journal will be reset. 

## Inbound MIDI commands 
raveloxmidi will also accept inbound RTP-MIDI from remote hosts and will write the MIDI commands to a named file. MIDI commands are written at the time they are received and in the order that they are listed in the MIDI payload of the RTP packet. At this time, there is no handling of the RTP-MIDI journal on the inbound connection. A Feedback response is sent back when inbound midi events are received.

## Command interface
raveloxmidi provides a simple set of commands for shutdown, heartbeat and connection status. The commands can only be received on the local listening port ( default is 5006 ). The commands are:

*STAT*

This is the heartbeat command. The response will always be *OK* if raveloxmidi is running. The script python/send_stat.py is available for this command.

*QUIT*

This will shut down raveloxmidi. The response will always be *QT* to indicate that the command has been received. The script python/send_quit.py is available for this command.

*LIST*

This requests a lists of current connections into raveloxmidi. The response will be a JSON blob of information. The script python/send_list.py is available for this command.
The JSON data looks like this:

```
~/pimidi/python$ ./send_list.py | python -m json.tool

{
    "connections": [
        {
            "control": 5004,
            "ctx": "0x556d380bac30",
            "data": 5005,
            "host": "192.168.1.145",
            "id": 0,
            "initiator": "0x19495cff",
            "send_ssrc": "0x2dfec3cc",
            "seq": 1,
            "ssrc": "0x4181a596",
            "start": 15861082588188,
            "status": "idle"
        }
    ],
    "count": 1
}
```
To parse the data, the *count* field will be the number of connections in the list. The connections array holds each connection. The *id* field in the connections array is an internal id for the array. The value of that field *may* change. It is recommended that you use the *ssrc* field as the uniq identifier for the connection.

## Configuration
raveloxmidi can be run with a -c parameter to specify a configuration file with the options listed below.
Where the option isn't specified, a default value is used.

For debugging, you can run ```raveloxmidi -N -d``` to keep raveloxmidi in the foreground and send debug-level output to stderr.

### Options

Some parameters are marked as multi-value which means you can either specify them as a single value or with numerical index as a suffix.  For example, if there is a parameter called ```alsa.output_device``` you can use either:

```
alsa.output_device = device_name
```
or

```
alsa.output_device.0 = first_device
alsa.output_device.1 = second_device
```

A multi-value configuration option must start at index 0 and parsing will stop when there is a break in the sequence. That is, if the values are 0,1,3,4,5 then only 0 and 1 will be detected.

```
network.bind_address
	IP address that raveloxmidi listens on. This can be an IPv4 or IPv6 address.
	Default is 0.0.0.0 ( meaning all IPv4 interfaces ). IPv6 equivalent is ::
	This must be set in the configuration file for raveloxmidi to run.
network.control.port
	Main RTP MIDI listening port for new connections and shutdowns.
	Used in the zeroconf definition for the RTP MIDI service.
	Default is 5004.
network.data.port
	Listening port for all other data in the conversation.
	Default is 5005.
network.local.port
	Local listening port for accepting MIDI events.
	Default is 5006.
service.ipv4
	Indicate whether Avahi service should use IPv4 addresses. Default is yes.
service.ipv6
	Indicate whether Avahi service should use IPv6 addressed. Default is no.
network.max_connections
	Maximum number of incoming connections that can be stored.
	Default is 8.
service.name
	Name used in the zeroconf definition for the RTP MIDI service.
	Default is 'raveloxmidi'.
remote.connect
	Name of remote service to connect to.
	The value can be one of two formats:
		To connect to a Bonjour-advertised service, use the format:
			remote.connect = service
		To connect directly to a server/port, use the format:
			remote.connect = [address]:port
			A port number must be specified if making a direct connection.
remote.use_control_for_ck
	Indicates whether CK (AppleMIDI Feedback) messages are sent to the a remote connection using the control port.
	Default is yes
client.name
	Name to use when connecting to remote service. If not defined, service.name will be used.
network.socket_timeout
	Polling timeout for the listening sockets.
	Default is 30 seconds.
discover.timeout
	Length of time in seconds to wait for new remote services to be seen. Default is 5 seconds.
run_as_daemon
	Specifies that raveloxmidi should run in the background.
	Default is yes.
daemon.pid_file
	If raveloxmidi is run in the background. The pid for the process is written to this file.
	Default is raveloxmidi.pid.
logging.enabled
	Set to yes to write output to a log file. Set to no to disable.
	Default is "yes".
logging.log_file
	Name of file to write logging to.
	Default is stderr.
logging.log_level
	Threshold for log events. Acceptable values are debug,info,normal,warning and error.
	Default is normal.
logging.hex_dump
	Set to yes to write hex dump of data buffers to log file. This can slow down processing for large buffers if enabled.
	Default is no
security.check
	If set to yes, it is not possible to write the daemon pid to a file with executable permissions.
	Default is yes.
inbound_midi
        Name of file to write inbound MIDI events to. This file is governed by the security check option.
	Default is /dev/sequencer
file_mode
        File permissions on the inbound_midi file if it needs to be created. Specify as Unix octal permissions. 
	Default is 0640.
sync.interval
	Interval in seconds between SYNC commands for timing purposes. Default is 10s.
journal.write
	Set to yes to enable MIDI recovery journal. Default is no.
```

If ALSA is detected, the following options are also available:

```
alsa.output_device
	Name of the rawmidi ALSA device to send MIDI events to.
	This is a multi-value option.
alsa.input_device
	Name of the rawmidi ALSA device to read MIDI events from.
	This is a multi-value option.
alsa.input_buffer_size
	Size of the buffer to use for reading data from the input device.
	Default is 4096. Maximum is 65535.
alsa.writeback
	If a MIDI command is received from an inbound ALSA device, this option controls whether that event is written to an ALSA output device if it has the same card number.
	This is a yes/no option. Default is no.
alsa.writeback
	If a MIDI command is received from an inbound ALSA device, this option controls whether that event is written to an ALSA output device if it has the same level number.
	See also **alsa.writeback.level**.
	This is a yes/no option. Default is no.
alsa.writeback.level
	Indicates how granular to make the alsa.writeback check.
	Possible values are **card** (hw:X,*,*) or **device** (hw:X,Y,*)
	Default is card.
```
