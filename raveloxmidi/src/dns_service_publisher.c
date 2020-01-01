/***
  This file is a modified version of client-publish-service.c
  which uses AvahiThreadedPoll

  client-publish-service.c is part of avahi.

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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <avahi-client/client.h>
#include <avahi-client/publish.h>

#include <avahi-common/alternative.h>
#include <avahi-common/thread-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>

#include "config.h"

#include "dns_service_publisher.h"
#include "logging.h"

static AvahiEntryGroup *group = NULL;
static AvahiThreadedPoll *threaded_poll = NULL;
static AvahiClient *client = NULL;
static char *sd_name_copy = NULL;
static char *sd_service_copy = NULL;
static int sd_port = 0;
static int use_ipv4 = 0;
static int use_ipv6 = 0;

static void create_services(AvahiClient *c);

static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata) {
    assert(g == group || group == NULL);
    group = g;

    /* Called whenever the entry group state changes */

    switch (state) {
        case AVAHI_ENTRY_GROUP_ESTABLISHED :
            /* The entry group has been established successfully */
            logging_printf(LOGGING_INFO, "Service '%s' successfully established.\n", sd_name_copy);
            break;

        case AVAHI_ENTRY_GROUP_COLLISION : {
            char *n;

            /* A service name collision with a remote service
             * happened. Let's pick a new name */
            n = avahi_alternative_service_name( sd_name_copy );
            avahi_free(sd_name_copy);
            sd_name_copy = n;

            logging_printf(LOGGING_WARN, "Service name collision, renaming service to '%s'\n", sd_name_copy);

            /* And recreate the services */
            create_services( avahi_entry_group_get_client(g) );
            break;
        }

        case AVAHI_ENTRY_GROUP_FAILURE :

            logging_printf(LOGGING_WARN, "Entry group failure: %s\n", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));

            /* Some kind of failure happened while we were registering our services */
            avahi_threaded_poll_quit(threaded_poll);
            break;

        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
            ;
    }
}

static void create_services(AvahiClient *c) {
    char *n, r[128];
    int ret;
    AvahiProtocol protocol;
    assert(c);

    /* If this is the first time we're called, let's create a new
     * entry group if necessary */

    if (!group)
        if (!(group = avahi_entry_group_new(c, entry_group_callback, NULL))) {
            logging_printf( LOGGING_ERROR, "avahi_entry_group_new() failed: %s\n", avahi_strerror(avahi_client_errno(c)));
            goto fail;
        }

    /* If the group is empty (either because it was just created, or
     * because it was reset previously, add our entries.  */

    if (avahi_entry_group_is_empty(group)) {
        logging_printf(LOGGING_INFO, "Adding service '%s.%s'\n", sd_name_copy, sd_service_copy );

        /* Create some random TXT data */
        snprintf(r, sizeof(r), "random=%i", rand());

	if( use_ipv4 && use_ipv6 )
	{
		protocol = AVAHI_PROTO_UNSPEC;
	} else if( use_ipv4 ) {
		protocol = AVAHI_PROTO_INET;
	} else {
		protocol = AVAHI_PROTO_INET6;
	}

        if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, protocol, 0,  sd_name_copy , sd_service_copy , NULL, NULL,  sd_port, "test=blah", r, NULL)) < 0) {

            if (ret == AVAHI_ERR_COLLISION)
                goto collision;

            logging_printf(LOGGING_ERROR, "Failed to add %s service: %s\n", sd_service_copy,avahi_strerror(ret));
            goto fail;
        }

        /* Tell the server to register the service */
        if ((ret = avahi_entry_group_commit(group)) < 0) {
            logging_printf(LOGGING_ERROR, "Failed to commit entry group: %s\n", avahi_strerror(ret));
            goto fail;
        }
    }

    return;

collision:

    /* A service name collision with a local service happened. Let's
     * pick a new name */
    n = avahi_alternative_service_name( sd_name_copy );
    avahi_free(sd_name_copy);
    sd_name_copy = n;

    logging_printf(LOGGING_WARN, "Service name collision, renaming service to '%s'\n", sd_name_copy );

    avahi_entry_group_reset(group);

    create_services(c);
    return;

fail:
    avahi_threaded_poll_quit(threaded_poll);
}

static void publish_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
    assert(c);

    /* Called whenever the client or server state changes */

    switch (state) {
        case AVAHI_CLIENT_S_RUNNING:

            /* The server has startup successfully and registered its host
             * name on the network, so it's time to create our services */
            create_services(c);
            break;

        case AVAHI_CLIENT_FAILURE:

            logging_printf(LOGGING_ERROR, "Client failure: %s\n", avahi_strerror(avahi_client_errno(c)));
            avahi_threaded_poll_quit(threaded_poll);

            break;

        case AVAHI_CLIENT_S_COLLISION:

            /* Let's drop our registered services. When the server is back
             * in AVAHI_SERVER_RUNNING state we will register them
             * again with the new host name. */

        case AVAHI_CLIENT_S_REGISTERING:

            /* The server records are now being established. This
             * might be caused by a host name change. We need to wait
             * for our own records to register until the host name is
             * properly esatblished. */

            if (group)
                avahi_entry_group_reset(group);

            break;

        case AVAHI_CLIENT_CONNECTING:
		break;

	default:
	   logging_printf(LOGGING_DEBUG, "avahi publish_callback: unknown state\n");
	   break;
    }
}

void dns_service_publisher_cleanup( void )
{
	if( group )
	{
		avahi_entry_group_free( group );
		group = NULL;
	}

	if( client)
	{
		avahi_client_free( client );
		client = NULL;
	}

	if( threaded_poll )
	{
		avahi_threaded_poll_free( threaded_poll );
		threaded_poll = NULL;
	}

	if( sd_name_copy )
	{
		avahi_free( sd_name_copy );
		sd_name_copy = NULL;
	}

	if( sd_service_copy )
	{
		avahi_free( sd_service_copy );
		sd_service_copy = NULL;
	}
}

int dns_service_publisher_start( dns_service_desc_t *service_desc )
{
	int ret = 0;
	int error;

	if( ! service_desc ) return 1;

	if( ! (threaded_poll = avahi_threaded_poll_new() ) ) {
		logging_printf(LOGGING_ERROR, "Unable to create publisher thread\n");
		return 1;
	}

	sd_name_copy = avahi_strdup( service_desc->name );
	sd_service_copy = avahi_strdup( service_desc->service );
	sd_port = service_desc->port;

	use_ipv4 = service_desc->publish_ipv4;
	use_ipv6 = service_desc->publish_ipv6;

	client = avahi_client_new(avahi_threaded_poll_get(threaded_poll), 0, publish_callback, NULL, &error);

	if (! client) {
		logging_printf(LOGGING_ERROR, "Failed to create client: %s\n", avahi_strerror(error));
		return 1;
	}

	avahi_threaded_poll_start( threaded_poll );

	return ret;
}

void dns_service_publisher_stop( void )
{
	if( threaded_poll )
	{
		avahi_threaded_poll_stop( threaded_poll );
	}
	dns_service_publisher_cleanup();
}
