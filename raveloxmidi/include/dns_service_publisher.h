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

#ifndef DNS_SERVICE_PUBLISHER_H
#define DNS_SERVICE_PUBLISHER_H

typedef struct dns_service_desc_t {
	char *name;
	char *service;
	int port;
	int publish_ipv4;
	int publish_ipv6;
} dns_service_desc_t;

void dns_service_publisher_cleanup( void );
int dns_service_publisher_start( dns_service_desc_t *service );
void dns_service_publisher_stop( void );

#endif
