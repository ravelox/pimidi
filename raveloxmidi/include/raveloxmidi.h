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

#ifndef RAVELOXMIDI_H
#define RAVELOXMIDI_H

#include <stddef.h>
#include <stdint.h>

#if defined _WIN32 || defined __CYGWIN__
#  if defined RAVELOXMIDI_BUILDING_LIBRARY
#    define RAVELOXMIDI_API __declspec(dllexport)
#  else
#    define RAVELOXMIDI_API __declspec(dllimport)
#  endif
#elif defined __GNUC__ && __GNUC__ >= 4
#  define RAVELOXMIDI_API __attribute__((visibility("default")))
#else
#  define RAVELOXMIDI_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct raveloxmidi_context raveloxmidi_context_t;

typedef enum raveloxmidi_status_t {
	RAVELOXMIDI_OK = 0,
	RAVELOXMIDI_ERROR,
	RAVELOXMIDI_ERROR_INVALID_ARGUMENT,
	RAVELOXMIDI_ERROR_NO_MEMORY,
	RAVELOXMIDI_ERROR_INVALID_STATE,
	RAVELOXMIDI_ERROR_NOT_RUNNING
} raveloxmidi_status_t;

typedef enum raveloxmidi_event_type_t {
	RAVELOXMIDI_EVENT_NOTE_OFF,
	RAVELOXMIDI_EVENT_NOTE_ON,
	RAVELOXMIDI_EVENT_CONTROL_CHANGE,
	RAVELOXMIDI_EVENT_PROGRAM_CHANGE,
	RAVELOXMIDI_EVENT_RAW_MIDI
} raveloxmidi_event_type_t;

RAVELOXMIDI_API const char *raveloxmidi_version( void );

RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_create( raveloxmidi_context_t **context );
RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_set_config( raveloxmidi_context_t *context, const char *key, const char *value );
RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_start( raveloxmidi_context_t *context );
RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_stop( raveloxmidi_context_t *context );
RAVELOXMIDI_API void raveloxmidi_context_free( raveloxmidi_context_t **context );

#ifdef __cplusplus
}
#endif

#endif
