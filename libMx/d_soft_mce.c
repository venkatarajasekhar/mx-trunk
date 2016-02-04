/*
 * Name:    d_soft_mce.c
 *
 * Purpose: MX MCE driver to support MX network multichannel encoders.
 *
 * Author:  William Lavender
 *
 * FIXME:   We need to implement a poll callback for this function
 *          in order to fill in the values of mce->value_array.
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SOFT_MCE_DEBUG			TRUE

#define MXD_SOFT_MCE_DEBUG_READ_MEASUREMENT	TRUE

#define MXD_SOFT_MCE_DEBUG_MONITOR_THREAD	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_thread.h"
#include "mx_mutex.h"
#include "mx_condition_variable.h"
#include "mx_atomic.h"
#include "mx_mce.h"
#include "mx_motor.h"
#include "d_soft_mce.h"

/* Initialize the MCE driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_soft_mce_record_function_list = {
	mxd_soft_mce_initialize_driver,
	mxd_soft_mce_create_record_structures,
	mxd_soft_mce_finish_record_initialization,
	mxd_soft_mce_delete_record,
	NULL,
	mxd_soft_mce_open
};

MX_MCE_FUNCTION_LIST mxd_soft_mce_mce_function_list = {
	NULL,
	NULL,
	mxd_soft_mce_read,
	mxd_soft_mce_get_current_num_values,
	mxd_soft_mce_get_last_measurement_number,
	mxd_soft_mce_get_status,
	mxd_soft_mce_start,
	mxd_soft_mce_stop,
	mxd_soft_mce_clear,
	mxd_soft_mce_read_measurement,
	NULL,
	NULL,
	mxd_soft_mce_get_parameter,
	mxd_soft_mce_set_parameter
};

/* soft mce data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_soft_mce_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCE_STANDARD_FIELDS,
	MXD_SOFT_MCE_STANDARD_FIELDS
};

long mxd_soft_mce_num_record_fields
		= sizeof( mxd_soft_mce_record_field_defaults )
		  / sizeof( mxd_soft_mce_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_mce_rfield_def_ptr
			= &mxd_soft_mce_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_soft_mce_get_pointers( MX_MCE *mce,
			MX_SOFT_MCE **soft_mce,
			MX_MOTOR **motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_soft_mce_get_pointers()";

	MX_SOFT_MCE *soft_mce_ptr = NULL;
	MX_RECORD *motor_record = NULL;

	if ( mce == (MX_MCE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	soft_mce_ptr = (MX_SOFT_MCE *) mce->record->record_type_struct;

	if ( soft_mce_ptr == (MX_SOFT_MCE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SOFT_MCE pointer for record '%s' is NULL.",
			mce->record->name );
	}

	if ( soft_mce != (MX_SOFT_MCE **) NULL ) {
		*soft_mce = soft_mce_ptr;
	}

	if ( motor != (MX_MOTOR **) NULL ) {
		motor_record = soft_mce_ptr->motor_record;

		if ( motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The motor record pointer for record '%s' is NULL.",
				mce->record->name );
		}

		*motor = (MX_MOTOR *) motor_record->record_class_struct;

		if ( (*motor) == (MX_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MOTOR pointer for motor record '%s' "
			"used by soft MCE '%s' is NULL.",
				motor_record->name,
				mce->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------*/

