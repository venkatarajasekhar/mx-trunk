/*
 * Name:    i_compumotor.c
 *
 * Purpose: MX driver for Compumotor 6000 and 6K series motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2006, 2010, 2012-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_COMPUMOTOR_INTERFACE_DEBUG		FALSE

#define MXI_COMPUMOTOR_INTERFACE_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_ascii.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_cfn.h"
#include "mx_rs232.h"
#include "i_compumotor.h"
#include "d_compumotor.h"

MX_RECORD_FUNCTION_LIST mxi_compumotor_record_function_list = {
	mxi_compumotor_initialize_driver,
	mxi_compumotor_create_record_structures,
	mxi_compumotor_finish_record_initialization,
	mxi_compumotor_delete_record,
	NULL,
	mxi_compumotor_open,
	mxi_compumotor_close,
	NULL,
	mxi_compumotor_resynchronize,
	mxi_compumotor_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_compumotor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_COMPUMOTOR_INTERFACE_STANDARD_FIELDS
};

long mxi_compumotor_num_record_fields
		= sizeof( mxi_compumotor_record_field_defaults )
			/ sizeof( mxi_compumotor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_compumotor_rfield_def_ptr
			= &mxi_compumotor_record_field_defaults[0];

static mx_status_type mxi_compumotor_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

/* A private function for the use of the driver. */

