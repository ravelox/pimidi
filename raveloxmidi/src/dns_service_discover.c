/***
  This file is a modified version of client-browse-services.c
  which uses AvahiThreadedPoll

  client-browse-services.c is part of avahi.

  avahi is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.
  avahi is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
  Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with avahi; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <sys/select.h>
#include <string.h>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>

#include <avahi-common/thread-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>

#include <pthread.h>

#include "dns_service_discover.h"

#include "logging.h"
#include "utils.h"

#include "raveloxmidi_config.h"

static pthread_mutex_t	discover_mutex;

static dns_service_t **services = NULL;
static int num_services = 0;

static AvahiThreadedPoll *threaded_poll = NULL;
static AvahiClient *client = NULL;

static void resolve_callback(
    AvahiServiceResolver *r,
    AVAHI_GCC_UNUSED AvahiIfIndex interface,
    AVAHI_GCC_UNUSED AvahiProtocol protocol,
    AvahiResolverEvent event,
    const char *name,
    const char *type,
    const char *domain,
    const char *host_name,
    const AvahiAddress *address,
    uint16_t port,
    AvahiStringList *txt,
    AvahiLookupResultFlags flags,
    AVAHI_GCC_UNUSED void* userdata) {

    assert(r);

    /* Called whenever a service has been resolved successfully or timed out */
    switch (event) {
	case AVAHI_RESOLVER_FAILURE:
		logging_printf(LOGGING_WARN, "Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
		break;
        case AVAHI_RESOLVER_FOUND: {
		char a[AVAHI_ADDRESS_STR_MAX];
		avahi_address_snprint(a, sizeof(a), address);
		dns_discover_add( name, a, port );
        }
    }
    avahi_service_resolver_free(r);
}

static void browse_callback(
    AvahiServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
    void* userdata) {
    AvahiClient *c = userdata;

    assert(b);

    /* Called whenever a new services becomes available on the LAN or is removed from the LAN */
    switch (event) {
        case AVAHI_BROWSER_FAILURE:
            logging_printf(LOGGING_WARN, "%s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
            avahi_threaded_poll_quit( threaded_poll );
            return;
        case AVAHI_BROWSER_NEW:
            if (!(avahi_service_resolver_new(c, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, 0, resolve_callback, c)))
                logging_printf( LOGGING_WARN, "Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(c)));
            break;
        case AVAHI_BROWSER_REMOVE:
            break;
        case AVAHI_BROWSER_ALL_FOR_NOW:
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
            break;
    }
}

static void discover_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {

    assert(c);

    /* Called whenever the client or server state changes */
    if (state == AVAHI_CLIENT_FAILURE) {
        logging_printf(LOGGING_WARN, "Server connection failure: %s\n", avahi_strerror(avahi_client_errno(c)));
        avahi_threaded_poll_quit(threaded_poll);
    }
}

int dns_discover_services( int use_ipv4, int use_ipv6 )
{
    AvahiServiceBrowser *sb = NULL;
    int error;
    struct timeval timeout;
    AvahiProtocol protocol;

    if (!(threaded_poll = avahi_threaded_poll_new())) {
        logging_printf(LOGGING_WARN, "dns_discover_services: Failed to create threaded poll object.\n");
        goto fail;
    }

    client = avahi_client_new(avahi_threaded_poll_get(threaded_poll), 0, discover_callback, NULL, &error);

    if (!client) {
        logging_printf(LOGGING_WARN, "dns_discover_services: Failed to create client: %s\n", avahi_strerror(error));
        goto fail;
    }

    if( use_ipv4 && use_ipv6 )
    {
        protocol = AVAHI_PROTO_UNSPEC;
    } else if ( use_ipv4 ) {
        protocol = AVAHI_PROTO_INET;
    } else {
        protocol = AVAHI_PROTO_INET6;
    }

    if (!(sb = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, protocol, "_apple-midi._udp", NULL, 0, browse_callback, client))) {
        logging_printf(LOGGING_WARN, "dns_discover_services: Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(client)));
        goto fail;
    }

    avahi_threaded_poll_start(threaded_poll);

    /* Keep the threads running until the timeout */
    timeout.tv_sec = config_int_get("discover.timeout");
    timeout.tv_usec= 0;
    select( 0, NULL, NULL, NULL, &timeout );

fail:
    if (sb) avahi_service_browser_free(sb);

    if (threaded_poll) avahi_threaded_poll_stop( threaded_poll );

    if (client) avahi_client_free(client);

    if (threaded_poll) avahi_threaded_poll_free( threaded_poll );

    return num_services;
}

dns_service_t *dns_discover_by_name( const char *name )
{
	int i = 0;

	if( num_services < 1 ) return NULL;
	if( ! services ) return NULL;

	for( i = 0; i < num_services; i++ )
	{
		if( strcmp( name, services[i]->name ) == 0 )
		{
			return services[i];
		}
	}

	return NULL;
}

void dns_discover_add( const char *name, char *address, int port)
{
	dns_service_t **new_services_list = NULL;
	dns_service_t *found_service = NULL;

	if( ! name ) return;
	if( ! address ) return;

	logging_printf( LOGGING_DEBUG, "dns_discover_add: name=\"%s\" address=[%s]:%d\n", name, address, port );
	// Check for duplicates by name
	found_service = dns_discover_by_name( name );
	if( found_service )
	{
		return;
	}

	new_services_list = (dns_service_t ** ) realloc( services, sizeof(dns_service_t) * ( num_services + 1 ) ) ;

	if( new_services_list )	
	{
		dns_service_t *new_service;
		services = new_services_list;

		new_service = (dns_service_t *)malloc( sizeof( dns_service_t ) );

		if( new_service )
		{
			new_service->name = (char *)strdup( name );
			new_service->ip_address = ( char *)strdup( address );
			new_service->port = port;
			services[ num_services ] = new_service;
			num_services++;
		} else {
			logging_printf(LOGGING_WARN, "dns_service_discover_add:  Unable to allocate memory for new service item\n");
		}
	} else {
		logging_printf(LOGGING_WARN, "dns_service_discover_add: Unable to allocate memory for new service list\n");
	}
}

void dns_discover_free_services( void )
{
	if( ! services ) return;
	if( num_services <= 0 ) return;
	for(int i = 0; i < num_services; i++)
	{
		if( services[i]->name ) free( services[i]->name );
		if( services[i]->ip_address) free( services[i]->ip_address);
		FREENULL( "dns_service_t", (void **)&(services[i]) );
	}
	free(services);
	services = NULL;
	num_services = 0;
}

void dns_discover_init( void )
{
	pthread_mutex_init( &discover_mutex , NULL );
	dns_discover_free_services();
	num_services = 0;
}

void dns_discover_teardown( void )
{
	dns_discover_free_services();
	pthread_mutex_destroy( &discover_mutex );
}

void dns_discover_dump( void )
{
	if( ! services ) return;
	if( num_services <= 0 ) return;

	for( int i = 0; i < num_services; i++ )
	{
		fprintf(stderr, "%03d) %s\t%s:%u\n", i, services[i]->name, services[i]->ip_address, services[i]->port );
	}
}
