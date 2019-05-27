/*
   This file is part of raveloxmidi.

   Copyright (C) 2019 Dave Kelly

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

#ifndef _REMOTE_CONNECTION_H
#define _REMOTE_CONNECTION_H

void remote_connect_init( void );
void remote_connect_ok( char *name );
void remote_connect_teardown( void );
void remote_connect_sync_start( void );
void remote_connect_wait_for_thread( void );

#define DEFAULT_CONTROL_PORT 	5004
#endif
