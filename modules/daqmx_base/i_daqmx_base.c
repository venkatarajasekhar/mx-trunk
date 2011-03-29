/*
 * Name:    i_daqmx_base.c
 *
 * Purpose: MX interface driver for the National Instruments DAQmx Base system.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_DAQMX_BASE_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "i_daqmx_base.h"

MX_RECORD_FUNCTION_LIST mxi_daqmx_base_record_function_list = {
	NULL,
	mxi_daqmx_base_create_record_structures,
	mxi_daqmx_base_finish_record_initialization,
	NULL,
	NULL,
	mxi_daqmx_base_open,
	mxi_daqmx_base_close
};

MX_RECORD_FIELD_DEFAULTS mxi_daqmx_base_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_DAQMX_BASE_STANDARD_FIELDS
};

long mxi_daqmx_base_num_record_fields
		= sizeof( mxi_daqmx_base_record_field_defaults )
			/ sizeof( mxi_daqmx_base_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_daqmx_base_rfield_def_ptr
			= &mxi_daqmx_base_record_field_defaults[0];

static mx_status_type
mxi_daqmx_base_get_pointers( MX_RECORD *record,
				MX_DAQMX_BASE **daqmx_base,
				const char *calling_fname )
{
	static const char fname[] = "mxi_daqmx_base_get_pointers()";

	MX_DAQMX_BASE *daqmx_base_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	daqmx_base_ptr = (MX_DAQMX_BASE *) record->record_type_struct;

	if ( daqmx_base_ptr == (MX_DAQMX_BASE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DAQMX_BASE pointer for record '%s' is NULL.",
			record->name );
	}

	if ( daqmx_base != (MX_DAQMX_BASE **) NULL ) {
		*daqmx_base = daqmx_base_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxi_daqmx_base_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_daqmx_base_create_record_structures()";

	MX_DAQMX_BASE *daqmx_base;

	/* Allocate memory for the necessary structures. */

	daqmx_base = (MX_DAQMX_BASE *) malloc( sizeof(MX_DAQMX_BASE) );

	if ( daqmx_base == (MX_DAQMX_BASE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_DAQMX_BASE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = daqmx_base;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	daqmx_base->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_daqmx_base_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_daqmx_base_finish_record_initialization()";

	MX_DAQMX_BASE *daqmx_base;
	mx_status_type mx_status;

	mx_status = mxi_daqmx_base_get_pointers( record,
						&daqmx_base, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_daqmx_base_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_daqmx_base_open()";

	MX_DAQMX_BASE *daqmx_base;
	mx_status_type mx_status;

	mx_status = mxi_daqmx_base_get_pointers( record,
						&daqmx_base, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_warning( "The National Instruments DAQmx Base system "
	"takes a _long_ time to initialize itself, so please be patient." );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_daqmx_base_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_daqmx_base_close()";

	MX_DAQMX_BASE *daqmx_base;
	mx_status_type mx_status;

	mx_status = mxi_daqmx_base_get_pointers( record,
						&daqmx_base, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------- Exported driver-specific functions ---------------*/

MX_EXPORT mx_status_type
mxi_daqmx_base_shutdown_task( MX_RECORD *record, TaskHandle task_handle )
{
	static const char fname[] = "mxi_daqmx_base_shutdown_task()";

	int32 daqmx_status;

	/* Stop the task. */

	daqmx_status = DAQmxBaseStopTask( task_handle );

#if MXI_DAQMX_BASE_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseStopTask( %#lx ) = %d",
		fname, (unsigned long) task_handle, (int) daqmx_status));
#endif

	if ( daqmx_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to stop task %#lx for DAQmx Base device '%s' "
		"failed.  DAQmx error code = %d",
			(unsigned long) task_handle,
			record->name,
			(int) daqmx_status );
	}

#if MXI_DAQMX_BASE_DEBUG
	MX_DEBUG(-2,("%s: DAQmxBaseClearTask( %#lx ) = %d",
		fname, (unsigned long) task_handle, (int) daqmx_status));
#endif

	/* Release the resources used by this task. */

	daqmx_status = DAQmxBaseClearTask( task_handle );

	if ( daqmx_status != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to clear task %#lx for DAQmx Base device '%s' "
		"failed.  DAQmx error code = %d",
			(unsigned long) task_handle,
			record->name,
			(int) daqmx_status );
	}

	return MX_SUCCESSFUL_RESULT;
}
