/*
   This file is part of raveloxmidi.

   Copyright (C) 2026 Dave Kelly

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

#ifndef RAVELOXMIDI_EVENTS_H
#define RAVELOXMIDI_EVENTS_H

#include <stdint.h>

#include "midi_command.h"
#include "raveloxmidi.h"

raveloxmidi_status_t raveloxmidi_events_init( raveloxmidi_context_t *context );
void raveloxmidi_events_teardown( raveloxmidi_context_t *context );
raveloxmidi_status_t raveloxmidi_events_set_callback( raveloxmidi_context_t *context, raveloxmidi_midi_event_callback_t callback, void *user_data );
raveloxmidi_status_t raveloxmidi_events_clear_callback( raveloxmidi_context_t *context );
void raveloxmidi_events_publish_midi_command( const midi_command_t *command, uint32_t originator_ssrc, int originator_alsa_device_hash );

#endif
