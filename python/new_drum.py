#!/usr/bin/env python
import time
import os
import RPi.GPIO as GPIO
import socket
import struct
 
GPIO.setmode(GPIO.BCM)
DEBUG = 0
 
local_port = 5006

if len(sys.argv) == 1:
	family = socket.AF_INET
	connect_tuple = ( 'localhost', local_port )
else:
	details = socket.getaddrinfo( sys.argv[1], local_port, socket.AF_UNSPEC, socket.SOCK_DGRAM)
	family = details[0][0]
	if family == socket.AF_INET6:
		connect_tuple = ( sys.argv[1], local_port, 0, 0)
	else:
		connect_tuple = ( sys.argv[1], local_port)

s = socket.socket( family, socket.SOCK_DGRAM )
s.connect( connect_tuple )

def send_note( velocity):
	# Note ON
	bytes = struct.pack( "BBB", 0x96, 0x1f, velocity )
	s.send( bytes )
	# Note OFF
	bytes = struct.pack( "BBB", 0x86, 0x1f, velocity )
	s.send( bytes )

# read SPI data from MCP3008 chip, 8 possible adc's (0 thru 7)
def readadc(adcnum, clockpin, mosipin, misopin, cspin):
        if ((adcnum > 7) or (adcnum < 0)):
                return -1
        GPIO.output(cspin, True)
 
        GPIO.output(clockpin, False)  # start clock low
        GPIO.output(cspin, False)     # bring CS low
 
        commandout = adcnum
        commandout |= 0x18  # start bit + single-ended bit
        commandout <<= 3    # we only need to send 5 bits here
        for i in range(5):
                if (commandout & 0x80):
                        GPIO.output(mosipin, True)
                else:
                        GPIO.output(mosipin, False)
                commandout <<= 1
                GPIO.output(clockpin, True)
                GPIO.output(clockpin, False)
 
        adcout = 0
        # read in one empty bit, one null bit and 10 ADC bits
        for i in range(12):
                GPIO.output(clockpin, True)
                GPIO.output(clockpin, False)
                adcout <<= 1
                if (GPIO.input(misopin)):
                        adcout |= 0x1
 
        GPIO.output(cspin, True)
        
        adcout >>= 1       # first bit is 'null' so drop it
        return adcout
 
# change these as desired - they're the pins connected from the
# SPI port on the ADC to the Cobbler
SPICLK = 18
SPIMISO = 23
SPIMOSI = 24
SPICS = 25
 
# set up the SPI interface pins
GPIO.setup(SPIMOSI, GPIO.OUT)
GPIO.setup(SPIMISO, GPIO.IN)
GPIO.setup(SPICLK, GPIO.OUT)
GPIO.setup(SPICS, GPIO.OUT)
 
drum_adc = 0;
 
last_read = 0       # this keeps track of the last drum value
tolerance = 5       # to keep from being jittery we'll only change
                    # volume when the pot has moved more than 5 'counts'
 
while True:
        # we'll assume that the pot didn't move
        drum_hit = False
 
        # read the analog pin
        adc_reading = readadc(drum_adc, SPICLK, SPIMOSI, SPIMISO, SPICS)
        # how much has it changed since the last read?
        drum_vel_adjust = abs(adc_reading - last_read)
 
        if DEBUG:
                print "adc_reading:", adc_reading
                print "drum_vel_adjust:", drum_vel_adjust
                print "last_read", last_read
 
        if ( drum_vel_adjust > tolerance ):
               drum_hit = True
 
        if DEBUG:
                print "drum_hit", drum_hit

	if ( drum_hit ):
		# Range should be 0 - 127
		velocity = int( 127 * (adc_reading/1024.00)  )
		if velocity > 10:
			print velocity
			send_note( velocity )
 
                # save the drum reading for the next loop
                last_read = adc_reading
 
        # hang out and do nothing for a half second
        time.sleep(0.005)

s.close()