static mx_status_type
mxi_compumotor_get_pointers( MX_RECORD *record,
			MX_COMPUMOTOR_INTERFACE **compumotor_interface,
			const char *calling_fname )
{
	static const char fname[] = "mxi_compumotor_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( compumotor_interface == (MX_COMPUMOTOR_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_COMPUMOTOR_INTERFACE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
					record->record_type_struct;

	if ( *compumotor_interface == (MX_COMPUMOTOR_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_COMPUMOTOR_INTERFACE pointer for record '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_compumotor_run_program( MX_COMPUMOTOR_INTERFACE *compumotor_interface,
				char *program_type,
				char *program_filename )
{
	static const char fname[] = "mxi_compumotor_run_program()";

	int saved_errno;
	FILE *program_file;
	char program_line[200];
	char *ptr;
	long i, length;
	mx_bool_type suppress_comments;
	mx_status_type mx_status;

	if ( compumotor_interface->interface_flags
			& MXF_COMPUMOTOR_SUPPRESS_COMMENTS )
	{
		suppress_comments = TRUE;
	} else {
		suppress_comments = FALSE;
	}

	/* Try to open the startup program file. */

	program_file = mx_cfn_fopen( MX_CFN_CONFIG, program_filename, "r" );

	if ( program_file == ( FILE * ) NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt by record '%s' to open %s program "
		"file '%s' failed.  Errno = %d, error message = '%s'.",
			compumotor_interface->record->name,
			program_type, program_filename,
			saved_errno, strerror(saved_errno) );
	}

	/* Read the startup program one line at a time
	 * and send it to the Compumotor controller.
	 */

	while (1) {
		mx_fgets( program_line, sizeof(program_line),
						program_file );

		if ( feof(program_file) ) {
			fclose( program_file );
			program_file = NULL;
			break;		/* Exit the while() loop. */
		} else
		if ( ferror(program_file) ) {
			saved_errno = errno;
			fclose( program_file );

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while reading from "
			"%s program file '%s' for record '%s'.  "
			"Errno = %d, error message = '%s'.",
				program_type, program_filename,
				compumotor_interface->record->name,
				saved_errno, strerror(saved_errno) );
		}

		if ( suppress_comments ) {
			/* Look for the comment character ';'. */

			ptr = strchr( program_line, ';' );

			if ( ptr != NULL ) {
				*ptr = '\0';
			}

			/* Suppress trailing spaces or tabs. */

			length = strlen( program_line );

			for ( i = length-1; i >= 0; i-- ) {

				if ( program_line[i] == ' ' ) {
					program_line[i] = '\0';
				} else
				if ( program_line[i] == '\t' ) {
					program_line[i] = '\0';
				} else {
					break;	/* Exit the for() loop. */
				}
			}

			if ( strlen( program_line ) == 0 ) {
				/* If we have an empty line, do not send it
				 * to the controller and go back to the top
				 * of the while() loop.
				 */

				continue;
			}
		}

		mx_status = mxi_compumotor_command(
				compumotor_interface,
				program_line,
				NULL, 0,
				MXI_COMPUMOTOR_INTERFACE_DEBUG );

		if ( mx_status.code != MXE_SUCCESS ) {
			fclose( program_file );
			return mx_status;
		}
	}

	/* This should never happen, but we check anyway. */

	if ( program_file != NULL ) {
		fclose( program_file );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The FILE pointer for file '%s' used by record '%s' "
		"was not NULL at a time when the file should "
		"already be closed.",
			program_filename,
			compumotor_interface->record->name );
	}

	/* If the commands we sent generated any output,
	 * we throw the output away now.
	 */

	mx_msleep(100);

	mx_status = mx_rs232_discard_unread_input(
					compumotor_interface->rs232_record,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

	return mx_status;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_compumotor_initialize_driver( MX_DRIVER *driver )
{
	static const char fname[] = "mxi_compumotor_initialize_driver()";

	MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
	long num_controllers_varargs_cookie;
	mx_status_type mx_status;

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}

	mx_status = mx_find_record_field_defaults_index( driver,
						"num_controllers",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index,
				0, &num_controllers_varargs_cookie);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults( driver,
						"num_axes", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_controllers_varargs_cookie;

	mx_status = mx_find_record_field_defaults( driver,
						"controller_number", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_controllers_varargs_cookie;

	mx_status = mx_find_record_field_defaults( driver,
						"controller_type", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_controllers_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_compumotor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_compumotor_create_record_structures()";

	MX_COMPUMOTOR_INTERFACE *compumotor_interface;

	/* Allocate memory for the necessary structures. */

	compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
				malloc( sizeof(MX_COMPUMOTOR_INTERFACE) );

	if ( compumotor_interface == (MX_COMPUMOTOR_INTERFACE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_COMPUMOTOR_INTERFACE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = compumotor_interface;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	compumotor_interface->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_compumotor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_compumotor_finish_record_initialization()";

	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	long i, j, num_controllers;
#if 0
	mx_status_type mx_status;
#endif

	compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
					record->record_type_struct;

	num_controllers = compumotor_interface->num_controllers;

	/* Allocate the private arrays that the driver users. */

	compumotor_interface->motor_array
		= ( MX_RECORD *(*)[MX_MAX_COMPUMOTOR_AXES] )
			malloc( num_controllers * MX_MAX_COMPUMOTOR_AXES
			* sizeof( MX_RECORD * ) );

	if ( compumotor_interface->motor_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate motor_array for Compumotor interface '%s'.",
			record->name );
	}

	if ( compumotor_interface->num_controllers <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Compumotor interface '%s' is configured for an illegal number "
		"of controllers (%ld).  The minimum value allowed is 1.",
			compumotor_interface->record->name,
			compumotor_interface->num_controllers );
	}

	/* Initialize the arrays. */

	for ( i = 0; i < compumotor_interface->num_controllers; i++ ) {
		compumotor_interface->controller_type[i] = 0;

		for ( j = 0; j < MX_MAX_COMPUMOTOR_AXES; j++ ) {
			compumotor_interface->motor_array[i][j] = NULL;
		}
	}

#if 0
	record->event_time_manager = (MX_EVENT_TIME_MANAGER *)
		malloc( sizeof( MX_EVENT_TIME_MANAGER ) );

	if ( record->event_time_manager == (MX_EVENT_TIME_MANAGER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
  "Ran out of memory trying to allocate an MX_EVENT_TIME_MANAGER structure." );
	}

	mx_status = mx_set_event_interval( record, 0.010 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_compumotor_delete_record( MX_RECORD *record )
{
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;

        if ( record == NULL ) {
                return MX_SUCCESSFUL_RESULT;
        }

	compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
					record->record_type_struct;

        if ( compumotor_interface != NULL ) {
		if ( compumotor_interface->motor_array != NULL ) {
			free( compumotor_interface->motor_array );
		}
                free( compumotor_interface );

                record->record_type_struct = NULL;
        }
        if ( record->record_class_struct != NULL ) {
                free( record->record_class_struct );

                record->record_class_struct = NULL;
        }
        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_compumotor_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_compumotor_open()";

	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	MX_RECORD *rs232_record;
	MX_RS232 *rs232;
	long i;
	unsigned long interface_flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
					(record->record_type_struct);

	if ( compumotor_interface == (MX_COMPUMOTOR_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_COMPUMOTOR_INTERFACE pointer for record '%s' is NULL.",
		record->name);
	}

	/* Are the line terminators set correctly? */

	rs232_record = compumotor_interface->rs232_record;

	if ( rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"rs232_record pointer for Compumotor interface '%s' is NULL.",
			record->name );
	}

	rs232 = (MX_RS232 *) rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RS232 pointer for RS-232 record '%s' is NULL.",
			compumotor_interface->rs232_record->name );
	}

#if 0
	if ( (rs232->read_terminators != 0x0d0a)
	  || (rs232->write_terminators != 0x0d0a) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The Compumotor interface '%s' requires that the line terminators "
	"of RS-232 record '%s' be a carriage return followed by a line feed.  "
	"Instead saw read terminator %#x and write terminator %#x.",
		record->name, compumotor_interface->rs232_record->name,
		rs232->read_terminators, rs232->write_terminators );
	}
#endif

	/* If requested, attempt to automatically configure the parameter
	 * settings needed for correct communication handshaking with the
	 * Compumotor controller.  The most important ones are
	 *     EOT13,10,0
	 *     ERRLVL1
	 *     ECHO1
	 *     MA11111111
	 */

	interface_flags = compumotor_interface->interface_flags;

	if ( interface_flags & MXF_COMPUMOTOR_AUTO_COMMUNICATION_CONFIG ) {

#if MXI_COMPUMOTOR_INTERFACE_DEBUG
		MX_DEBUG(-2,
	  ("%s: Attempting automatic communication config for controller '%s'.",
			fname, record->name));
#endif

		/* Since we do not necessarily have correct handshaking set up
		 * yet, we send the first three commands in the blind.
		 */

		(void) mx_rs232_discard_unwritten_output( rs232_record,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

		(void) mx_rs232_discard_unread_input( rs232_record,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

		mx_status = mx_rs232_putline( rs232_record,
					"!EOT13,10,0", NULL,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

		mx_status = mx_rs232_putline( rs232_record,
					"!ERRLVL1", NULL,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

		mx_status = mx_rs232_putline( rs232_record,
					"!ECHO1", NULL,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

		mx_status = mx_rs232_putline( rs232_record,
					"!MA11111111", NULL,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

		if ( interface_flags & MXF_COMPUMOTOR_KILL_ON_STARTUP ) {
			mx_status = mx_rs232_putline( rs232_record,
					"!K", NULL,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

		}

		/* If all went well, the controllers have been configured
		 * to handshake correctly with the MX driver.  We discard
		 * any responses from the controller so far.
		 */

		mx_msleep(100);

		mx_status = mx_rs232_discard_unread_input( rs232_record,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* If requested, send an '!ADDR1' command to automatically configure
	 * unit addresses.  This will not do the right thing for an RS-485
	 * multi-drop configuration, so it must be optional.
	 */

	if ( compumotor_interface->interface_flags
		& MXF_COMPUMOTOR_AUTO_ADDRESS_CONFIG )
	{
		/* If we are using autoaddressing, the controllers must be 
		 * numbered from 1 to num_controllers in the controller_number
		 * array for consistency.  If they are not, generate an
		 * error message.
		 */

		for ( i = 0; i < compumotor_interface->num_controllers; i++ ) {
			if ( compumotor_interface->controller_number[i] != i+1 )
			{
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"If MXF_COMPUMOTOR_AUTO_ADDRESS_CONFIG is set for record '%s', then the "
"controller addresses in the database must be in order from 1 to %ld.",
				compumotor_interface->record->name,
				compumotor_interface->num_controllers );
			}
		}

		/* Send the automatic configuration command. */

		(void) mx_rs232_discard_unwritten_output( rs232_record,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

		mx_status = mx_rs232_putline( rs232_record,
					"!ADDR1", NULL,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			break;
		case MXE_NOT_READY:
		case MXE_INTERFACE_IO_ERROR:
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Cannot set addresses for Compumotor interface '%s' on RS-232 port '%s'.  "
"Is it turned on?", record->name, compumotor_interface->rs232_record->name );
		default:
			return mx_status;
		}
	}

	/* Synchronize with the controllers. */

	mx_status = mxi_compumotor_resynchronize( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If a startup program is defined, then run it. */

	if ( strlen( compumotor_interface->startup_program ) > 0 ) {
		mx_status = mxi_compumotor_run_program(
				compumotor_interface,
				"startup",
				compumotor_interface->startup_program );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_compumotor_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_compumotor_close()";

	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
					(record->record_type_struct);

	if ( compumotor_interface == (MX_COMPUMOTOR_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_COMPUMOTOR_INTERFACE pointer for record '%s' is NULL.",
		record->name);
	}

	/* Get rid of any remaining characters in the input and output
	 * buffers and do it quietly.
	 */

	(void) mx_rs232_discard_unwritten_output(
				compumotor_interface->rs232_record, FALSE );

	mx_status = mx_rs232_discard_unread_input(
				compumotor_interface->rs232_record, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If a shutdown program is defined, then run it. */

	if ( strlen( compumotor_interface->shutdown_program ) > 0 ) {
		mx_status = mxi_compumotor_run_program(
				compumotor_interface,
				"shutdown",
				compumotor_interface->shutdown_program );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_compumotor_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_compumotor_resynchronize()";

	MX_COMPUMOTOR_INTERFACE *compumotor_interface = NULL;
	char command[80], response[80];
	char version_string[80], type_string[80];
	long i, j;
	int num_items, command_flags;
	mx_status_type mx_status;

	mx_status = mxi_compumotor_get_pointers( record,
				&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Throw away any pending input and output on the 
	 * Compumotor RS-232 port.
	 */

	mx_status = mx_rs232_discard_unwritten_output(
					compumotor_interface->rs232_record,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
	case MXE_UNSUPPORTED:
		break;		/* Continue on. */
	default:
		return mx_status;
	}

	mx_status = mx_rs232_discard_unread_input(
					compumotor_interface->rs232_record,
					MXI_COMPUMOTOR_INTERFACE_DEBUG);

	/* Verify that each of the controllers are there by asking them
	 * for their revision number.
	 */

	command_flags = MXI_COMPUMOTOR_INTERFACE_DEBUG
			| MXF_COMPUMOTOR_NO_RECURSION;

	for ( i = 0; i < compumotor_interface->num_controllers; i++ ) {

		snprintf( command, sizeof(command), "%ld_!TREV",
				compumotor_interface->controller_number[i] );
	
		mx_status = mxi_compumotor_command( compumotor_interface,
				command, response, sizeof response,
				command_flags );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			break;
		case MXE_NOT_READY:
		case MXE_INTERFACE_IO_ERROR:
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Cannot communicate with Compumotor controller %ld on RS-232 port '%s'.  "
"Is it turned on?", i+1, compumotor_interface->rs232_record->name );
			break;
		default:
			return mx_status;
			break;
		}

		/* Attempt to determine the model of each type of
		 * controller.  The effectiveness of this logic is
		 * limited by the fact that I only have access to
		 * 6K and Zeta 6104 controllers.  (W. Lavender)
		 */

		compumotor_interface->controller_type[i]
						= MXT_COMPUMOTOR_UNKNOWN;

		num_items = sscanf( response, "%s %s",
					version_string, type_string );

		if ( num_items == 1 ) {
			compumotor_interface->controller_type[i]
						= MXT_COMPUMOTOR_6000_SERIES;

		} else if ( num_items == 2 ) {
			if ( strcmp( type_string, "6K" ) == 0 ) {

				compumotor_interface->controller_type[i]
						= MXT_COMPUMOTOR_6K;
			} else
			if ( strcmp( type_string, "ZETA6000" ) == 0 ) {

				compumotor_interface->controller_type[i]
						= MXT_COMPUMOTOR_ZETA_6000;
			} else
			if ( strcmp( type_string, "6104" ) == 0 ) {

				compumotor_interface->controller_type[i]
						= MXT_COMPUMOTOR_ZETA_6000;
			} else {
				compumotor_interface->controller_type[i]
						= MXT_COMPUMOTOR_6000_SERIES;
			}
		}

		if ( compumotor_interface->controller_type[i]
				== MXT_COMPUMOTOR_UNKNOWN )
		{
			(void) mx_error( MXE_UNPARSEABLE_STRING, fname,
	"Compumotor interface '%s' had an unrecognized response to "
	"the TREV command.  TREV response = '%s'",
			record->name, response );
		}
	}

	/* Try to enable all of the axes for each controller. */

	for ( i = 0; i < compumotor_interface->num_controllers; i++ ) {

		snprintf( command, sizeof(command), "%ld_!DRIVE",
				compumotor_interface->controller_number[i] );

		for ( j = 0; j < compumotor_interface->num_axes[i]; j++ ) {

			strlcat( command, "1", sizeof(command) );
		}

		(void) mxi_compumotor_command( compumotor_interface,
					command, NULL, 0,
					command_flags );
	}

	/* Discard any unread output from the controller. */

	mx_msleep(100);

	mx_status = mx_rs232_discard_unread_input(
					compumotor_interface->rs232_record,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_compumotor_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxi_compumotor_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_COMPUMOTOR_COMMAND:
		case MXLV_COMPUMOTOR_RESPONSE:
		case MXLV_COMPUMOTOR_COMMAND_WITH_RESPONSE:
			record_field->process_function
					    = mxi_compumotor_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxi_compumotor_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxi_compumotor_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
					record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_COMPUMOTOR_RESPONSE:
			/* Nothing to do since the necessary string is
			 * already stored in the 'response' field.
			 */

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_COMPUMOTOR_COMMAND:
			mx_status = mxi_compumotor_command(
				compumotor_interface,
				compumotor_interface->command,
				NULL, 0, MXI_COMPUMOTOR_INTERFACE_DEBUG );

			break;
		case MXLV_COMPUMOTOR_COMMAND_WITH_RESPONSE:
			mx_status = mxi_compumotor_command(
				compumotor_interface,
				compumotor_interface->command,
				compumotor_interface->response,
				MX_COMPUMOTOR_MAX_COMMAND_LENGTH,
				MXI_COMPUMOTOR_INTERFACE_DEBUG );

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}

/* === Functions specific to this driver. === */

#if MXI_COMPUMOTOR_INTERFACE_DEBUG_TIMING	
#include "mx_hrt_debug.h"
#endif

MX_EXPORT mx_status_type
mxi_compumotor_command( MX_COMPUMOTOR_INTERFACE *compumotor_interface,
		char *command, char *response, size_t response_buffer_length,
		int command_flags )
{
	static const char fname[] = "mxi_compumotor_command()";

	MX_RS232 *rs232;
	unsigned long interface_flags;
	unsigned long sleep_ms, num_bytes_available;
	long num_command_attempts;
	long i, max_response_attempts;
	size_t command_length, response_length;
	char c;
	char echoed_command_string[200];
	mx_bool_type debug_flag, debug_getchar_flag;
	mx_status_type mx_status;
#if MXI_COMPUMOTOR_INTERFACE_DEBUG_TIMING	
	MX_HRT_RS232_TIMING command_timing, response_timing;
#endif

	debug_flag = FALSE;

	MX_DEBUG(2,("%s invoked.", fname));

	if ( compumotor_interface == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_COMPUMOTOR_INTERFACE pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}
	if ( compumotor_interface->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The rs232_record pointer for record '%s' is NULL.",
			compumotor_interface->record->name );
	}

	rs232 = (MX_RS232 *)
			compumotor_interface->rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RS232 pointer for record '%s' used by '%s' is NULL.",
			compumotor_interface->rs232_record->name,
			compumotor_interface->record->name );
	}

	interface_flags = compumotor_interface->interface_flags;

	if ( interface_flags & MXF_COMPUMOTOR_ECHO_ON ) {

		command_length = strlen( command );

		if ( command_length > ( sizeof(echoed_command_string) - 1 ) ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Compumotor interface record '%s' has ECHO set to 1.  When ECHO is "
	"set to 1, commands are limited to a maximum of %ld characters, but "
	"the command string passed is %ld characters long.  command = '%s'",
			compumotor_interface->record->name,
			(long) sizeof( echoed_command_string ) - 1L,
			(long) command_length, command );
		}
	}

	if ( command_flags & MXI_COMPUMOTOR_INTERFACE_DEBUG ) {
		debug_flag = TRUE;
	} else
	if ( interface_flags & MXF_COMPUMOTOR_DEBUG_SERIAL ) {
	} else {
		debug_flag = FALSE;
	}

	if ( command_flags & MXF_COMPUMOTOR_NO_RECURSION ) {
		num_command_attempts = 1;
	} else
	if ( interface_flags & MXF_COMPUMOTOR_AUTOMATIC_RESYNCHRONIZE ) {
		num_command_attempts = 2;
	} else {
		num_command_attempts = 1;
	}

	if ( interface_flags & MXF_COMPUMOTOR_DEBUG_SERIAL_GETCHAR ) {
		if ( interface_flags & MXF_COMPUMOTOR_DEBUG_SERIAL ) {
			debug_getchar_flag = TRUE;
		} else
		if ( rs232->rs232_flags & MXF_232_DEBUG_SERIAL ) {
			debug_getchar_flag = TRUE;
		} else
		if ( rs232->rs232_flags & MXF_232_DEBUG_SERIAL_VERBOSE ) {
			debug_getchar_flag = TRUE;
		} else {
			debug_getchar_flag = FALSE;
		}
	} else {
		debug_getchar_flag = FALSE;
	}

	/* Send the command string. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, compumotor_interface->record->name));
	}

	while ( num_command_attempts > 0 ) {

	    num_command_attempts--;

#if MXI_COMPUMOTOR_INTERFACE_DEBUG_TIMING	
	    MX_HRT_RS232_START_COMMAND( command_timing, 2 + strlen(command) );
#endif

	    mx_status = mx_rs232_putline( compumotor_interface->rs232_record,
					command, NULL, 0 );

#if MXI_COMPUMOTOR_INTERFACE_DEBUG_TIMING
	    MX_HRT_RS232_END_COMMAND( command_timing );
#endif

	    if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_COMPUMOTOR_INTERFACE_DEBUG_TIMING
	MX_HRT_RS232_COMMAND_RESULTS( command_timing, command, fname );
#endif

	    if ( interface_flags & MXF_COMPUMOTOR_ECHO_ON ) {

		/* If the Compumotor is configured to echo commands,
		 * read the echoed command string.
		 */

#if MXI_COMPUMOTOR_INTERFACE_DEBUG_TIMING
		MX_HRT_RS232_START_RESPONSE( response_timing,
					compumotor_interface->rs232_record );
#endif

		mx_status = mx_rs232_getline(compumotor_interface->rs232_record,
					echoed_command_string,
					sizeof( echoed_command_string ) - 1L,
					NULL, 0 );

#if MXI_COMPUMOTOR_INTERFACE_DEBUG_TIMING
		MX_HRT_RS232_END_RESPONSE( response_timing,
					strlen(echoed_command_string) );

		MX_HRT_RS232_RESPONSE_RESULTS( response_timing,
					echoed_command_string, fname );
#endif

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	    }

	    /* Get the response, if one is expected. */

	    i = 0;

	    max_response_attempts = 1000;
	    sleep_ms = 1;

	    if ( response == NULL ) {

		/* If we are not checking for a response, then we have no
		 * way to tell whether or not the command succeeded.  That
		 * means that we have no justification for retrying a
		 * command, so we suppress command retries.
		 */

		num_command_attempts = 0;

	    } else {
		/* If we get here, a response is expected. */

#if MXI_COMPUMOTOR_INTERFACE_DEBUG_TIMING
#if 0
		MX_HRT_RS232_START_RESPONSE( response_timing,
					compumotor_interface->rs232_record );
#else
		MX_HRT_RS232_START_RESPONSE( response_timing, NULL );
#endif
#endif

		/* Any text sent by the Compumotor controller should
		 * be prefixed by an asterisk character, so to begin
		 * with, we try to read and discard characters until
		 * we see an asterisk.
		 */

		c = '\0';

		for ( i = 0; i < max_response_attempts; i++ ) {

			mx_status = mx_rs232_num_input_bytes_available(
					compumotor_interface->rs232_record,
					&num_bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( num_bytes_available == 0 ) {
				/* Sleep and then go back to the top
				 * of the for() loop.
				 */

				mx_msleep(sleep_ms);
				continue;
			}

			mx_status = mx_rs232_getchar(
					compumotor_interface->rs232_record,
					&c, MXF_232_WAIT );

			if ( debug_getchar_flag ) {
				MX_DEBUG(-2,
				("%s: mx_rs232_getchar() = '%c' %#x",
					fname, c, c));
			}

			if ( mx_status.code != MXE_SUCCESS ) {
				response[0] = '\0';

				if ( debug_flag ) {
					MX_DEBUG(-2,
		("%s failed while waiting for an asterisk character from '%s'.",
				fname, compumotor_interface->record->name));
				}
				return mx_status;
			}

			/* Did we see the asterisk character? */

			if ( c != '*' ) {
				/* If not, sleep and then go back to
				 * the top of the for() loop.
				 */

				mx_msleep(sleep_ms);
				continue;	
			}

			/* Read in the Compumotor response. */

			mx_status = mx_rs232_getline(
					compumotor_interface->rs232_record,
					response, response_buffer_length,
					NULL, 0 );

			if ( mx_status.code == MXE_SUCCESS ) {
				break;		/* Exit the for() loop. */

			} else if ( mx_status.code != MXE_NOT_READY ) {
				MX_DEBUG(-2,
		("*** Exiting with status = %ld for Compumotor interface '%s'.",
			mx_status.code, compumotor_interface->record->name));

				return mx_status;
			}
			mx_msleep(sleep_ms);

		}   /* End of response attempt loop (i) */

		if ( i >= max_response_attempts ) {

		    if ( num_command_attempts > 0 ) {
			mx_warning( "Resynchronizing '%s'.",
					compumotor_interface->record->name );

			mx_status = mxi_compumotor_resynchronize(
					compumotor_interface->record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_warning( "Resynchronization of '%s' complete.",
					compumotor_interface->record->name );
		    } else {
			mx_status = mx_rs232_discard_unread_input(
					compumotor_interface->rs232_record,
					debug_flag );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Failed at attempt to discard unread characters in buffer for record '%s'",
					compumotor_interface->record->name );
			}

			return mx_error( MXE_TIMED_OUT, fname,
				"No response seen to '%s' command after "
				"%ld attempts to read from Compumotor "
				"interface '%s'.",
					command, max_response_attempts,
					compumotor_interface->record->name );
		    }
		} else {

		    /* Successfully got a response. */

		    num_command_attempts = 0;    /* No more attempts needed. */

		    /* Sometimes, the response string after the asterisk '*'
		     * character starts with a newline character.  If this
		     * has happened, strip off the leading newline.
		     */

		    if ( response[0] == MX_LF ) {
			response_length = strlen(response);

			memmove( response, response+1, response_length );
		    }

#if MXI_COMPUMOTOR_INTERFACE_DEBUG_TIMING
		    MX_HRT_RS232_END_RESPONSE( response_timing,
						strlen(response) );

		    MX_HRT_TIME_BETWEEN_MEASUREMENTS( command_timing,
						response_timing, fname );

		    MX_HRT_RS232_RESPONSE_RESULTS( response_timing,
						response, fname);
#endif

		    if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
				fname, response,
				compumotor_interface->record->name));
		    }
#if 0
		    {
			long j;

			response_length = strlen(response);

			for ( j = 0; j < response_length; j++ ) {
				MX_DEBUG(-2,("%s: response[%ld] = %#x '%c'",
				fname, j, response[j], response[j]));
			}
		    }
#endif
		}

	    } /* End of ( response != NULL ) block */

	} /* End of while() command attempt loop. */

	MX_DEBUG(2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_compumotor_get_controller_index(
				MX_COMPUMOTOR_INTERFACE *compumotor_interface,
				long controller_number,
				long *controller_index )
{
	static const char fname[] = "mxi_compumotor_get_controller_index()";

	long i;
	long *controller_number_array;

	/* Find the array index for this controller number. */

	controller_number_array = compumotor_interface->controller_number;

	for ( i = 0; i < compumotor_interface->num_controllers; i++ ) {

		if ( controller_number == controller_number_array[i] ) {
			*controller_index = i;

			MX_DEBUG( 2,
			("%s: controller_number = %ld controller_index = %ld",
			fname, controller_number, *controller_index ));

			break;	/* Exit the for() loop. */
		}
	}

	if ( i >= compumotor_interface->num_controllers ) {
		return mx_error( MXE_NOT_FOUND, fname,
"Compumotor interface record '%s' does not have a controller number %ld.",
			compumotor_interface->record->name, controller_number );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_compumotor_multiaxis_move( MX_COMPUMOTOR_INTERFACE *compumotor_interface,
				long controller_index,
				long num_motors,
				MX_RECORD **motor_record_array,
				double *motor_position_array,
				mx_bool_type simultaneous_start )
{
	static const char fname[] = "mxi_compumotor_multiaxis_move()";

	MX_MOTOR *motor;
	MX_COMPUMOTOR *compumotor;
	double velocity;

	double raw_acceleration_parameter_array[
					MX_MOTOR_NUM_ACCELERATION_PARAMS];

	double destination[MX_MAX_COMPUMOTOR_AXES];
	mx_bool_type will_be_moved[MX_MAX_COMPUMOTOR_AXES];
	long axis, controller_number;
	long i, num_axes;
	char command[80];
	char *ptr;
	size_t length, buffer_left;
	mx_status_type mx_status;

	controller_number
		= compumotor_interface->controller_number[ controller_index ];

	/* Are we using a simultaneous start or are we using the 
	 * linear interpolation functionality?
	 */

	if ( simultaneous_start == FALSE ) {

		/* We only set velocities and accelerations with PV and PA
		 * if we are doing a linear interpolation move.
		 */

		/* Get the acceleration and velocity for the first motor. */

		mx_status = mx_motor_get_raw_acceleration_parameters(
				motor_record_array[0],
				raw_acceleration_parameter_array );
	
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	
		mx_status = mx_motor_get_speed( motor_record_array[0],
							&velocity );
	
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	
		motor = (MX_MOTOR *) motor_record_array[0]->record_class_struct;
	
		velocity = mx_divide_safely( velocity , motor->scale );
	
		/* Set the acceleration for the linear interpolation move. */
	
		snprintf( command, sizeof(command),
			"%ld_!PA%g", controller_number,
				raw_acceleration_parameter_array[0] );
	
		mx_status = mxi_compumotor_command( compumotor_interface,
					command, NULL, 0,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	
		/* Set the velocity for the linear interpolation move. */
	
		snprintf( command, sizeof(command),
			"%ld_!PV%g", controller_number, velocity );
	
		mx_status = mxi_compumotor_command( compumotor_interface,
					command, NULL, 0,
					MXI_COMPUMOTOR_INTERFACE_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Construct the command to set the destination. */

	for ( i = 0; i < MX_MAX_COMPUMOTOR_AXES; i++ ) {
		destination[i]   = 0.0;
		will_be_moved[i] = FALSE;
	}

	for ( i = 0; i < num_motors; i++ ) {
		compumotor = (MX_COMPUMOTOR *)
				motor_record_array[i]->record_type_struct;

		axis = compumotor->axis_number;

		MX_DEBUG( 2,("%s: motor_record_array[%ld] = %p",
				fname, i, motor_record_array[i]));

		MX_DEBUG( 2,("%s: motor_record_array[%ld] = '%s'",
				fname, i, motor_record_array[i]->name));

		motor = (MX_MOTOR *)
				motor_record_array[i]->record_class_struct;

		MX_DEBUG( 2,("%s: motor_position_array[%ld] = %g, scale = %g",
			fname, i, motor_position_array[i], motor->scale ));

		destination[axis-1] = mx_divide_safely( motor_position_array[i],
							motor->scale );

		MX_DEBUG( 2, ("%s: axis = %ld, destination = %g",
			fname, axis, destination[axis-1] ));

		will_be_moved[axis-1] = TRUE;
	}

	num_axes = compumotor_interface->num_axes[ controller_number - 1 ];

	snprintf( command, sizeof(command), "%ld_!D", controller_number );

	for ( i = 0; i < num_axes; i++ ) {

		MX_DEBUG( 2,("%s: command = '%s'", fname, command));

		MX_DEBUG( 2,
		("%s: i = %ld, will_be_moved = %d, destination[%ld] = %.5f",
			fname, i, (int) will_be_moved[i], i, destination[i]));

		length = strlen( command );

		ptr = command + length;

		buffer_left = sizeof( command ) - length;

		if ( i != 0 ) {
			*ptr = ',';

			ptr++;

			*ptr = '\0';
		}
		if ( will_be_moved[i] ) {
			/*
			 * Add to the move command.
			 *
			 * Note that the Compumotor manual specifies a maximum
			 * of 5 digits after the decimal point.
			 */

			snprintf( ptr, buffer_left, "%.5f", destination[i] );
		}
	}

	MX_DEBUG( 2,("%s: command = '%s'", fname, command));

	/* Set the destination. */

	mx_status = mxi_compumotor_command( compumotor_interface, command,
			NULL, 0, MXI_COMPUMOTOR_INTERFACE_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct the command to start the move. */

	if ( simultaneous_start ) {
		snprintf( command, sizeof(command),
				"%ld_!GO", controller_number );
	} else {
		snprintf( command, sizeof(command),
				"%ld_!GOL", controller_number );
	}

	for ( i = 0; i < num_axes; i++ ) {

		length = strlen( command );

		ptr = command + length;

		buffer_left = sizeof( command ) - length;

		if ( will_be_moved[i] ) {
			strlcat( ptr, "1", buffer_left );
		} else {
			strlcat( ptr, "0", buffer_left );
		}
	}

	/* Start the move. */

	MX_DEBUG( 2,("%s: move command = '%s'", fname, command));

	mx_status = mxi_compumotor_command( compumotor_interface, command,
			NULL, 0, MXI_COMPUMOTOR_INTERFACE_DEBUG );

	return mx_status;
}

