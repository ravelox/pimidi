/*
   This file is part of raveloxmidi.

   Copyright (C) 2014 Dave Kelly

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA 
*/

#ifndef MIDI_COMMAND_H
#define MIDI_COMMAND_H

typedef struct midi_command_t {
	uint8_t	command;
	uint8_t	note;
	uint8_t velocity;
} midi_command_t;

#define MIDI_COMMAND_NOTE_ON	0x90
#define MIDI_COMMAND_NOTE_OFF	0x80

#endif
