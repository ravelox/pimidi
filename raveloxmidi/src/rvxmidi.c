#include <stdio.h>
#include <stdlib.h>

#include "net_connection.h"
#include "dns_service_publisher.h"

#include "utils.h"
#include "raveloxmidi_config.h"
#include "logging.h"

#include "rvxmidi/rvxmidi.h"


void rvxmidi_init( void )
{
	utils_init();
	config_init();
	net_ctx_init();
}

void rvxmidi_logging_init( void )
{
	logging_init();
}

void rvxmidi_config_load( char *filename )
{
	if( ! filename ) return;
	config_load_file( filename );
}

void rvxmidi_teardown( void )
{
	config_teardown();

        logging_teardown();

        utils_teardown();
        utils_mem_tracking_teardown();
        utils_pthread_tracking_teardown();
}

int rvxmidi_service_start(void)
{
	dns_service_desc_t service_desc;

        service_desc.name = config_string_get("service.name");
        service_desc.service = "_apple-midi._udp";
        service_desc.port = config_int_get("network.control.port");
        service_desc.publish_ipv4 = is_yes( config_string_get("service.ipv4"));
        service_desc.publish_ipv6 = is_yes( config_string_get("service.ipv6"));

	return dns_service_publisher_start( &service_desc ) ;
}

void rvxmidi_service_stop(void)
{
	dns_service_publisher_stop();
}