static mx_status_type
mxd_soft_mce_monitor_thread_fn( MX_THREAD *thread, void *record_ptr )
{
	static const char fname[] = "mxd_soft_mce_monitor_thread_fn()";

	MX_RECORD *record = NULL;
	MX_MCE *mce = NULL;
	MX_SOFT_MCE *soft_mce = NULL;
	unsigned long mx_status_code;
	mx_status_type mx_status;

	/* Initialize the variables to be used by this thread. */

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	if ( record_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	record = (MX_RECORD *) record_ptr;

	mce = (MX_MCE *) record->record_class_struct;

	if ( mce == (MX_MCE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCE pointer for MCE record '%s' is NULL.",
			record->name );
	}

	soft_mce = (MX_SOFT_MCE *) record->record_type_struct;

	if ( soft_mce == (MX_SOFT_MCE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SOFT_MCE pointer for MCE record '%s' is NULL.",
			record->name );
	}

	/* Lock the mutex in preparation for entering the thread's
	 * event handling loop.
	 */

	mx_status_code = mx_mutex_lock( soft_mce->monitor_thread_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"the attempt to lock the monitor_thread_mutex for "
		"soft MCE record '%s' failed.", record->name );
	}

	/* Now that we have locked the mutex, we can not set the
	 * monitor status to idle.
	 */

	mx_atomic_write32( &(soft_mce->monitor_status), MXS_SOFT_MCE_IDLE );

#if MXD_SOFT_MCE_DEBUG_MONITOR_THREAD
	MX_DEBUG(-2,("%s: MCE '%s' entering event loop.", fname, record->name));
#endif

	while (TRUE) {

		/* Wait on the condition variable. */

		if ( soft_mce->monitor_status == MXS_SOFT_MCE_IDLE ) {
			mx_status = mx_condition_variable_wait(
					soft_mce->monitor_thread_cv,
					soft_mce->monitor_thread_mutex );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

#if MXD_SOFT_MCE_DEBUG_MONITOR_THREAD
		MX_DEBUG(-2,("%s: '%s' command = %ld, status = %ld",
			fname, record->name,
			(long) soft_mce->monitor_command,
			(long) soft_mce->monitor_status));
#endif

		switch( soft_mce->monitor_command ) {
		case MXS_SOFT_MCE_START:
			mx_atomic_write32( &(soft_mce->monitor_status),
						MXS_SOFT_MCE_ACQUIRING );
			break;
		case MXS_SOFT_MCE_STOP:
			mx_atomic_write32( &(soft_mce->monitor_status),
						MXS_SOFT_MCE_IDLE );
			break;
		case MXS_SOFT_MCE_CLEAR:
			mx_atomic_write32( &(soft_mce->monitor_status),
						MXS_SOFT_MCE_IDLE );
			break;
		default:
			(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Unsupported command %ld requested "
				"for soft MCE '%s' monitor thread.",
					(long) soft_mce->monitor_command,
					record->name );
			break;
		}
	}
}

static mx_status_type
mxd_soft_mce_send_command_to_monitor_thread( MX_SOFT_MCE *soft_mce,
					int32_t monitor_command )
{
	static const char fname[] =
		"mxd_soft_mce_send_command_to_monitor_thread()";

	unsigned long mx_status_code;
	mx_status_type mx_status;

	if ( soft_mce == (MX_SOFT_MCE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOFT_MCE pointer passed was NULL." );
	}

	/* Prepare to tell the "monitor" thread to start a command. */

	mx_status_code = mx_mutex_lock( soft_mce->monitor_thread_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock the monitor thread mutex for "
		"soft MCE '%s' failed.",
			soft_mce->record->name );
	}

	soft_mce->monitor_command = monitor_command;

	mx_status_code = mx_mutex_unlock( soft_mce->monitor_thread_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to unlock the monitor thread mutex for "
		"soft MCE '%s' failed.",
			soft_mce->record->name );
	}

	/* Tell the "monitor" thread to start the command. */

	mx_status = mx_condition_variable_signal( soft_mce->monitor_thread_cv );

	return mx_status;
}

