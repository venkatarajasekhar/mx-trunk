/*
 * Name:    mxget.c
 *
 * Purpose: Mxget reads the value of a network field from an MX server.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2013-2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_record.h"
#include "mx_multi.h"
#include "mx_net.h"

static void
print_usage( void )
{
	fprintf( stderr,
"\n"
"Usage: mxget <options> record_name.field\n"
"       mxget <options> hostname:record_name.field\n"
"       mxget <options> hostname@port:record_name.field\n"
"\n"
"where the options are:\n"
"   -a   Enable network debugging.\n"
"   -D   Start the source code debugger.\n"
"   -v   Verbose output (show the field name)\n"
	);
}

int
main( int argc, char *argv[] )
{
	char server_name[ MXU_HOSTNAME_LENGTH+1 ];
	char server_arguments[ MXU_SERVER_ARGUMENTS_LENGTH+1 ];
	char record_name[ MXU_RECORD_NAME_LENGTH+1 ];
	char field_name[ MXU_FIELD_NAME_LENGTH+1 ];

	MX_RECORD *server_record = NULL;
	int server_port;
	unsigned long server_flags;

	MX_RECORD *mx_record_list = NULL;
	MX_NETWORK_FIELD *nf = NULL;
	MX_RECORD_FIELD *local_field = NULL;
	void *value_ptr;

	char display_buffer[10000];
	int c;
	mx_bool_type network_debugging, start_debugger, verbose;
	mx_status_type mx_status;

	network_debugging = FALSE;
	start_debugger = FALSE;
	verbose = FALSE;

	if ( argc < 2 ) {
		print_usage();
		exit(1);
	}

	/* See if any command line arguments were specified. */

	while ( (c = getopt(argc, argv, "aDv")) != -1 )
	{
		switch(c) {
		case 'a':
			network_debugging = TRUE;
			break;
		case 'D':
			start_debugger = TRUE;
			break;
		case 'v':
			verbose = TRUE;
			break;
		default:
			print_usage();
			break;
		}
	}

	if ( start_debugger ) {
		mx_breakpoint();
	}

	/* If we create an MX record list right now, then we can turn on
	 * network debugging right away.  The alternative would be to allow
	 * mx_connect_to_mx_server() to automatically create an MX database
	 * for us, but that would cause us to miss the first few network
	 * messages that are sent and received.
	 */

	mx_status = mx_initialize_drivers();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	mx_record_list = mx_initialize_record_list();

	if ( mx_record_list == NULL ) {
		mx_status = mx_error( MXE_FUNCTION_FAILED, "mxget",
			"Unable to setup an MX record list." );

		return mx_status.code;
	}

	if ( network_debugging ) {
		mx_multi_set_debug_flags( mx_record_list, MXF_NETDBG_SUMMARY );
	}

	/* Parse argv[optind] to get the network field arguments. */

	mx_status = mx_parse_network_field_id( argv[optind],
				server_name, sizeof(server_name) - 1,
				server_arguments, sizeof(server_arguments) - 1,
				record_name, sizeof(record_name) - 1,
				field_name, sizeof(field_name) - 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	server_port = atoi( server_arguments );

	/* Connect to the specified server.
	 *
	 * Assigning mx_record_list to server_record gives the function
	 * mx_connect_to_mx_server() a way to find the MX record list.
	 * After returning, server_record should contain a pointer to
	 * the newly created MX server record.
	 */

	server_record = mx_record_list;

	server_flags = MXF_NETWORK_SERVER_NO_AUTO_RECONNECT
			| MXF_NETWORK_SERVER_BLOCKING_IO;

	mx_status = mx_connect_to_mx_server( &server_record,
				server_name, server_port, 0.1, server_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	mx_status = mx_set_program_name( mx_record_list, "mxget" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	/* Create an MX_NETWORK_FIELD and a local MX_RECORD_FIELD
	 * for the requested network field name.
	 */

	mx_status = mx_create_network_field( &nf, server_record,
					record_name, field_name, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	/* Connect the MX_NETWORK_FIELD to the server. */

	mx_status = mx_network_field_connect( nf );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	/* Get the value of the local MX_RECORD_FIELD. */

	local_field = nf->local_field;

	value_ptr = mx_get_field_value_pointer( local_field );

	/* Get the field value from the remote MX server. */

	mx_status = mx_get_field_array( NULL, NULL,
					nf, local_field, value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	/* Display the value of the field. */

	mx_status = mx_create_description_from_field( NULL, local_field,
				display_buffer, sizeof(display_buffer) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	if ( verbose ) {
		printf( "%s: ", argv[optind] );
	}

	printf( "%s\n", display_buffer );

	exit(0);
}

