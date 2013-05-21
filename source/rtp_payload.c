/*

43 80 27 00 20 63 90 00 07 08 81 F1 27 7F

// Channel header
Set BJZP flags = 0000 ( if no journal ) and 0100 ( if journal )
Set length = 3

// MIDI Note ON/OFF
0x90 NOTE ON 0x80 NOTE OFF
NOTE
VELOCITY

// Journal header
Set SYAH = 0010
Set totchan = 0000
SEQ NUM

// Channel header
Set S=0
Set channel = 0000
Set H=0
Set Length = 0000000111
Set chapter flags
   N=1

// Chapter N header
Set B=1
Set len=1
Set low=1111
Set high=0001

*/

