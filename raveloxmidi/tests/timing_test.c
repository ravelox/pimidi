/* Deterministic tests for AppleMIDI clock-domain conversion. */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "net_connection.h"
#include "utils.h"

static int expect_u64( const char *label, uint64_t actual, uint64_t expected )
{
	if( actual == expected ) return 0;
	fprintf( stderr, "%s: expected %llu, got %llu\n", label,
		(unsigned long long)expected, (unsigned long long)actual );
	return 1;
}

int main( void )
{
	net_ctx_t context;
	uint64_t due;
	uint64_t before;
	uint64_t after;

	memset( &context, 0, sizeof( context ) );
	pthread_mutex_init( &context.lock, NULL );
	context.start = 1000000;

	/* Remote midpoint is 1010; local timestamp is 2010, so the offset is +1000 ticks. */
	net_ctx_update_clock( &context, 1000, 2010, 1020 );
	if( ! context.clock.valid ) return 1;
	if( context.clock.remote_to_local_offset_ticks != 1000 ) return 1;

	due = net_ctx_command_due_ns( &context, 1020, 30, 5000000 );
	if( expect_u64( "synchronized deadline", due,
		( 1000000ULL + 1020ULL + 30ULL + 1000ULL ) * APPLEMIDI_TICK_NS + 5000000ULL ) ) return 1;

	/* A second sample is filtered at 3:1 instead of replacing the prior estimate. */
	net_ctx_update_clock( &context, 2000, 3100, 2020 );
	if( context.clock.remote_to_local_offset_ticks != 1022 )
	{
		fprintf( stderr, "clock offset filter produced %lld\n",
			(long long)context.clock.remote_to_local_offset_ticks );
		return 1;
	}

	memset( &context.clock, 0, sizeof( context.clock ) );
	net_ctx_update_clock_from_initiator( &context, 3000, 2000, 3020 );
	if( context.clock.remote_to_local_offset_ticks != 1010 ||
		context.clock.remote_anchor_ticks != 2000 )
	{
		fprintf( stderr, "initiator clock sample was not mapped correctly\n" );
		return 1;
	}

	memset( &context.clock, 0, sizeof( context.clock ) );
	before = time_monotonic_ns();
	due = net_ctx_command_due_ns( &context, 0, 20, 5000000 );
	after = time_monotonic_ns();
	if( due < before + 7000000ULL || due > after + 7000000ULL )
	{
		fprintf( stderr, "unsynchronized fallback deadline is outside expected range\n" );
		return 1;
	}

	pthread_mutex_destroy( &context.lock );
	return 0;
}