/*------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_soft_mce_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_values_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mce_initialize_driver( driver,
					&maximum_num_values_varargs_cookie );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_mce_create_record_structures()";

	MX_MCE *mce;
	MX_SOFT_MCE *soft_mce;

	/* Allocate memory for the necessary structures. */

	mce = (MX_MCE *) malloc( sizeof(MX_MCE) );

	if ( mce == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCE structure." );
	}

	soft_mce = (MX_SOFT_MCE *) malloc( sizeof(MX_SOFT_MCE) );

	if ( soft_mce == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SOFT_MCE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mce;
	record->record_type_struct = soft_mce;
	record->class_specific_function_list = &mxd_soft_mce_mce_function_list;

	mce->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mce_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_soft_mce_finish_record_initialization()";

	MX_MCE *mce = NULL;
	MX_SOFT_MCE *soft_mce = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	mce = (MX_MCE *) record->record_class_struct;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mce->window_is_available = FALSE;
	mce->use_window = FALSE;

	memset( mce->window,
		0, MXU_MTR_NUM_WINDOW_PARAMETERS * sizeof(double) );

	/*----*/

	mce->encoder_type = MXT_MCE_ABSOLUTE_ENCODER;

	mce->current_num_values = mce->maximum_num_values;

	strlcpy( mce->selected_motor_name,
			soft_mce->motor_record->name,
			MXU_RECORD_NAME_LENGTH );

	mce->num_motors = 1;

	mce->motor_record_array = &(soft_mce->motor_record);

	mx_status = mx_mce_fixup_motor_record_array_field( mce );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mce_delete_record( MX_RECORD *record )
{
	MX_MCE *mce;

	if ( record == (MX_RECORD *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mce = (MX_MCE *) record->record_class_struct;

	if ( mce != (MX_MCE *) NULL ) {
		if ( mce->motor_record_array != (MX_RECORD **) NULL ) {
			mx_free( mce->motor_record_array );
		}
		mx_free( record->record_class_struct );
	}

	if ( record->record_type_struct != NULL ) {
		mx_free( record->record_type_struct );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mce_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_mce_open()";

	MX_MCE *mce = NULL;
	MX_SOFT_MCE *soft_mce = NULL;
	int32_t test_value;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	mce = (MX_MCE *) record->record_class_struct;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*-----------------------------------------------------------------*/

	soft_mce->monitor_command = MXS_SOFT_MCE_IDLE;
	soft_mce->monitor_status  = MXS_SOFT_MCE_NOT_INITIALIZED;

	/* Create some synchronization objects to be used by the
	 * monitor thread.
	 */

	mx_status = mx_mutex_create( &(soft_mce->monitor_thread_mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status =
		mx_condition_variable_create( &(soft_mce->monitor_thread_cv) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create the monitor thread to handle updates to 'mce->value_array'. */

	mx_status = mx_thread_create( &(soft_mce->monitor_thread),
					mxd_soft_mce_monitor_thread_fn,
					record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the monitor thread to get itself initialized. */

	do {
		test_value = mx_atomic_read32( &(soft_mce->monitor_status) );
	} while ( test_value == MXS_SOFT_MCE_NOT_INITIALIZED );

	/*-----------------------------------------------------------------*/

	/* Clear the contents of the MCE. */

	mx_status = mxd_soft_mce_clear( mce );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_read( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_read()";

	MX_SOFT_MCE *soft_mce;
	mx_status_type mx_status;

	soft_mce = NULL;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	/* The contents of value_array have already been set by this driver's
	 * poll callback, so we do not need to do anything further here.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mce_get_current_num_values( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_get_current_num_values()";

	MX_SOFT_MCE *soft_mce = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	/* We use the already existing value of mce->current_num_values. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mce_get_last_measurement_number( MX_MCE *mce )
{
	static const char fname[] =
		"mxd_soft_mce_get_last_measurement_number()";

	MX_SOFT_MCE *soft_mce = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	/* We use the already existing value of mce->current_num_values. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mce_get_status( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_get_status()";

	MX_SOFT_MCE *soft_mce = NULL;
	int32_t monitor_thread_status;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	monitor_thread_status = mx_atomic_read32( &(soft_mce->monitor_status) );

	if ( monitor_thread_status == MXS_SOFT_MCE_IDLE ) {
		mce->status = 0;
	} else {
		mce->status = 0x1;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mce_start( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_start()";

	MX_SOFT_MCE *soft_mce = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	mx_status =  mxd_soft_mce_send_command_to_monitor_thread(
					soft_mce, MXS_SOFT_MCE_START );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_stop( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_stop()";

	MX_SOFT_MCE *soft_mce = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	mx_status =  mxd_soft_mce_send_command_to_monitor_thread(
					soft_mce, MXS_SOFT_MCE_STOP );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_clear( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_clear()";

	MX_SOFT_MCE *soft_mce = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));
#endif

	mx_status =  mxd_soft_mce_send_command_to_monitor_thread(
					soft_mce, MXS_SOFT_MCE_CLEAR );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_read_measurement( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_read_measurement()";

	MX_SOFT_MCE *soft_mce = NULL;
	MX_MOTOR *motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, &motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mce->value = mce->value_array[ mce->measurement_index ];

#if MXD_SOFT_MCE_DEBUG_READ_MEASUREMENT
	MX_DEBUG(-2,("%s: MCE '%s', index = %ld, value = %f",
		fname, mce->record->name, mce->measurement_index, mce->value));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_get_parameter( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_get_parameter()";

	MX_SOFT_MCE *soft_mce;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, mce->record->name,
		mx_get_field_label_string( mce->record, mce->parameter_type ),
#endif
		mce->parameter_type ));

	switch( mce->parameter_type ) {
	case MXLV_MCE_MEASUREMENT_WINDOW_OFFSET:
		mce->measurement_window_offset = 0;
		break;
	default:
		return mx_mce_default_get_parameter_handler( mce );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mce_set_parameter( MX_MCE *mce )
{
	static const char fname[] = "mxd_soft_mce_set_parameter()";

	MX_SOFT_MCE *soft_mce;
	mx_status_type mx_status;

	mx_status = mxd_soft_mce_get_pointers( mce, &soft_mce, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCE_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, mce->record->name,
		mx_get_field_label_string( mce->record, mce->parameter_type ),
		mce->parameter_type ));
#endif

	switch( mce->parameter_type ) {
	default:
		return mx_mce_default_set_parameter_handler( mce );
		break;
	}

	return mx_status;
}

