/*
   This file is part of raveloxmidi.

   Copyright (C) 2018 Dave Kelly

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

#ifndef DNS_SERVICE_DISCOVERER_H
#define DNS_SERVICE_DISCOVERER_H

typedef struct dns_service_t {
	char *name;
	char *ip_address;
	int port;
} dns_service_t;

int dns_discover_services( int use_ipv4, int use_ipv6 );
void dns_discover_add( const char *name, char *address, int port );
dns_service_t *dns_discover_by_name( const char *name );
dns_service_t *set_fixed_ip( const char *ip );
void dns_discover_free_services( void );
void dns_discover_init( void );
void dns_discover_teardown( void );

void dns_discover_dump( void );


#endif
