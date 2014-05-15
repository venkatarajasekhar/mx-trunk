/*
 * Name:    i_newport_xps.c
 *
 * Purpose: MX driver for Newport XPS motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_NEWPORT_XPS_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "i_newport_xps.h"

/* Vendor include file. */

#include "XPS_C8_drivers.h"

MX_RECORD_FUNCTION_LIST mxi_newport_xps_record_function_list = {
	NULL,
	mxi_newport_xps_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_newport_xps_open
};

MX_RECORD_FIELD_DEFAULTS mxi_newport_xps_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_NEWPORT_XPS_STANDARD_FIELDS
};

long mxi_newport_xps_num_record_fields
		= sizeof( mxi_newport_xps_record_field_defaults )
			/ sizeof( mxi_newport_xps_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_newport_xps_rfield_def_ptr
			= &mxi_newport_xps_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_newport_xps_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_newport_xps_create_record_structures()";

	MX_NEWPORT_XPS *newport_xps = NULL;

	/* Allocate memory for the necessary structures. */

	newport_xps = (MX_NEWPORT_XPS *) malloc( sizeof(MX_NEWPORT_XPS) );

	if ( newport_xps == (MX_NEWPORT_XPS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NEWPORT_XPS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = newport_xps;

	record->record_function_list = &mxi_newport_xps_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	newport_xps->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_newport_xps_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_newport_xps_open()";

	MX_NEWPORT_XPS *newport_xps = NULL;
	int xps_status;
	int controller_status_code;
	char controller_status_string[200];
	char firmware_version[200];
	char hardware_date_and_time[200];
	double elapsed_seconds_since_power_on;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	newport_xps = (MX_NEWPORT_XPS *) record->record_type_struct;

	if ( newport_xps == (MX_NEWPORT_XPS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NEWPORT_XPS pointer for record '%s' is NULL.",
			record->name );
	}

	MX_DEBUG(-2,("%s: '%s'", fname, GetLibraryVersion() ));

	/* Connect to the Newport XPS controller. */

	newport_xps->socket_id = TCP_ConnectToServer(
					newport_xps->hostname,
					newport_xps->port_number,
					5.0 );

	MX_DEBUG(-2,("%s: newport_xps->socket_id = %d",
			fname, newport_xps->socket_id));

	/* Login to the Newport XPS controller. */

	xps_status = Login( newport_xps->socket_id,
				newport_xps->username,
				newport_xps->password );

	MX_DEBUG(-2,("%s: Login() status = %d", fname, xps_status));

	if ( xps_status == 0 ) {
		MX_DEBUG(-2,("%s: Login() successfully completed.", fname));
	} else {
		MX_DEBUG(-2,("%s: Login() failed.", fname));
	}

	/*---*/

	xps_status = FirmwareVersionGet( newport_xps->socket_id,
					firmware_version );

	MX_DEBUG(-2,("%s: firmware version = '%s'", fname, firmware_version));

	/*---*/

	xps_status = ElapsedTimeGet( newport_xps->socket_id,
					&elapsed_seconds_since_power_on );

	MX_DEBUG(-2,("%s: %f seconds since power on.",
		fname, elapsed_seconds_since_power_on));

	/*---*/

	xps_status = HardwareDateAndTimeGet( newport_xps->socket_id,
					hardware_date_and_time );

	MX_DEBUG(-2,("%s: hardware date and time = '%s'",
		fname, hardware_date_and_time ));

	/*---*/

	xps_status = ControllerStatusGet( newport_xps->socket_id,
					&controller_status_code );

	xps_status = ControllerStatusStringGet( newport_xps->socket_id,
						controller_status_code,
						controller_status_string );

	MX_DEBUG(-2,("%s: controller status = %d, '%s'",
		fname, controller_status_code, controller_status_string));

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

