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

#ifndef _LOGGING_H
#define _LOGGING_H

#include <stdarg.h>

typedef struct {
	char *name;
	int value;
} name_map_t;

#define LOGGING_DEBUG	0
#define LOGGING_INFO	1
#define LOGGING_NORMAL	2
#define LOGGING_WARN	3
#define LOGGING_ERROR	4

#ifdef INSIDE_LOGGING
int logging_threshold = 3;
int logging_hex_dump = 0;
int logging_enabled = 0;
#else
extern int logging_threshold;
extern int logging_hex_dump;
extern int logging_enabled;
#define DEBUG_ONLY	if( (logging_enabled==0) || (logging_threshold!=LOGGING_DEBUG)) return;
#define INFO_ONLY	if( (logging_enabled==0) || (logging_threshold>LOGGING_INFO)) return;
#define HEX_DUMP_ENABLED	if(logging_hex_dump==0) return;
#endif

int logging_name_to_value(name_map_t *map, const char *name);
char *logging_value_to_name(name_map_t *map, int value);
void logging_printf(int level, const char *format, ...);
void logging_init(void);
void logging_teardown(void);
void logging_prefix_enable(void);
void logging_prefix_disable(void);
int logging_get_threshold(void);

#endif
