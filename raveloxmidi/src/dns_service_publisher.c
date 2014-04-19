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

#include "dns_service_publisher.h"

static AvahiEntryGroup *group = NULL;
static AvahiThreadedPoll *threaded_poll = NULL;
static AvahiClient *client = NULL;
static char *service_name_copy = NULL;
static char *service_service_copy = NULL;

static void create_services(AvahiClient *c);

static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata) {
    assert(g == group || group == NULL);
    group = g;

    /* Called whenever the entry group state changes */

    switch (state) {
        case AVAHI_ENTRY_GROUP_ESTABLISHED :
            /* The entry group has been established successfully */
            fprintf(stderr, "Service '%s' successfully established.\n", service_name_copy);
            break;

        case AVAHI_ENTRY_GROUP_COLLISION : {
            char *n;

            /* A service name collision with a remote service
             * happened. Let's pick a new name */
            n = avahi_alternative_service_name( service_name_copy );
            avahi_free(service_name_copy);
            service_name_copy = n;

            fprintf(stderr, "Service name collision, renaming service to '%s'\n", service_name_copy);

            /* And recreate the services */
            create_services(avahi_entry_group_get_client(g));
            break;
        }

        case AVAHI_ENTRY_GROUP_FAILURE :

            fprintf(stderr, "Entry group failure: %s\n", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));

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
    assert(c);

    /* If this is the first time we're called, let's create a new
     * entry group if necessary */

    if (!group)
        if (!(group = avahi_entry_group_new(c, entry_group_callback, NULL))) {
            fprintf(stderr, "avahi_entry_group_new() failed: %s\n", avahi_strerror(avahi_client_errno(c)));
            goto fail;
        }

    /* If the group is empty (either because it was just created, or
     * because it was reset previously, add our entries.  */

    if (avahi_entry_group_is_empty(group)) {
        fprintf(stderr, "Adding service '%s.%s'\n", service_name_copy, service_service_copy );

        /* Create some random TXT data */
        snprintf(r, sizeof(r), "random=%i", rand());

        if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0,  service_name_copy , service_service_copy , NULL, NULL, 5004, "test=blah", r, NULL)) < 0) {

            if (ret == AVAHI_ERR_COLLISION)
                goto collision;

            fprintf(stderr, "Failed to add %s service: %s\n", service_service_copy,avahi_strerror(ret));
            goto fail;
        }

        /* Tell the server to register the service */
        if ((ret = avahi_entry_group_commit(group)) < 0) {
            fprintf(stderr, "Failed to commit entry group: %s\n", avahi_strerror(ret));
            goto fail;
        }
    }

    return;

collision:

    /* A service name collision with a local service happened. Let's
     * pick a new name */
    n = avahi_alternative_service_name( service_name_copy );
    avahi_free(service_name_copy);
    service_name_copy = n;

    fprintf(stderr, "Service name collision, renaming service to '%s'\n", service_name_copy );

    avahi_entry_group_reset(group);

    create_services(c);
    return;

fail:
    avahi_threaded_poll_quit(threaded_poll);
}

static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
    assert(c);

    /* Called whenever the client or server state changes */

    switch (state) {
        case AVAHI_CLIENT_S_RUNNING:

            /* The server has startup successfully and registered its host
             * name on the network, so it's time to create our services */
            create_services(c);
            break;

        case AVAHI_CLIENT_FAILURE:

            fprintf(stderr, "Client failure: %s\n", avahi_strerror(avahi_client_errno(c)));
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
            ;
    }
}

void dns_service_publisher_cleanup( void )
{
	if( client)
	{
		avahi_client_free( client );
	}

	if( service_name_copy )
	{
		avahi_free( service_name_copy );
	}

	if( service_service_copy )
	{
		avahi_free( service_service_copy );
	}

	if( threaded_poll )
	{
		avahi_threaded_poll_free( threaded_poll );
	}
}

int dns_service_publisher_start( dns_service_t *service )
{
	int ret = 0;
	int error;

	if( ! service ) return 1;

	if( ! (threaded_poll = avahi_threaded_poll_new() ) ) {
		fprintf(stderr, "Unable to create publisher thread\n");
		return 1;
	}

	service_name_copy = avahi_strdup( service->name );
	service_service_copy = avahi_strdup( service->service );

	client = avahi_client_new(avahi_threaded_poll_get(threaded_poll), 0, client_callback, NULL, &error);

	if (! client) {
		fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));
		dns_service_publisher_cleanup();
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
