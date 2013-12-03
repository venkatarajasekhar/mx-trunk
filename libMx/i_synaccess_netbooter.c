/*
 * Name:    i_synaccess_netbooter.c
 *
 * Purpose: MX interface driver for the Linkam T92, T93, T94, and T95 series
 *          of temperature controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_SYNACCESS_NETBOOTER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_motor.h"
#include "i_synaccess_netbooter.h"

MX_RECORD_FUNCTION_LIST mxi_synaccess_netbooter_record_function_list = {
	NULL,
	mxi_synaccess_netbooter_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_synaccess_netbooter_open
};

MX_RECORD_FIELD_DEFAULTS mxi_synaccess_netbooter_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_SYNACCESS_NETBOOTER_STANDARD_FIELDS
};

long mxi_synaccess_netbooter_num_record_fields
		= sizeof( mxi_synaccess_netbooter_record_field_defaults )
		    / sizeof( mxi_synaccess_netbooter_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_synaccess_netbooter_rfield_def_ptr
			= &mxi_synaccess_netbooter_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_synaccess_netbooter_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_synaccess_netbooter_create_record_structures()";

	MX_SYNACCESS_NETBOOTER *synaccess_netbooter;

	/* Allocate memory for the necessary structures. */

	synaccess_netbooter = (MX_SYNACCESS_NETBOOTER *)
				malloc( sizeof(MX_SYNACCESS_NETBOOTER) );

	if ( synaccess_netbooter == (MX_SYNACCESS_NETBOOTER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SYNACCESS_NETBOOTER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = synaccess_netbooter;

	record->record_function_list =
			&mxi_synaccess_netbooter_record_function_list;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	synaccess_netbooter->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_synaccess_netbooter_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_synaccess_netbooter_open()";

	MX_SYNACCESS_NETBOOTER *synaccess_netbooter;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL.");
	}

	synaccess_netbooter = (MX_SYNACCESS_NETBOOTER *)
					record->record_type_struct;

	if ( synaccess_netbooter == (MX_SYNACCESS_NETBOOTER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SYNACCESS_NETBOOTER pointer for record '%s' is NULL.",
			record->name);
	}

	/* Throw away any unread characters. */

	mx_status = mx_rs232_discard_unread_input(
				synaccess_netbooter->rs232_record,
				MXI_SYNACCESS_NETBOOTER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_msleep(500);

	/* Verify that the Synaccess netBooter controller is present by asking
	 * for its status.
	 */

	mx_status = mxi_synaccess_netbooter_get_status( synaccess_netbooter,
						MXI_SYNACCESS_NETBOOTER_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_synaccess_netbooter_command( MX_SYNACCESS_NETBOOTER *synaccess_netbooter,
			char *command,
			char *response,
			size_t response_buffer_length,
			mx_bool_type debug_flag )
{
	static const char fname[] = "mxi_synaccess_netbooter_command()";

	char local_buffer[80];
	char *local_response;
	size_t local_response_length;
	size_t bytes_read;
	mx_status_type mx_status;

	if ( synaccess_netbooter == (MX_SYNACCESS_NETBOOTER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SYNACCESS_NETBOOTER pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed was NULL." );
	}

	/* Send the command. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
		    fname, command, synaccess_netbooter->record->name ));
	}

	mx_status = mx_rs232_putline( synaccess_netbooter->rs232_record,
					command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the response. */

	if ( response == NULL ) {
		local_response = local_buffer;
		local_response_length = sizeof(local_buffer);
	} else {
		local_response = response;
		local_response_length = response_buffer_length;
	}

	mx_status = mx_rs232_getline( synaccess_netbooter->rs232_record,
					local_response, local_response_length,
					&bytes_read, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
#if 0
		{
			char *ptr = local_response;

			fprintf( stderr,
			"%s: response (%d bytes) = ( ", fname, (int)bytes_read);

			while (1) {
				fprintf( stderr, "%#x ", ((*ptr) & 0xff) );

				if ( (*ptr) == '\0' )
					break;

				ptr++;
			}

			fprintf( stderr, ")\n" );
		}
#endif
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
		    fname, local_response, synaccess_netbooter->record->name ));
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_synaccess_netbooter_get_status(
			MX_SYNACCESS_NETBOOTER *synaccess_netbooter,
			mx_bool_type debug_flag )
{
	static const char fname[] = "mxi_synaccess_netbooter_get_status()";

	char response[80];
	mx_status_type mx_status;

	if ( synaccess_netbooter == (MX_SYNACCESS_NETBOOTER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SYNACCESS_NETBOOTER pointer passed was NULL." );
	}

	mx_status = mxi_synaccess_netbooter_command( synaccess_netbooter, "T",
					response, sizeof(response),
					debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

