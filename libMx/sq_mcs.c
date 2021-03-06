/*
 * Name:    sq_mcs.c
 *
 * Purpose: Driver for quick scans that use an MX MCS record
 *          to collect the data.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2006, 2008, 2010-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_TIMING		FALSE

#define DEBUG_SPEED		FALSE

#define DEBUG_SCAN_PROGRESS	FALSE

#define DEBUG_WAIT_FOR_MCS	FALSE

#define DEBUG_PAUSE_REQUEST	FALSE

#define DEBUG_READ_MEASUREMENT	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_variable.h"
#include "mx_hrt_debug.h"
#include "mx_mcs.h"
#include "mx_timer.h"
#include "mx_pulse_generator.h"
#include "mx_scan.h"
#include "mx_scan_quick.h"
#include "sq_mcs.h"
#include "m_time.h"
#include "m_pulse_period.h"
#include "d_network_motor.h"
#include "d_mcs_mce.h"
#include "d_mcs_scaler.h"
#include "d_mcs_timer.h"

MX_RECORD_FUNCTION_LIST mxs_mcs_quick_scan_record_function_list = {
	mx_quick_scan_initialize_driver,
	mxs_mcs_quick_scan_create_record_structures,
	mxs_mcs_quick_scan_finish_record_initialization,
	mxs_mcs_quick_scan_delete_record,
	mx_quick_scan_print_scan_structure
};

MX_SCAN_FUNCTION_LIST mxs_mcs_quick_scan_scan_function_list = {
	mxs_mcs_quick_scan_prepare_for_scan_start,
	mxs_mcs_quick_scan_execute_scan_body,
	mxs_mcs_quick_scan_cleanup_after_scan_end,
	NULL,
	mxs_mcs_quick_scan_get_parameter
};

MX_RECORD_FIELD_DEFAULTS mxs_mcs_quick_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_QUICK_SCAN_STANDARD_FIELDS
};

long mxs_mcs_quick_scan_num_record_fields
			= sizeof( mxs_mcs_quick_scan_defaults )
			/ sizeof( mxs_mcs_quick_scan_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_mcs_quick_scan_def_ptr
			= &mxs_mcs_quick_scan_defaults[0];

#define FREE_MOTOR_POSITION_ARRAYS \
		mxs_mcs_quick_scan_free_arrays(scan,quick_scan,mcs_quick_scan)

static void
mxs_mcs_quick_scan_free_arrays( MX_SCAN *scan,
				MX_QUICK_SCAN *quick_scan,
				MX_MCS_QUICK_SCAN *mcs_quick_scan )
{
	if ( scan == NULL )
		return;

	if ( mcs_quick_scan == NULL )
		return;

	/* Please note that mx_free() and mx_free_array() both check 
	 * for NULL pointer arguments and will merely return if you feed
	 * a NULL pointer to them.
	 */

	/* WARNING: Some of the array pointers found in MX_MCS_QUICK_SCAN
	 * are allocated in mxs_mcs_quick_scan_prepare_for_scan_start()
	 * and should be freed by this function.  The rest of these pointers
	 * are allocated in mxs_mcs_quick_scan_finish_record_initialization()
	 * and should _NOT_ be freed here.
	 * 
	 * As of 2016-02-09, the array pointers are as follows:
	 *
	 * Allocated by mxs_mcs_quick_scan_prepare_for_scan_start()
	 * and should be freed here:
	 *
	 *   mcs_quick_scan->motor_position_array
	 *   mcs_quick_scan->real_motor_record_array
	 *   mcs_quick_scan->mce_record_array
	 *
	 * Allocated by mxs_mcs_quick_scan_finish_record_initialization()
	 * and should __NOT__ be freed here:
	 *
	 *   mcs_quick_scan->mcs_record_array
	 *   mcs_quick_scan->real_start_position
	 *   mcs_quick_scan->real_end_position
	 *   mcs_quick_scan->backlash_position
	 *   mcs_quick_scan->use_window
	 *   mcs_quick_scan->window
	 *   mcs_quick_scan->mcs_measurement_offset
	 *
	 * In earlier versions of the code, if you freed array pointers here
	 * that should not be freed here, the symptom was that a given instance
	 * of an MX client program such as mxmotor could only run a given quick
	 * scan _once_.  To recover, you had to exit the program and reenter it.
	 *
	 * This problem was introduced in SVN revision 3138 and fixed in
	 * SVN revision 3357.  (W. Lavender, 2016-02-09)
	 */

	if ( mcs_quick_scan != (MX_MCS_QUICK_SCAN *) NULL ) {
		(void) mx_free_array( mcs_quick_scan->motor_position_array );

		mx_free( mcs_quick_scan->real_motor_record_array );
		mx_free( mcs_quick_scan->mce_record_array );

		mcs_quick_scan->motor_position_array = NULL;
		mcs_quick_scan->real_motor_record_array = NULL;
		mcs_quick_scan->mce_record_array = NULL;
	}

	if ( scan != (MX_SCAN *) NULL ) {
		(void) mx_free_array( scan->datafile.x_position_array );
		(void) mx_free_array( scan->plot.x_position_array );

		if ( scan->missing_record_array != NULL ) {
			int i;

			for ( i = 0; i < scan->num_missing_records; i++ ) {
				mx_delete_record(
					scan->missing_record_array[i] );
			}

			mx_free( scan->missing_record_array );
		}
	}
}

MX_EXPORT mx_status_type
mxs_mcs_quick_scan_get_pointers( MX_SCAN *scan,
				MX_QUICK_SCAN **quick_scan,
				MX_MCS_QUICK_SCAN **mcs_quick_scan,
				const char *calling_fname )
{
	static const char fname[] = "mxs_mcs_quick_scan_get_pointers()";

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCAN pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( quick_scan == (MX_QUICK_SCAN **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_QUICK_SCAN pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( mcs_quick_scan == (MX_MCS_QUICK_SCAN **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MCS_QUICK_SCAN pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*quick_scan = (MX_QUICK_SCAN *) (scan->record->record_class_struct);

	if ( *quick_scan == (MX_QUICK_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_QUICK_SCAN pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	*mcs_quick_scan = (MX_MCS_QUICK_SCAN *)
					scan->record->record_type_struct;

	if ( *mcs_quick_scan == (MX_MCS_QUICK_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCS_QUICK_SCAN pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_mcs_quick_scan_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxs_mcs_quick_scan_create_record_structures()";

	MX_SCAN *scan;
	MX_QUICK_SCAN *quick_scan;
	MX_MCS_QUICK_SCAN *mcs_quick_scan;

	/* Allocate memory for the necessary structures. */

	scan = (MX_SCAN *) malloc( sizeof(MX_SCAN) );

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCAN structure." );
	}

	quick_scan = (MX_QUICK_SCAN *) malloc( sizeof(MX_QUICK_SCAN) );

	if ( quick_scan == (MX_QUICK_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_QUICK_SCAN structure." );
	}

	mcs_quick_scan = (MX_MCS_QUICK_SCAN *)
				malloc( sizeof(MX_MCS_QUICK_SCAN) );

	if ( mcs_quick_scan == (MX_MCS_QUICK_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCS_QUICK_SCAN structure.");
	}

	/* Now set up the necessary pointers. */

	record->record_superclass_struct = scan;
	record->record_class_struct = quick_scan;
	record->record_type_struct = mcs_quick_scan;

	scan->record = record;

	scan->measurement.scan = scan;
	scan->datafile.scan = scan;
	scan->plot.scan = scan;

	scan->datafile.x_motor_array = NULL;
	scan->plot.x_motor_array = NULL;

	quick_scan->scan = scan;

	record->superclass_specific_function_list
				= &mxs_mcs_quick_scan_scan_function_list;

	record->class_specific_function_list = NULL;

	scan->num_missing_records = 0;
	scan->missing_record_array = NULL;

	mcs_quick_scan->motor_position_array = NULL;
	mcs_quick_scan->real_motor_record_array = NULL;
	mcs_quick_scan->mce_record_array = NULL;
	mcs_quick_scan->mcs_record_array = NULL;
	mcs_quick_scan->real_start_position = NULL;
	mcs_quick_scan->real_end_position = NULL;
	mcs_quick_scan->backlash_position = NULL;
	mcs_quick_scan->use_window = NULL;
	mcs_quick_scan->window = NULL;

	mcs_quick_scan->extension_ptr = NULL;

	mcs_quick_scan->move_to_start_fn =
			mxs_mcs_quick_scan_default_move_to_start;

	mcs_quick_scan->compute_motor_positions_fn =
			mxs_mcs_quick_scan_default_compute_motor_positions;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_mcs_quick_scan_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxs_mcs_quick_scan_finish_record_initialization()";

	MX_SCAN *scan;
	MX_QUICK_SCAN *quick_scan;
	MX_MCS_QUICK_SCAN *mcs_quick_scan;
	MX_RECORD *input_device_record;
	MX_MCS_SCALER *mcs_scaler;
	MX_RECORD *mcs_record;
	size_t new_array_size;
	long i, j, num_mcs;
	int mcs_already_found;
	void *ptr;
	mx_status_type mx_status;

	scan = (MX_SCAN *) record->record_superclass_struct;

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcs_quick_scan->num_mcs = 0;
	mcs_quick_scan->mcs_record_array = NULL;
	mcs_quick_scan->mcs_array_size = 0;

	if ( scan->num_input_devices <= 0 )
		return MX_SUCCESSFUL_RESULT;

	if ( scan->num_motors < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal number of motors %ld for scan '%s'.  "
		"The number of motors must not be negative.",
			scan->num_motors, record->name );
	} else
	if ( scan->num_motors == 0 ) {
		mcs_quick_scan->real_start_position = NULL;
		mcs_quick_scan->real_end_position = NULL;
		mcs_quick_scan->backlash_position = NULL;
		mcs_quick_scan->use_window = NULL;
		mcs_quick_scan->window = NULL;
	} else {
		/* Allocate some start and end position arrays. */

		long dimension[2];
		size_t element_size[2];

		mcs_quick_scan->real_start_position = (double *)
			calloc( scan->num_motors, sizeof(double) );

		if ( mcs_quick_scan->real_start_position == (double *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %ld motor "
			"array of real start positions for scan '%s'.",
				scan->num_motors, record->name );
		}

		mcs_quick_scan->real_end_position = (double *)
			calloc( scan->num_motors, sizeof(double) );

		if ( mcs_quick_scan->real_end_position == (double *) NULL ) {
			mx_free( mcs_quick_scan->real_start_position );

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %ld motor "
			"array of real end positions for scan '%s'.",
				scan->num_motors, record->name );
		}

		mcs_quick_scan->backlash_position = (double *)
			calloc( scan->num_motors, sizeof(double) );

		if ( mcs_quick_scan->backlash_position == (double *) NULL ) {
			mx_free( mcs_quick_scan->real_start_position );
			mx_free( mcs_quick_scan->real_end_position );

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %ld motor "
			"array of backlash positions for scan '%s'.",
				scan->num_motors, record->name );
		}

		mcs_quick_scan->use_window = (mx_bool_type *)
			calloc( scan->num_motors, sizeof(mx_bool_type) );

		if ( mcs_quick_scan->use_window == (mx_bool_type *) NULL ) {
			mx_free( mcs_quick_scan->real_start_position );
			mx_free( mcs_quick_scan->real_end_position );
			mx_free( mcs_quick_scan->backlash_position );

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %ld motor "
			"array of 'use window' flags for scan '%s'.",
				scan->num_motors, record->name );
		}

		dimension[0] = scan->num_motors;
		dimension[1] = 2;

		element_size[0] = sizeof(double);
		element_size[1] = sizeof(double *);

		mcs_quick_scan->window = (double **)
			mx_allocate_array( MXFT_DOUBLE,
				2, dimension, element_size );

		if ( mcs_quick_scan->window == (double **) NULL ) {
			mx_free( mcs_quick_scan->real_start_position );
			mx_free( mcs_quick_scan->real_end_position );
			mx_free( mcs_quick_scan->backlash_position );
			mx_free( mcs_quick_scan->use_window );

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to alllocate a (%ld, 2) "
			"motor array of 'window' positions for scan '%s'.",
				scan->num_motors, record->name );
		}

		mcs_quick_scan->mcs_measurement_offset = (long *)
			calloc( scan->num_motors, sizeof(long) );

		if ( mcs_quick_scan->mcs_measurement_offset == (long *) NULL ) {
			mx_free( mcs_quick_scan->real_start_position );
			mx_free( mcs_quick_scan->real_end_position );
			mx_free( mcs_quick_scan->backlash_position );
			mx_free( mcs_quick_scan->use_window );
			(void) mx_free_array( mcs_quick_scan->window );

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %ld motor "
			"array of 'mcs_measurement_offset's for scan '%s'.",
				scan->num_motors, record->name );
		}
	}

	/*---*/

	for ( i = 0; i < scan->num_motors; i++ ) {
		scan->motor_is_independent_variable[i] = TRUE;
	}

	/* Assign a nominal value for actual_num_measurements here.
	 * The real value will be calculated when the scan is run.
	 */

	quick_scan->actual_num_measurements
			= quick_scan->requested_num_measurements;

	/* Allocate an array to contain the MCS records used by this scan. */

	new_array_size = MXS_SQ_MCS_ARRAY_BLOCK_SIZE;

	mcs_quick_scan->mcs_record_array = (MX_RECORD **)
		malloc( new_array_size * sizeof(MX_RECORD *) );

	if ( mcs_quick_scan->mcs_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array "
		"of MX_RECORD pointers for scan '%s'.",
			(unsigned long) new_array_size,
			record->name );
	}

	mcs_quick_scan->mcs_array_size = new_array_size;
	mcs_quick_scan->num_mcs = 0;

	/* Fill in the MCS record array. */

	for ( i = 0; i < scan->num_input_devices; i++ ) {

		/* Get the next input device. */

		input_device_record = ( scan->input_device_array )[i];

		if ( input_device_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Input device record pointer %ld for scan '%s' is NULL.",
				i, record->name );
		}

		if ( mx_verify_driver_type( input_device_record,
			MXR_DEVICE, MXC_SCALER, MXT_SCL_MCS ) == FALSE )
		{
#if 0
			/* If the input device is not an MCS scaler record,
			 * then we are done with it and should go back to
			 * the top of the for() loop for the next device.
			 */

			continue;
#else
			return mx_error( MXE_TYPE_MISMATCH, fname,
	"Input device '%s' for MCS scan '%s' is not a MCS scaler record.",
				input_device_record->name, record->name );
#endif
		}

		mcs_scaler = (MX_MCS_SCALER *)
				input_device_record->record_type_struct;

		if ( mcs_scaler == (MX_MCS_SCALER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCS_SCALER pointer for input device '%s' is NULL.",
				input_device_record->name );
		}

		mcs_record = mcs_scaler->mcs_record;

		if ( mcs_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The mcs record pointer for MCS scaler '%s' is NULL.",
				input_device_record->name );
		}

		/* See if we have already added this MCS to the array. */

		mcs_already_found = FALSE;

		num_mcs = (long) mcs_quick_scan->num_mcs;

		for ( j = 0; j < num_mcs; j++ ) {

			if (mcs_record == mcs_quick_scan->mcs_record_array[j])
			{
				mcs_already_found = TRUE;

				break;	/* exit the for(j) loop. */
			}
		}

		if ( mcs_already_found ) {
			continue;  /* go back to the top of the for(i) loop. */
		}

		if ( mx_verify_driver_type( mcs_record,
		    MXR_DEVICE, MXC_MULTICHANNEL_SCALER, MXT_ANY ) == FALSE ) 
		{
			return mx_error( MXE_TYPE_MISMATCH, fname,
"MCS record '%s' for MCS scaler '%s' is not a multichannel scaler record.",
				mcs_record->name, input_device_record->name );
		}

		/* Add the MCS to the array. */

		mcs_quick_scan->mcs_record_array[ num_mcs ] = mcs_record;

		(mcs_quick_scan->num_mcs)++;

		/* If we have just used the last element of the array,
		 * enlarge the array.
		 */

		if ( mcs_quick_scan->num_mcs >= mcs_quick_scan->mcs_array_size )
		{
			new_array_size = mcs_quick_scan->mcs_array_size
						+ MXS_SQ_MCS_ARRAY_BLOCK_SIZE;

			ptr = realloc( mcs_quick_scan->mcs_record_array,
					new_array_size * sizeof(MX_RECORD *) );

			if ( ptr == NULL ) {
				return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to extend an "
			"MX_RECORD * array to %lu elements for scan '%s'.",
					(unsigned long) new_array_size,
					record->name );
			}

			mcs_quick_scan->mcs_record_array = ptr;
			mcs_quick_scan->mcs_array_size = new_array_size;
		}
	}

	return mx_scan_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxs_mcs_quick_scan_delete_record( MX_RECORD *record )
{
	MX_SCAN *scan;
	MX_QUICK_SCAN *quick_scan;
	MX_MCS_QUICK_SCAN *mcs_quick_scan;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	scan = (MX_SCAN *) record->record_superclass_struct;

	quick_scan = (MX_QUICK_SCAN *) record->record_class_struct;

	mcs_quick_scan = (MX_MCS_QUICK_SCAN *) record->record_type_struct;

	FREE_MOTOR_POSITION_ARRAYS;

	mx_free( record->record_type_struct );
	mx_free( record->record_class_struct );
	mx_free( record->record_superclass_struct );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxs_mcs_quick_scan_set_motor_speeds( MX_SCAN *scan,
					MX_QUICK_SCAN *quick_scan,
					MX_MCS_QUICK_SCAN *mcs_quick_scan )
{
	MX_RECORD *motor_record;
	long i, j;
	mx_status_type mx_status;

	for ( i = 0; i < scan->num_motors; i++ ) {

		/* Save the old motor speed and synchronous motion mode. */

		motor_record = (scan->motor_record_array)[i];

		/* Change the motor speeds. */

		mx_status = mx_motor_set_speed_between_positions(
					motor_record,
					(quick_scan->start_position)[i],
					(quick_scan->end_position)[i],
					mcs_quick_scan->scan_body_time );

		if ( mx_status.code != MXE_SUCCESS ) {

			/* If the speed change failed, restore the old speeds
			 * for the other motors.
			 */

			for ( j = 0; j < i; j++ ) {
				(void) mx_motor_restore_speed(
					(scan->motor_record_array)[i] );
			}

			return mx_status;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxs_mcs_quick_scan_compute_backlash_position(
			MX_RECORD *motor_record,
			double start_position,
			double end_position,
			double *backlash_position )
{
	static const char fname[] =
		"mxs_mcs_quick_scan_compute_backlash_position()";

	MX_RECORD *real_motor_record;
	MX_MOTOR *real_motor;
	double real_start_position;
	double real_end_position;
	double real_backlash_position;
	double backlash;

	mx_status_type mx_status;

	/* Compute the backlash position using the real motor record. */

	mx_status = mx_motor_get_real_motor_record( motor_record,
						&real_motor_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	real_motor = (MX_MOTOR *) real_motor_record->record_class_struct;

	backlash = real_motor->quick_scan_backlash_correction;

	mx_status = mx_motor_compute_real_position_from_pseudomotor_position(
		motor_record, start_position, &real_start_position, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_compute_real_position_from_pseudomotor_position(
		motor_record, end_position, &real_end_position, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We perform the backlash correction no matter which direction the
	 * scan is going.
	 */

	if ( real_end_position >= real_start_position ) {
		real_backlash_position = real_start_position - fabs( backlash );
	} else {
		real_backlash_position = real_start_position + fabs( backlash );
	}

	mx_status = mx_motor_compute_pseudomotor_position_from_real_position(
		motor_record, real_backlash_position, backlash_position, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: motor '%s', start = %g, end = %g",
		fname, motor_record->name, start_position, end_position));

	MX_DEBUG( 2,
	("%s: real motor '%s', real start = %g, real end = %g, backlash = %g",
		fname, real_motor_record->name,
		real_start_position, real_end_position, backlash));

	MX_DEBUG( 2,("%s: motor '%s', real backlash position = %g",
		fname, motor_record->name, real_backlash_position ));

	MX_DEBUG( 2,("%s: motor '%s', backlash position = %g",
		fname, motor_record->name, *backlash_position ));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxs_mcs_quick_scan_compute_scan_parameters(
				MX_SCAN *scan,
				MX_QUICK_SCAN *quick_scan,
				MX_MCS_QUICK_SCAN *mcs_quick_scan,
				double measurement_time_per_point )
{
	static const char fname[] =
		"mxs_mcs_quick_scan_compute_scan_parameters()";

	MX_RECORD *motor_record;
	double raw_actual_measurements;
	long actual_num_measurements;
	double acceleration_time, longest_acceleration_time;
	double start_position, end_position, backlash_position;
	double real_start_position, real_end_position;
	long i;
	mx_status_type mx_status;

	/* Temporarily change the motor speeds so that we can correctly
	 * compute the acceleration times and distances.  We will restore
	 * the speeds before returning from this function.
	 */

	mx_status = mxs_mcs_quick_scan_set_motor_speeds( scan,
						quick_scan, mcs_quick_scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out what the longest acceleration time is. */

	longest_acceleration_time = 0.0;

	for ( i = 0; i < scan->num_motors; i++ ) {

		motor_record = (scan->motor_record_array)[i];

		mx_status = mx_motor_get_acceleration_time( motor_record,
							&acceleration_time );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_scan_restore_speeds( scan );
			return mx_status;
		}

		if ( acceleration_time > longest_acceleration_time ) {
			longest_acceleration_time = acceleration_time;
		}
	}

	/* If the longest acceleration time is zero, then we assume
	 * that the premove and postmove intervals are to be ignored.
	 * Otherwise, we calculate the contribution to the scan 
	 * duration of the premove and postmove intervals.
	 */

	if ( longest_acceleration_time < 1.0e-9 ) {

		mcs_quick_scan->premove_measurement_time = 0.0;
		mcs_quick_scan->postmove_measurement_time = 0.0;
		mcs_quick_scan->acceleration_time = 0.0;
		mcs_quick_scan->deceleration_time = 0.0;
		quick_scan->estimated_quick_scan_duration = 0.0;

	} else {
		/* The premove and postmove measurement times are time
		 * intervals used to measure the motor positions before
		 * the motors have started moving and after they have 
		 * stopped.
		 */

		mcs_quick_scan->premove_measurement_time
				= measurement_time_per_point
					* MXS_SQ_MCS_NUM_PREMOVE_MEASUREMENTS;

		mcs_quick_scan->postmove_measurement_time
				= mcs_quick_scan->premove_measurement_time;

		quick_scan->estimated_quick_scan_duration
			= 2.0 * mcs_quick_scan->premove_measurement_time;

		/* Add in the acceleration and deceleration intervals. */

		mcs_quick_scan->acceleration_time = longest_acceleration_time;
		mcs_quick_scan->deceleration_time = longest_acceleration_time;

		quick_scan->estimated_quick_scan_duration
				+= (2.0 * longest_acceleration_time);
	}

	/* Add the time for the body of the scan. */

	quick_scan->estimated_quick_scan_duration
				+= mcs_quick_scan->scan_body_time;

	MX_DEBUG( 2,("%s: mcs_quick_scan->premove_measurement_time = %g",
		fname, mcs_quick_scan->premove_measurement_time));

	MX_DEBUG( 2,("%s: mcs_quick_scan->acceleration_time = %g",
		fname, mcs_quick_scan->acceleration_time));

	MX_DEBUG( 2,("%s: mcs_quick_scan->scan_body_time = %g",
		fname, mcs_quick_scan->scan_body_time));

	MX_DEBUG( 2,("%s: mcs_quick_scan->deceleration_time = %g",
		fname, mcs_quick_scan->deceleration_time));

	MX_DEBUG( 2,("%s: mcs_quick_scan->postmove_measurement_time = %g",
		fname, mcs_quick_scan->postmove_measurement_time));

	MX_DEBUG( 2,("%s: quick_scan->estimated_quick_scan_duration = %g",
		fname, quick_scan->estimated_quick_scan_duration));

	MX_DEBUG( 2,("%s: quick_scan->requested_num_measurements = %ld",
		fname, quick_scan->requested_num_measurements));

	/*---*/

	if ( mcs_quick_scan->real_start_position == (double *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The real_start_position pointer for MCS quick scan '%s' is NULL.",
			scan->record->name );
	}
	if ( mcs_quick_scan->real_end_position == (double *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The real_end_position pointer for MCS quick scan '%s' is NULL.",
			scan->record->name );
	}
	if ( mcs_quick_scan->backlash_position == (double *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The backlash_position pointer for MCS quick scan '%s' is NULL.",
			scan->record->name );
	}

	/*---*/

	/* Compute the actual number of measurements required for this scan.
	 * For this case, we always round up.
	 */

	raw_actual_measurements = 1.0 + mx_divide_safely(
				quick_scan->estimated_quick_scan_duration,
				measurement_time_per_point );

	actual_num_measurements = mx_round( raw_actual_measurements );

	if ( raw_actual_measurements > (double) actual_num_measurements )
	{
		actual_num_measurements += 1L;
	}

	MX_DEBUG( 2,("%s: actual_num_measurements = %ld",
			fname, actual_num_measurements));

	quick_scan->actual_num_measurements = actual_num_measurements;

	for ( i = 0; i < scan->num_motors; i++ ) {

		/* Compute the real start and end positions for the motor. */

		motor_record = (scan->motor_record_array)[i];

		start_position = (quick_scan->start_position)[i];
		end_position   = (quick_scan->end_position)[i];

		mx_status = mx_motor_compute_extended_scan_range(
				motor_record, start_position, end_position,
				&real_start_position, &real_end_position );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_scan_restore_speeds( scan );
			return mx_status;
		}

		mcs_quick_scan->real_start_position[i] = real_start_position;
		mcs_quick_scan->real_end_position[i] = real_end_position;

		MX_DEBUG( 2,("%s: motor '%s' start_position = %g",
			fname, motor_record->name,
			(quick_scan->start_position)[i] ));

		MX_DEBUG( 2,("%s: motor '%s' end_position = %g",
			fname, motor_record->name,
			(quick_scan->end_position)[i] ));

		MX_DEBUG( 2,("%s: motor '%s' real_start_position = %g",
			fname, motor_record->name,
			(mcs_quick_scan->real_start_position)[i] ));

		MX_DEBUG( 2,("%s: motor '%s' real_end_position = %g",
			fname, motor_record->name,
			(mcs_quick_scan->real_end_position)[i] ));

		/* Also compute the quick scan backlash_position. */

		mx_status = mxs_mcs_quick_scan_compute_backlash_position(
				motor_record,
				real_start_position,
				real_end_position,
				&backlash_position );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_scan_restore_speeds( scan );
			return mx_status;
		}

		mcs_quick_scan->backlash_position[i] = backlash_position;

		MX_DEBUG( 2,("%s: motor '%s' backlash_position = %g",
			fname, motor_record->name,
			(mcs_quick_scan->backlash_position)[i] ));
	}

	/* Restore the speeds to the original values. */

	mx_status = mx_scan_restore_speeds( scan );

	return mx_status;
}

MX_EXPORT MX_RECORD *
mxs_mcs_quick_scan_find_encoder_readout( MX_RECORD *motor_record )
{
	static const char fname[] = "mxs_mcs_quick_scan_find_encoder_readout()";

	MX_RECORD *record_list;
	MX_RECORD *current_record, *encoder_record;
	MX_MCE *mce;
	long i, num_motors;
	MX_RECORD **motor_record_array;
	MX_NETWORK_MOTOR *network_motor;
	mx_status_type mx_status;

	if ( motor_record == (MX_RECORD *) NULL )
		return (MX_RECORD *) NULL;

	record_list = motor_record->list_head;

	encoder_record = NULL;

	current_record = record_list->next_record;

	while ( current_record != record_list ) {

		if ( ( current_record->mx_class == MXC_MULTICHANNEL_ENCODER )
		  && ( current_record->mx_superclass == MXR_DEVICE ) )
		{
			mx_status = mx_mce_get_motor_record_array(
					current_record, &num_motors,
					&motor_record_array );

			MXW_UNUSED( mx_status );

			for ( i = 0; i < num_motors; i++ ) {
				if ( motor_record == motor_record_array[i] ) {
					encoder_record = current_record;

					break;  /* Exit the for() loop. */
				}
			}

			if ( encoder_record != (MX_RECORD *) NULL ) {
				break;		/* Exit the while() loop. */
			}
		}
		current_record = current_record->next_record;
	}

	if ( encoder_record != (MX_RECORD *) NULL ) {
		mce = (MX_MCE *) encoder_record->record_class_struct;

		if ( mce == (MX_MCE *) NULL ) {
			(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MCE pointer for MCE record '%s' is NULL.",
				encoder_record->name );

			return NULL;
		}

		/* Save the 'name' of the selected motor. */

		if ( motor_record->mx_type != MXT_MTR_NETWORK ) {

			/* For non-MX network protocols, we _ASSUME_ that
			 * the foreign motor has the same name in the
			 * remote MX server's database as is used in our
			 * local database.  If this is not true, then
			 * the MCS quick scan will fail later on.
			 */

			strlcpy( mce->selected_motor_name,
				motor_record->name,
				MXU_RECORD_NAME_LENGTH );
		} else {
			network_motor = (MX_NETWORK_MOTOR *)
					motor_record->record_type_struct;

			if ( network_motor == (MX_NETWORK_MOTOR *) NULL ) {
			    (void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			    "The MX_NETWORK_MOTOR pointer for motor record "
				"'%s' is NULL.", motor_record->name );

			    return NULL;
			}

			strlcpy( mce->selected_motor_name,
				network_motor->remote_record_name,
				MXU_RECORD_NAME_LENGTH );
		}
	}

	return encoder_record;
}

static mx_status_type
mxs_mcs_quick_scan_use_dead_reckoning(
		MX_SCAN *scan,
		MX_QUICK_SCAN *quick_scan,
		MX_MCS_QUICK_SCAN *mcs_quick_scan,
		long motor_index,
		double *motor_position_array )
{
	static const char fname[] = "mxs_mcs_quick_scan_use_dead_reckoning()";
	
	MX_RECORD *pseudomotor_record;
	MX_RECORD *real_motor_record;
	MX_MEASUREMENT_PRESET_TIME *preset_time_struct;
	MX_MEASUREMENT_PRESET_PULSE_PERIOD *preset_pulse_period_struct;

	double pseudomotor_start_position, pseudomotor_end_position;
	double pseudomotor_real_start_position, pseudomotor_real_end_position;

	double real_motor_start_position, real_motor_end_position;
	double real_motor_real_start_position, real_motor_real_end_position;

	double fake_acceleration, fake_deceleration, fake_slew_velocity;
	double acceleration_distance, acceleration_time;
	double base_velocity;
	double measurement_time, current_time;
	long i, j;
	long first_acceleration_measurement, first_scan_body_measurement;
	long first_post_body_measurement, first_post_move_measurement;
	long num_acceleration_measurements;
	mx_status_type mx_status;

	/*******************************************************************/

	/* Get the start and end positions for the pseudomotor. */

	pseudomotor_start_position = (quick_scan->start_position)[motor_index];
	pseudomotor_end_position = (quick_scan->end_position)[motor_index];

	MX_DEBUG( 2,("%s: pseudomotor_start_position = %g",
					fname, pseudomotor_start_position));
	MX_DEBUG( 2,("%s: pseudomotor_end_position = %g",
					fname, pseudomotor_end_position));

	pseudomotor_real_start_position
		= (mcs_quick_scan->real_start_position)[motor_index];
	pseudomotor_real_end_position
		= (mcs_quick_scan->real_end_position)[motor_index];

	MX_DEBUG( 2,("%s: pseudomotor_real_start_position = %g",
				fname, pseudomotor_real_start_position));
	MX_DEBUG( 2,("%s: pseudomotor_real_end_position = %g",
				fname, pseudomotor_real_end_position));

	/* Compute the start and end positions for the real motor. */

	pseudomotor_record = ( scan->motor_record_array )[ motor_index ];

	mx_status = mx_motor_compute_real_position_from_pseudomotor_position(
			pseudomotor_record, pseudomotor_start_position,
			&real_motor_start_position, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_compute_real_position_from_pseudomotor_position(
			pseudomotor_record, pseudomotor_end_position,
			&real_motor_end_position, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: real_motor_start_position = %g",
					fname, real_motor_start_position));
	MX_DEBUG( 2,("%s: real_motor_end_position = %g",
					fname, real_motor_end_position));

	mx_status = mx_motor_compute_real_position_from_pseudomotor_position(
			pseudomotor_record, pseudomotor_real_start_position,
			&real_motor_real_start_position, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_compute_real_position_from_pseudomotor_position(
			pseudomotor_record, pseudomotor_real_end_position,
			&real_motor_real_end_position, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: real_motor_real_start_position = %g",
					fname, real_motor_real_start_position));
	MX_DEBUG( 2,("%s: real_motor_real_end_position = %g",
					fname, real_motor_real_end_position));

	/*******************************************************************/

	/* We first fill in motor_position_array with real motor positions
	 * and then later convert them to pseudomotor positions.
	 */

	/* During the premove period, the motor should not be moving. */

	first_acceleration_measurement = MXS_SQ_MCS_NUM_PREMOVE_MEASUREMENTS;

	MX_DEBUG( 2,("%s: first_acceleration_measurement = %ld",
			fname, first_acceleration_measurement));

	if ( first_acceleration_measurement
			>= quick_scan->actual_num_measurements )
	{
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"first_acceleration_measurement (%ld) "
			"> quick_scan->actual_num_measurements (%ld).",
			first_acceleration_measurement,
			quick_scan->actual_num_measurements );
	}

	for ( i = 0; i < first_acceleration_measurement; i++ ) {
		motor_position_array[i] = real_motor_real_start_position;
	}

	/* Get the measurement time per point. */

	switch( scan->measurement.type ) {
	case MXM_PRESET_TIME:
		preset_time_struct = (MX_MEASUREMENT_PRESET_TIME *)
				scan->measurement.measurement_type_struct;

		measurement_time = preset_time_struct->integration_time;
		break;
	case MXM_PRESET_PULSE_PERIOD:
		preset_pulse_period_struct =
				(MX_MEASUREMENT_PRESET_PULSE_PERIOD *)
				scan->measurement.measurement_type_struct;

		measurement_time = preset_pulse_period_struct->pulse_period;
		break;
	case MXM_PRESET_COUNT:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Preset count measurements are not currently supported "
		"for MCS quick scan '%s'.",
			scan->record->name );
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Measurement type '%ld' for MCS quick scan '%s' is unsupported.",
			scan->measurement.type, scan->record->name );
	}

	/* Compute the number of measurements during the acceleration. */

	acceleration_time = mcs_quick_scan->acceleration_time;

	num_acceleration_measurements = mx_round( mx_divide_safely(
						acceleration_time,
						measurement_time ) );

	MX_DEBUG( 2,("%s: num_acceleration_measurements = %ld",
			fname, num_acceleration_measurements));

	/* Estimate the motor position during the acceleration interval. */

	/* We pretend here that the acceleration is constant. */

	first_scan_body_measurement = first_acceleration_measurement
					+ num_acceleration_measurements;

	MX_DEBUG( 2,("%s: first_scan_body_measurement = %ld",
			fname, first_scan_body_measurement));

	if ( first_scan_body_measurement
			>= quick_scan->actual_num_measurements )
	{
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"first_scan_body_measurement (%ld) "
			"> quick_scan->actual_num_measurements (%ld).",
			first_scan_body_measurement,
			quick_scan->actual_num_measurements );
	}

	/* We need the acceleration distance and the base velocity for
	 * this calculation.
	 */

	acceleration_distance = fabs( real_motor_start_position
					- real_motor_real_start_position );

	MX_DEBUG( 2,("%s: acceleration_distance = %g",
		fname, acceleration_distance));

	mx_status = mx_motor_get_real_motor_record( pseudomotor_record,
							&real_motor_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: pseudomotor_record = '%s', real_motor_record = '%s'.",
		fname, pseudomotor_record->name, real_motor_record->name ));

	mx_status = mx_motor_get_base_speed( real_motor_record,
							&base_velocity );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: base_velocity = %g",
		fname, base_velocity ));

	fake_acceleration = mx_divide_safely(
	  2.0 * ( acceleration_distance - base_velocity * acceleration_time ),
		acceleration_time * acceleration_time );

	MX_DEBUG( 2,("%s: fake_acceleration = %g", 
		fname, fake_acceleration));

	for ( i = first_acceleration_measurement;
		i < first_scan_body_measurement;
		i++ )
	{
		j = i - first_acceleration_measurement;

		current_time = measurement_time * (double) j;

		MX_DEBUG( 2,("%s: time = %g, term1 = %g, term2 = %g",
			fname, current_time, base_velocity * current_time,
		    0.5 * fake_acceleration * current_time * current_time ));

		motor_position_array[i] = real_motor_real_start_position
		    + base_velocity * current_time
		    + 0.5 * fake_acceleration * current_time * current_time;
	}

	/* Estimate the motor position during the body of the scan. */

	MX_DEBUG( 2,("%s: quick_scan->requested_num_measurements = %ld",
		fname, quick_scan->requested_num_measurements ));

	first_post_body_measurement = first_scan_body_measurement
				+ quick_scan->requested_num_measurements - 1L;

	MX_DEBUG( 2,("%s: first_post_body_measurement = %ld",
		fname, first_post_body_measurement));

	if ( first_post_body_measurement
			>= quick_scan->actual_num_measurements )
	{
#if 0
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"first_post_body_measurement (%ld) "
			"> quick_scan->actual_num_measurements (%ld).",
			first_post_body_measurement,
			quick_scan->actual_num_measurements );
#else
		first_post_body_measurement
			= quick_scan->actual_num_measurements;
#endif
	}

	fake_slew_velocity = mx_divide_safely(
			real_motor_end_position - real_motor_start_position,
				mcs_quick_scan->scan_body_time );

	MX_DEBUG( 2,("%s: fake_slew_velocity = %g",
		fname, fake_slew_velocity));

	for ( i = first_scan_body_measurement;
		i < first_post_body_measurement;
		i++ )
	{
		j = i - first_scan_body_measurement;

		current_time = measurement_time * (double) j;

		motor_position_array[i] = real_motor_start_position
				+ fake_slew_velocity * current_time;
	}

	/* Now do the deceleration. */

	first_post_move_measurement = first_post_body_measurement
					+ num_acceleration_measurements;

	MX_DEBUG( 2,("%s: first_post_move_measurement = %ld",
		fname, first_post_move_measurement));

	if ( first_post_move_measurement
			>= quick_scan->actual_num_measurements )
	{
#if 0
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"first_post_move_measurement (%ld) "
			"> quick_scan->actual_num_measurements (%ld).",
			first_post_move_measurement,
			quick_scan->actual_num_measurements );
#else
		first_post_move_measurement
			= quick_scan->actual_num_measurements;
#endif
	}

	fake_deceleration = mx_divide_safely(
    2.0 * ( acceleration_distance - fake_slew_velocity * acceleration_time ),
		acceleration_time * acceleration_time );

	MX_DEBUG( 2,("%s: fake_deceleration = %g", 
		fname, fake_deceleration));

	for ( i = first_post_body_measurement;
		i < first_post_move_measurement;
		i++ )
	{
		j = i - first_post_body_measurement;

		current_time = measurement_time * (double) j;

		MX_DEBUG( 2,("%s: time = %g, term1 = %g, term2 = %g",
			fname, current_time, fake_slew_velocity * current_time,
		    - 0.5 * fake_acceleration * current_time * current_time ));

		motor_position_array[i] = real_motor_end_position
		    + fake_slew_velocity * current_time
		    - 0.5 * fake_acceleration * current_time * current_time;
	}

	/* The rest of the points are filled in using
	 * real_motor_real_end_position.
	 */

	for ( i = first_post_move_measurement;
		i < quick_scan->actual_num_measurements;
		i++ )
	{
		motor_position_array[i] = real_motor_real_end_position;
	}

	/* At this point, motor_position_array should be filled with
	 * motor positions in terms of the real motor.  We now return
	 * the array and leave it up to the caller to convert the positions
	 * back to pseudomotor positions.
	 */

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxs_mcs_quick_scan_use_encoder_values(
		MX_RECORD *pseudomotor_record,
		MX_QUICK_SCAN *quick_scan,
		MX_MCS_QUICK_SCAN *mcs_quick_scan,
		double pseudomotor_real_start_position,
		MX_RECORD *mce_record,
		double *motor_position_array,
		mx_bool_type read_mce )
{
	static const char fname[] = "mxs_mcs_quick_scan_use_encoder_values()";

	MX_MCE *mce;
	MX_RECORD *real_motor_record;
	MX_MOTOR *real_motor;
	long i, j;
	unsigned long num_encoder_values;
	double *encoder_value_array;
	double scaled_encoder_value, start_of_bin_value;
	double real_motor_real_start_position;
	mx_status_type mx_status;

	mce = (MX_MCE *) mce_record->record_class_struct;

	if ( mce == (MX_MCE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCE pointer for record '%s' is NULL.",
			mce_record->name );
	}

	/* If requested, read the MCE values.  Otherwise, we assume that
	 * the values already in the MCE array are correct and current.
	 */

	if ( read_mce ) {
		mx_status = mx_mce_read( mce_record,
				&num_encoder_values,
				&encoder_value_array );
	} else {
		num_encoder_values = mce->current_num_values;

		encoder_value_array = mce->value_array;

		mx_status = MX_SUCCESSFUL_RESULT;
	}

	if ( mx_status.code != MXE_SUCCESS ) {

		/* An error occurred while reading the encoder
		 * value array, so fill the position array with -1.
		 */

		for ( i = 0; i < quick_scan->actual_num_measurements; i++ ) {
			motor_position_array[i] = -1.0;
		}
	} else {
		mx_status = mx_motor_get_real_motor_record( pseudomotor_record,
							&real_motor_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		real_motor = (MX_MOTOR *)
				real_motor_record->record_class_struct;

		if ( real_motor == (MX_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MOTOR pointer for record '%s' is NULL.",
				real_motor_record->name );
		}

		/* Some motor controllers (such as Newport XPS) can do
		 * things like only send quadrature signals while the
		 * position of the motor is within a user specified
		 * window.  If the motor controller is using this feature,
		 * then we get the real motor's start position from
		 * the window in the motor controller.
		 */
		
		if ( mce->use_window ) {
			mx_status = mx_motor_get_window( real_motor_record,
					NULL, MXU_MTR_NUM_WINDOW_PARAMETERS );

			real_motor_real_start_position = real_motor->window[0];
		} else {
			/* If the motor controller does _not_ use a window,
			 * then we must compute the start position of the
			 * real motor using the provided pseudomotor
			 * start position.
			 */

			mx_status
		  = mx_motor_compute_real_position_from_pseudomotor_position(
				pseudomotor_record,
				pseudomotor_real_start_position,
				&real_motor_real_start_position,
				TRUE );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor_position_array[0] = real_motor_real_start_position;

		/* Find and display the multichannel encoder contents. */

		if ( num_encoder_values > quick_scan->actual_num_measurements )
		{
		    num_encoder_values = quick_scan->actual_num_measurements;
		}

#if 0
		MX_DEBUG( 2,("%s: num_encoder_values = %lu",
					fname, num_encoder_values));

		MX_DEBUG( 2,("%s: encoder_value_array is:", fname));

		if ( mx_get_debug_level() >= 2 ) {
			for( i = 0; i < num_encoder_values; i++ ) {
				fprintf(stderr,"%g ", encoder_value_array[i]);
			}
			fprintf(stderr,"\n");
		}

		MX_DEBUG( 2,("%s: motor_position_array[0] = %g",
			fname, motor_position_array[0]));
#endif

		if ( mce->use_window == FALSE ) {
			i = 1;
		} else {
			i = mce->measurement_window_offset + 1;
		}

		/* Compute the motor positions from the encoder readout. */

		switch( mce->encoder_type ) {
		case MXT_MCE_ABSOLUTE_ENCODER:
			for ( j = 1; i < num_encoder_values; i++, j++ ) {

				scaled_encoder_value = real_motor->offset
				  + real_motor->scale * encoder_value_array[i];

				motor_position_array[i] = scaled_encoder_value;
			}
			break;
		case MXT_MCE_INCREMENTAL_ENCODER:
			for ( j = 1; i < num_encoder_values; i++, j++ ) {

				scaled_encoder_value =
				  real_motor->scale * encoder_value_array[i];

				motor_position_array[j] =
					motor_position_array[j-1]
						+ scaled_encoder_value;
			}
			break;
		case MXT_MCE_DELTA_ENCODER:

			start_of_bin_value = motor_position_array[0];

			for ( j = 1; i < num_encoder_values; i++, j++ ) {

				scaled_encoder_value =
				  real_motor->scale * encoder_value_array[i];

				/* The scaled motor value reflects the distance
				 * that the motor has moved during this bin,
				 * so we should put the reported position
				 * in the middle of the bin.
				 */

				motor_position_array[j] = start_of_bin_value
						+ 0.5 * scaled_encoder_value;

				start_of_bin_value += scaled_encoder_value;

			}
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal MCE encoder type %ld.  This is a program bug.",
				mce->encoder_type );
		}

		/* Fill in the rest of the array (if any) with 0. */

		for ( ; j < quick_scan->actual_num_measurements; j++ ) {
			motor_position_array[i] = 0.0;
		}
	}

#if 1
	MX_DEBUG( 2,("%s results are:", fname));

	if ( mx_get_debug_level() >= 2 ) {
		for ( i = 0; i < quick_scan->actual_num_measurements; i++ ) {
			fprintf(stderr, "%g ", motor_position_array[i]);
		}
		fprintf(stderr,"\n");
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxs_mcs_quick_scan_convert_encoder_readings( MX_QUICK_SCAN *quick_scan,
					MX_RECORD *motor_record,
					double *motor_position_array )
{
	double pseudomotor_position;
	long i;
	mx_status_type mx_status;

	for ( i = 0; i < quick_scan->actual_num_measurements; i++ ) {

		mx_status =
		  mx_motor_compute_pseudomotor_position_from_real_position(
				motor_record,
				motor_position_array[i],
				&pseudomotor_position, TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor_position_array[i] = pseudomotor_position;
	}

#if 1
	if ( mx_get_debug_level() >= 2 ) {
		for ( i = 0; i < quick_scan->actual_num_measurements; i++ ) {
			fprintf(stderr, "%g ", motor_position_array[i]);
		}
		fprintf(stderr,"\n");
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxs_mcs_quick_scan_compute_pseudomotor_start_from_scan_start(
		MX_RECORD *scan_motor_record,
		MX_RECORD *pseudomotor_record,
		double scan_start_position,
		double *pseudomotor_start_position )
{
	static const char fname[] =
	    "mxs_mcs_quick_scan_compute_pseudomotor_start_from_scan_start()";

	double real_start_position;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s: scan_motor = '%s', pseudomotor = '%s'",
		fname, scan_motor_record->name, pseudomotor_record->name));

	mx_status = mx_motor_compute_real_position_from_pseudomotor_position(
			scan_motor_record, scan_start_position,
			&real_start_position, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_compute_pseudomotor_position_from_real_position(
			pseudomotor_record, real_start_position,
			pseudomotor_start_position, TRUE );

	MX_DEBUG( 2,
	("%s: scan_start = %g, real_start = %g, pseudomotor_start = %g",
	 	fname, scan_start_position, real_start_position,
		*pseudomotor_start_position));

	return mx_status;
}

#define FREE_SCAN_MOTOR_ARRAYS \
	do { \
		if ( scan_motor_array != NULL ) \
			mx_free( scan_motor_array ); \
		if ( scan_start_position != NULL ) \
			mx_free( scan_start_position ); \
	} while (0)

MX_EXPORT mx_status_type
mxs_mcs_quick_scan_default_compute_motor_positions(
		MX_SCAN *scan,
		MX_QUICK_SCAN *quick_scan,
		MX_MCS_QUICK_SCAN *mcs_quick_scan,
		MX_MCS *mcs )
{
	static const char fname[] =
		"mxs_mcs_quick_scan_default_compute_motor_positions()";

	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	MX_RECORD *quick_scan_motor_record;
	MX_RECORD *mce_record;
	double **motor_position_array;
	MX_RECORD **scan_motor_array;
	double *scan_start_position;
	double motor_scan_start_position, motor_pseudomotor_start_position;
	long j, k;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname ));

	if ( scan->num_motors <= 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Allocate some arrays to save the real motor records and real
	 * start positions in so that we can find the real start positions
	 * if needed by alternate datafile X motors or alternate plot
	 * X motors.
	 */

	scan_motor_array = (MX_RECORD **)
			malloc( scan->num_motors * sizeof(MX_RECORD *) );

	if ( scan_motor_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory attempting to allocate a %ld element array "
	"of MX_RECORD pointers for the scan_motor_array data structure.",
			scan->num_motors );
	}

	scan_start_position = (double *)
			malloc( scan->num_motors * sizeof(double) );

	if ( scan_start_position == (double *) NULL ) {
		mx_free( scan_motor_array );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory attempting to allocate a %ld element array "
	"of doubles for the scan_start_position data structure.",
			scan->num_motors );
	}

	/* Compute the motor positions. */

	motor_position_array = mcs_quick_scan->motor_position_array;

	for ( j = 0; j < scan->num_motors; j++ ) {

		motor_record = scan->motor_record_array[j];

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		/*
		 * If the motor is not a pseudomotor, the encoder readings
		 * will reflect the motor's position directly.  However, if
		 * this motor is a pseudomotor, the encoder readings will
		 * depend on the position of the underlying real motor.
		 * Thus, for pseudomotors, we must get a pointer to the
		 * underlying real motor record.
		 *
		 * This does not work for pseudomotors that have multiple
		 * underlying real motors, but those records should have
		 * the flag MXF_MTR_CANNOT_QUICK_SCAN set in 
		 * motor->motor_flags.
		 */

		mx_status = mx_motor_get_real_motor_record(
				motor_record, &quick_scan_motor_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_SCAN_MOTOR_ARRAYS;
			return mx_status;
		}

		scan_motor_array[j] = quick_scan_motor_record;

		mce_record = mxs_mcs_quick_scan_find_encoder_readout(
						quick_scan_motor_record );

		/* Compute the real motor positions. */

		if ( mce_record == (MX_RECORD *) NULL ) {

			scan_start_position[j] = 0.0;

			for ( k = 0; k < 3; k++ ) {
				mx_info("\007");
				mx_msleep(150);
			}

			mx_msleep(500);

			for ( k = 0; k < 3; k++ ) {
				mx_info("\007");
				mx_msleep(150);
			}

			mx_info(
"Computing estimated positions for motor '%s' using DEAD RECKONING.\n"
"\n"
"*********************************************************************\n"
"*****       These are NOT accurate, measured positions.         *****\n"
"*****                                                           *****\n"
"***** These results should NOT be used for real measurements.   *****\n"
"***** For a real measurement, you MUST use a step scan instead. *****\n"
"*********************************************************************\n",
				motor_record->name );

			mx_info_dialog(
	"Press any key to continue with the dead reckoning calculation...",
	"Press OK to continue with the dead reckoning calculation.",
				"OK" );

			(void) mxs_mcs_quick_scan_use_dead_reckoning(
					scan,
					quick_scan,
					mcs_quick_scan,
					j,
					motor_position_array[j] );
		} else {
			mx_info(
"Computing positions for motor '%s' using multichannel encoder '%s'",
				motor_record->name,
				mce_record->name );

			(void) mxs_mcs_quick_scan_use_encoder_values(
					motor_record,
					quick_scan,
					mcs_quick_scan,
					mcs_quick_scan->real_start_position[j],
					mce_record,
					motor_position_array[j],
					TRUE );

			scan_start_position[j] = motor_position_array[j][0];

			MX_DEBUG( 2,
			("%s: motor[%ld] = '%s', scan_start_position[%ld] = %g",
				fname, j, motor_record->name,
				j, scan_start_position[j]));
		}

		/* If the motor is a pseudomotor, convert the real motor
		 * positions into pseudomotor positions.
		 */

		if ( motor->motor_flags & MXF_MTR_IS_PSEUDOMOTOR ) {
			(void) mxs_mcs_quick_scan_convert_encoder_readings(
					quick_scan,
					motor_record,
					motor_position_array[j] );
		}
	}

	/* Compute positions for the alternate datafile X motors (if any). */

	for ( j = 0; j < scan->datafile.num_x_motors; j++ ) {

		motor_record = scan->datafile.x_motor_array[j];

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		mx_status = mx_motor_get_real_motor_record(
				motor_record, &quick_scan_motor_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_SCAN_MOTOR_ARRAYS;
			return mx_status;
		}

		mce_record = mxs_mcs_quick_scan_find_encoder_readout(
						quick_scan_motor_record );

		if ( mce_record == (MX_RECORD *) NULL ) {
			mx_warning(
"\007Cannot find a multichannel encoder for alternate datafile X motor '%s'.  "
"The reported positions for the alternate X motor will be incorrect.\007",
				motor_record->name );

			continue; /* Skip to the next step of the for() loop. */
		}

		/* Find the alternate real start position. */

		for ( k = 0; k < scan->num_motors; k++ ) {
			if ( quick_scan_motor_record == scan_motor_array[k] )
			{
				break;    /* Exit the inner for() loop. */
			}
		}

		if ( k >= scan->num_motors ) {
			mx_warning(
"\007Cannot find the real start position for alternate datafile X motor '%s'.  "
"The reported positions for the alternate X motor will be relative to zero.\007",
				motor_record->name );

			motor_scan_start_position = 0.0;
			motor_pseudomotor_start_position = 0.0;
		} else {
			motor_scan_start_position = scan_start_position[k];

			mx_status =
		mxs_mcs_quick_scan_compute_pseudomotor_start_from_scan_start(
				quick_scan_motor_record, motor_record,
				motor_scan_start_position,
				&motor_pseudomotor_start_position );

			if ( mx_status.code != MXE_SUCCESS ) {
				FREE_SCAN_MOTOR_ARRAYS;
				return mx_status;
			}
		}

		MX_DEBUG( 2,
		("%s: motor = '%s', motor_pseudomotor_start_position = %g",
			fname, motor_record->name,
			motor_pseudomotor_start_position));

		/* Get the alternate real positions.  It should not be
		 * necessary to read the MCE again since we should have
		 * done that while reading out the real motors.
		 */

		mx_info(
		"Computing positions of alternate datafile motor '%s'",
			motor_record->name );

		(void) mxs_mcs_quick_scan_use_encoder_values(
				motor_record,
				quick_scan,
				mcs_quick_scan,
				motor_pseudomotor_start_position,
				mce_record,
				scan->datafile.x_position_array[j],
				FALSE );

		/* If the motor is a pseudomotor, convert the real motor
		 * positions into pseudomotor positions.
		 */

		if ( motor->motor_flags & MXF_MTR_IS_PSEUDOMOTOR ) {
			(void) mxs_mcs_quick_scan_convert_encoder_readings(
					quick_scan,
					motor_record,
					scan->datafile.x_position_array[j] );
		}
	}

	/* Compute positions for the alternate plot X motors (if any). */

	for ( j = 0; j < scan->plot.num_x_motors; j++ ) {

		motor_record = scan->plot.x_motor_array[j];

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		mx_status = mx_motor_get_real_motor_record(
				motor_record, &quick_scan_motor_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_SCAN_MOTOR_ARRAYS;
			return mx_status;
		}

		mce_record = mxs_mcs_quick_scan_find_encoder_readout(
						quick_scan_motor_record );

		if ( mce_record == (MX_RECORD *) NULL ) {
			mx_warning(
"\007Cannot find a multichannel encoder for alternate plot X motor '%s'.  "
"The positions for the alternate X motor will be incorrect.\007",
				motor_record->name );

			continue; /* Skip to the next step of the for() loop. */
		}

		/* Find the alternate real start position. */

		for ( k = 0; k < scan->num_motors; k++ ) {
			if ( quick_scan_motor_record == scan_motor_array[k] )
			{
				break;    /* Exit the inner for() loop. */
			}
		}

		if ( k >= scan->num_motors ) {
			mx_warning(
"\007Cannot find the real start position for alternate plot X motor '%s'.  "
"The reported positions for the alternate X motor will be relative to zero.\007",
				motor_record->name );

			motor_scan_start_position = 0.0;
			motor_pseudomotor_start_position = 0.0;
		} else {
			motor_scan_start_position = scan_start_position[k];

			mx_status =
		mxs_mcs_quick_scan_compute_pseudomotor_start_from_scan_start(
				quick_scan_motor_record, motor_record,
				motor_scan_start_position,
				&motor_pseudomotor_start_position );

			if ( mx_status.code != MXE_SUCCESS ) {
				FREE_SCAN_MOTOR_ARRAYS;
				return mx_status;
			}
		}

		MX_DEBUG( 2,
		("%s: motor = '%s', motor_pseudomotor_start_position = %g",
			fname, motor_record->name,
			motor_pseudomotor_start_position));

		/* Get the alternate real positions.  It should not be
		 * necessary to read the MCE again since we should have
		 * done that while reading out the real motors.
		 */

		mx_info(
		"Computing positions of alternate plot motor '%s'",
			motor_record->name );

		(void) mxs_mcs_quick_scan_use_encoder_values(
				motor_record,
				quick_scan,
				mcs_quick_scan,
				motor_pseudomotor_start_position,
				mce_record,
				scan->plot.x_position_array[j],
				FALSE );

		/* If the motor is a pseudomotor, convert the real motor
		 * positions into pseudomotor positions.
		 */

		if ( motor->motor_flags & MXF_MTR_IS_PSEUDOMOTOR ) {
			(void) mxs_mcs_quick_scan_convert_encoder_readings(
					quick_scan,
					motor_record,
					scan->plot.x_position_array[j] );
		}
	}

	FREE_SCAN_MOTOR_ARRAYS;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_mcs_quick_scan_move_absolute_and_wait( MX_SCAN *scan,
					double *position_array )
{
#if DEBUG_PAUSE_REQUEST
	static const char fname[] =
		"mxs_mcs_quick_scan_move_absolute_and_wait()";
#endif

	mx_bool_type exit_loop;
	mx_status_type mx_status;

	mx_status = mx_motor_array_move_absolute( scan->num_motors,
						scan->motor_record_array,
						position_array,
						MXF_MTR_NOWAIT );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	exit_loop = FALSE;

	while (exit_loop == FALSE) {
		mx_status = mx_wait_for_motor_array_stop( scan->num_motors,
						scan->motor_record_array, 0 );

#if DEBUG_PAUSE_REQUEST
		MX_DEBUG(-2,
		("%s: mx_wait_for_motor_array_stop(), mx_status = %ld",
			fname, mx_status.code));
#endif
		switch( mx_status.code ) {
		case MXE_SUCCESS:
			exit_loop = TRUE;
			break;

		case MXE_PAUSE_REQUESTED:
#if DEBUG_PAUSE_REQUEST
			MX_DEBUG(-2,("%s: PAUSE - wait for motors", fname));
#endif
			mx_status = mx_scan_handle_pause_request( scan );
			break;

		default:
			break;
		}

		if ( mx_status.code != MXE_SUCCESS ) {
#if DEBUG_PAUSE_REQUEST
			MX_DEBUG(-2,("%s: Aborting on error code %ld",
				fname, mx_status.code));
#endif
			return mx_status;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_mcs_quick_scan_default_move_to_start( MX_SCAN *scan,
				MX_QUICK_SCAN *quick_scan,
				MX_MCS_QUICK_SCAN *mcs_quick_scan,
				double measurement_time,
				mx_bool_type correct_for_quick_scan_backlash )
{
#if DEBUG_TIMING
	static const char fname[] =
		"mxs_mcs_quick_scan_default_move_to_start()";
#endif

	mx_status_type mx_status;

#if DEBUG_TIMING
	MX_HRT_TIMING timing_measurement;

	MX_HRT_START( timing_measurement );
#endif

	/**** Send the motors to the start position for the scan. ****/

	mx_info("Moving to the start of the scan region.");

	mx_status = mxs_mcs_quick_scan_move_absolute_and_wait( scan,
						quick_scan->start_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_info("Move complete.");

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname, "move to start position" );

	MX_HRT_START( timing_measurement );
#endif

	/***********************************************************
	 * Compute pre-move and post-move scan parameters to allow *
	 * the actual number of measurements and the acceleration  *
	 * distances to be computed.  This function also computes  *
	 * the quick scan backlash position.                       *
	 ***********************************************************/

	mx_status = mxs_mcs_quick_scan_compute_scan_parameters(
			scan, quick_scan, mcs_quick_scan,
			measurement_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"compute quick scan parameters" );

	MX_HRT_START( timing_measurement );
#endif

	/**** Move to the quick scan backlash position. ****/

	if ( correct_for_quick_scan_backlash ) {
		mx_info("Correcting for quick scan backlash." );

		mx_status = mxs_mcs_quick_scan_move_absolute_and_wait( scan,
					mcs_quick_scan->backlash_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_info("Correction for quick scan backlash complete." );
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"correcting for quick scan backlash" );

	MX_HRT_START( timing_measurement );
#endif

	/**** Move to the 'real' start position. ****/

	mx_info("Moving to the start position.");

	mx_status = mxs_mcs_quick_scan_move_absolute_and_wait( scan,
					mcs_quick_scan->real_start_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_info("All motors are at the start position.\n");

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"move to real start position" );

	MX_HRT_START( timing_measurement );
#endif

	/**** Set the motor speeds for the quick scan. ****/

	mx_status = mxs_mcs_quick_scan_set_motor_speeds( scan,
						quick_scan, mcs_quick_scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"set quick scan motor speeds" );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_mcs_quick_scan_prepare_for_scan_start( MX_SCAN *scan )
{
	static const char fname[] =
		"mxs_mcs_quick_scan_prepare_for_scan_start()";

	MX_RECORD *motor_record;
	MX_RECORD *clock_record;
	MX_RECORD *synchronous_motion_mode_record;
	MX_RECORD *quick_scan_motor_record;
	MX_RECORD *mcs_record;
	MX_MCS *mcs;
	MX_MOTOR *motor;
	MX_MOTOR *quick_scan_motor;
	MX_QUICK_SCAN *quick_scan;
	MX_MCS_QUICK_SCAN *mcs_quick_scan;
	MX_MEASUREMENT_PRESET_TIME *preset_time_struct;
	MX_MEASUREMENT_PRESET_PULSE_PERIOD *preset_pulse_period_struct;
	double measurement_time;
	double qs_backlash, qs_ratio;
	int motor_is_compatible, this_motor_is_compatible;
	long i, j;
	long dimension[2];
	size_t element_size[2];
	long readout_preference;
	mx_bool_type correct_for_quick_scan_backlash;
	mx_status_type mx_status;

	mx_status_type (*move_to_start_fn)( MX_SCAN *,
					MX_QUICK_SCAN *,
					MX_MCS_QUICK_SCAN *,
					double,
					mx_bool_type );

#if DEBUG_TIMING
	MX_HRT_TIMING timing_measurement;
#endif

	MX_DEBUG( 2,("%s invoked.", fname));

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	correct_for_quick_scan_backlash = FALSE;

	/* Figure out what kind of measurement type this is and get the
	 * clock record for it.
	 */

#if DEBUG_TIMING
	MX_HRT_START( timing_measurement );
#endif

	switch( scan->measurement.type ) {
	case MXM_PRESET_TIME:
		preset_time_struct = (MX_MEASUREMENT_PRESET_TIME *)
				scan->measurement.measurement_type_struct;

		measurement_time = preset_time_struct->integration_time;

		clock_record = preset_time_struct->timer_record;

		if ( mx_verify_driver_type( clock_record,
			MXR_DEVICE, MXC_TIMER, MXT_TIM_MCS ) == FALSE )
		{
			return mx_error( MXE_TYPE_MISMATCH, fname,
		"MCS quick scan '%s' is configured for a preset time "
		"measurement, but the timer record '%s' is not an MCS timer.",
				scan->record->name, clock_record->name );
		}
		break;
	case MXM_PRESET_PULSE_PERIOD:
		preset_pulse_period_struct =
				(MX_MEASUREMENT_PRESET_PULSE_PERIOD *)
				scan->measurement.measurement_type_struct;

		measurement_time = preset_pulse_period_struct->pulse_period;

		clock_record = 
			preset_pulse_period_struct->pulse_generator_record;

		if ( mx_verify_driver_type( clock_record,
			MXR_DEVICE, MXC_PULSE_GENERATOR, MXT_ANY ) == FALSE )
		{
			return mx_error( MXE_TYPE_MISMATCH, fname,
		"MCS quick scan '%s' is configured for a preset pulse period "
		"measurement, but the pulse generator record '%s' is not "
		"actually a pulse generator.",
				scan->record->name, clock_record->name );
		}
		break;
	case MXM_PRESET_COUNT:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Preset count measurements are not currently supported "
		"for MCS quick scan '%s'.",
			scan->record->name );
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Measurement type '%ld' for MCS quick scan '%s' is unsupported.",
			scan->measurement.type, scan->record->name );
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname, "get clock record" );

	MX_HRT_START( timing_measurement );
#endif

	/* Connect the real motor(s) to their multichannel encoders. */

	mcs_quick_scan->real_motor_record_array = (MX_RECORD **)
			malloc( scan->num_motors * sizeof(MX_RECORD *) );

	if ( mcs_quick_scan->real_motor_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Could not allocate a %ld element real_motor_record_array "
		"for scan '%s'.", scan->num_motors, scan->record->name );
	}

	mcs_quick_scan->mce_record_array = (MX_RECORD **)
			malloc( scan->num_motors * sizeof(MX_RECORD *) );

	if ( mcs_quick_scan->mce_record_array == (MX_RECORD **) NULL ) {
		mx_free( mcs_quick_scan->real_motor_record_array );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Could not allocate a %ld element mce_record_array "
		"for scan '%s'.", scan->num_motors, scan->record->name );
	}

	for ( i = 0; i < scan->num_motors; i++ ) {

		motor_record = scan->motor_record_array[i];

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		mx_status = mx_motor_get_real_motor_record(
				motor_record, &quick_scan_motor_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_free( mcs_quick_scan->real_motor_record_array );
			mx_free( mcs_quick_scan->mce_record_array );
			return mx_status;
		}

		mcs_quick_scan->real_motor_record_array[i]
					= quick_scan_motor_record;

		/* Only perform quick scan backlash correction if one or
		 * more of the quick scan motors has a non-zero value
		 * for the correction.
		 */

		quick_scan_motor = (MX_MOTOR *)
			quick_scan_motor_record->record_class_struct;

		if ( quick_scan_motor == (MX_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_MOTOR pointer for quick scan motor '%s' is NULL.",
				quick_scan_motor_record->name );
		}

		qs_backlash = quick_scan_motor->quick_scan_backlash_correction;

		qs_ratio = mx_divide_safely( qs_backlash,
				quick_scan_motor->scale );

		if ( fabs(qs_ratio) > 1.0e-6 ) {
			correct_for_quick_scan_backlash = TRUE;
		}

		mcs_quick_scan->mce_record_array[i]
			= mxs_mcs_quick_scan_find_encoder_readout(
					quick_scan_motor_record );

		if ( mcs_quick_scan->mce_record_array[i] == (MX_RECORD *) NULL )		{
			for ( j = 0; j < 3; j++ ) {
				mx_info("\007");
				mx_msleep(150);
			}

			mx_warning(
"*** Quick scan motor '%s' does NOT have an MCS encoder readout. ***\n"
"Estimated motor positions will be used instead of measured motor positions.\n"
"\n"
"*** It is probably a BAD IDEA to run an MCS scan of this sort. ***\n"
"*** If you want accurate motor positions, you MUST use a step scan instead. ***\n",
				motor_record->name );

			mx_msleep(500);

			for ( j = 0; j < 3; j++ ) {
				mx_info("\007");
				mx_msleep(150);
			}
		} else {
			mx_status = mx_mce_connect_mce_to_motor(
				mcs_quick_scan->mce_record_array[i],
				mcs_quick_scan->real_motor_record_array[i] );

			if ( mx_status.code != MXE_SUCCESS ) {
			    mx_free( mcs_quick_scan->mce_record_array );
			    mx_free( mcs_quick_scan->real_motor_record_array );
			    return mx_status;
			}
		}
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname, "setup encoder readout" );

	MX_HRT_START( timing_measurement );
#endif

	/* Check each alternate datafile X motor to see if the real scan
	 * motors of the scan will be generating encoder positions that
	 * the alternate X motor can use.
	 */

	for ( i = 0; i < scan->datafile.num_x_motors; i++ ) {

		motor_is_compatible = FALSE;

		for ( j = 0; j < scan->num_motors; j++ ) {

			mx_status = mx_alternate_motor_can_use_this_motors_mce(
				mcs_quick_scan->real_motor_record_array[j],
				scan->datafile.x_motor_array[i],
				&this_motor_is_compatible );

			if ( mx_status.code != MXE_SUCCESS ) {
			    mx_free( mcs_quick_scan->mce_record_array );
			    mx_free( mcs_quick_scan->real_motor_record_array );
			    return mx_status;
			}

			if ( this_motor_is_compatible ) {
			    motor_is_compatible = TRUE;

			    break;	/* Exit the inner for() loop. */
			}
		}

		if ( motor_is_compatible == FALSE ) {
			mx_free( mcs_quick_scan->mce_record_array );
			mx_free( mcs_quick_scan->real_motor_record_array );

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Alternate datafile X motor '%s' cannot be used by quick scan '%s' "
	"since the quick scan will not generate encoder positions that "
	"can be used to compute the position of motor '%s'.",
				scan->datafile.x_motor_array[i]->name,
				scan->record->name,
				scan->datafile.x_motor_array[i]->name );
		}
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"check for alternate datafile X motor" );

	MX_HRT_START( timing_measurement );
#endif

	/* Do the same for the alternative plot X axis motors. */

	for ( i = 0; i < scan->plot.num_x_motors; i++ ) {

		motor_is_compatible = FALSE;

		for ( j = 0; j < scan->num_motors; j++ ) {

			mx_status = mx_alternate_motor_can_use_this_motors_mce(
				mcs_quick_scan->real_motor_record_array[j],
				scan->plot.x_motor_array[i],
				&this_motor_is_compatible );

			if ( mx_status.code != MXE_SUCCESS ) {
			    mx_free( mcs_quick_scan->mce_record_array );
			    mx_free( mcs_quick_scan->real_motor_record_array );
			    return mx_status;
			}

			if ( this_motor_is_compatible ) {
			    motor_is_compatible = TRUE;

			    break;	/* Exit the inner for() loop. */
			}
		}

		if ( motor_is_compatible == FALSE ) {
			mx_free( mcs_quick_scan->mce_record_array );
			mx_free( mcs_quick_scan->real_motor_record_array );

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Alternate plot X motor '%s' cannot be used by quick scan '%s' "
	"since the quick scan will not generate encoder positions that "
	"can be used to compute the position of motor '%s'.",
				scan->plot.x_motor_array[i]->name,
				scan->record->name,
				scan->plot.x_motor_array[i]->name );
		}
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"check for alternate plot X motor" );

	MX_HRT_START( timing_measurement );
#endif
	/**** Compute the time required for the body of the scan. ****/

	mcs_quick_scan->scan_body_time = measurement_time
		* (double) ( quick_scan->requested_num_measurements - 1L );

	/* Move to the start position. */

	move_to_start_fn = mcs_quick_scan->move_to_start_fn;

	if ( move_to_start_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The move_to_start_fn pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	mx_status = ( *move_to_start_fn )( scan,
				quick_scan, mcs_quick_scan,
				measurement_time,
				correct_for_quick_scan_backlash );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"Total time for move to start position" );
#endif

	/* Figure out what kind of MCS readout preference to use for this scan.
	 *
	 * The available preference types in order from most desirable to
	 * least desirable is:
	 *
	 *   MXF_MCS_PREFER_READ_MEASUREMENT
	 *   MXF_MCS_PREFER_READ_SCALER
         *   MXF_MCS_PREFER_READ_ALL
	 *
	 * MXF_MCS_PREFER_READ_MEASUREMENT is most desirable since it gives
	 * the user the quickest feedback, while MXF_MCS_PREFER_READ_ALL is
	 * the least desirable, since it may require reading out data from
	 * MCS channels the user is not using.
	 *
	 * We loop through all of the MCS records to see what is the most
	 * restrictive preference used by any of the MCS records.
	 */

	readout_preference = MXF_MCS_PREFER_READ_MEASUREMENT;

	for ( i = 0; i < mcs_quick_scan->num_mcs; i++ ) {
		mcs_record = mcs_quick_scan->mcs_record_array[i];

		mcs = (MX_MCS *) mcs_record->record_class_struct;

		switch( mcs->readout_preference ) {
		case MXF_MCS_PREFER_READ_MEASUREMENT:
			switch( readout_preference ) {
			case MXF_MCS_PREFER_READ_ALL:
			case MXF_MCS_PREFER_READ_SCALER:
				break;
			case MXF_MCS_PREFER_READ_MEASUREMENT:
				readout_preference =
					MXF_MCS_PREFER_READ_MEASUREMENT;
				break;
			}
			break;
		case MXF_MCS_PREFER_READ_SCALER:
			switch( readout_preference ) {
			case MXF_MCS_PREFER_READ_ALL:
				break;
			case MXF_MCS_PREFER_READ_SCALER:
			case MXF_MCS_PREFER_READ_MEASUREMENT:
				readout_preference = MXF_MCS_PREFER_READ_SCALER;
				break;
			}
			break;
		case MXF_MCS_PREFER_READ_ALL:
			switch( readout_preference ) {
			case MXF_MCS_PREFER_READ_ALL:
			case MXF_MCS_PREFER_READ_SCALER:
			case MXF_MCS_PREFER_READ_MEASUREMENT:
				readout_preference = MXF_MCS_PREFER_READ_ALL;
				break;
			}
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"MCS '%s' is configured with illegal readout "
			"preference %ld.  The allowed values are "
			"'read scaler' (1), 'read measurement' (2), "
			"and 'read all' (3).",
				mcs_record->name,
				mcs->readout_preference );
			break;
		}
	}

#if DEBUG_SCAN_PROGRESS
	MX_DEBUG(-2,("%s: Scan '%s' MCS readout preference = %ld",
		fname, scan->record->name, readout_preference ));
#endif

	mcs_quick_scan->mcs_readout_preference = readout_preference;

	/* Reprogram all of the MCSs for this scan. */

	for ( i = 0; i < mcs_quick_scan->num_mcs; i++ ) {

#if DEBUG_TIMING
		MX_HRT_START( timing_measurement );
#endif

		mcs_record = mcs_quick_scan->mcs_record_array[i];

		/**** Put the MCS into preset time mode. ****/

		mx_status = mx_mcs_set_mode( mcs_record, MXM_PRESET_TIME );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_scan_restore_speeds( scan );
			return mx_status;
		}

#if DEBUG_TIMING
		MX_HRT_END( timing_measurement );
		MX_HRT_RESULTS( timing_measurement, fname,
			"set '%s' to preset time mode.", mcs_record->name );

		MX_HRT_START( timing_measurement );
#endif

		/**** Set the measurement time per point. ****/

		mx_status = mx_mcs_set_measurement_time( mcs_record,
							measurement_time );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_scan_restore_speeds( scan );
			return mx_status;
		}

#if DEBUG_TIMING
		MX_HRT_END( timing_measurement );
		MX_HRT_RESULTS( timing_measurement, fname,
			"set '%s' measurement time to %g.",
			mcs_record->name, measurement_time );

		MX_HRT_START( timing_measurement );
#endif

		/**** Set the number of measurements. ****/

		mx_status = mx_mcs_set_num_measurements( mcs_record,
			(unsigned long) quick_scan->actual_num_measurements );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_scan_restore_speeds( scan );
			return mx_status;
		}

#if DEBUG_TIMING
		MX_HRT_END( timing_measurement );
		MX_HRT_RESULTS( timing_measurement, fname,
			"set '%s' num measurements to %ld.",
			mcs_record->name,
			quick_scan->actual_num_measurements );

		MX_HRT_START( timing_measurement );
#endif

		/**** Erase the previous contents of the MCS. */

		mx_status = mx_mcs_clear( mcs_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_scan_restore_speeds( scan );
			return mx_status;
		}

#if DEBUG_TIMING
		MX_HRT_END( timing_measurement );
		MX_HRT_RESULTS( timing_measurement, fname,
			"clear MCS '%s'.", mcs_record->name );

		MX_HRT_START( timing_measurement );
#endif

		/**** Reprogram the channel advance for this measurement. */

		switch( scan->measurement.type ) {
		case MXM_PRESET_TIME:
			/* Disable external channel advance for a preset
			 * time measurement.  The MCS will use its own
			 * internal clock.
			 */

			mx_status = mx_mcs_set_external_channel_advance(
							mcs_record, FALSE );

			if ( mx_status.code != MXE_SUCCESS ) {
				(void) mx_scan_restore_speeds( scan );
				return mx_status;
			}
			break;

		case MXM_PRESET_PULSE_PERIOD:
			/* Enable external channel advance for a measurement
			 * that uses an external pulse generator as a clock.
			 */

			mx_status = mx_mcs_set_external_channel_advance(
							mcs_record, TRUE );

			if ( mx_status.code != MXE_SUCCESS ) {
				(void) mx_scan_restore_speeds( scan );
				return mx_status;
			}

			/* We want each pulse generator pulse to move the MCS
			 * to the next channel, so set the external prescale
			 * value to 1.
			 */

			mx_status = mx_mcs_set_external_prescale(mcs_record, 1);

			if ( mx_status.code != MXE_SUCCESS ) {
				(void) mx_scan_restore_speeds( scan );
				return mx_status;
			}

			/* Give the MCS the start signal.  The MCS will not
			 * actually start counting until the first clock
			 * pulse comes in from the pulse generator after
			 * mx_pulse_generator_start() is invoked.
			 */

			mx_status = mx_mcs_start( mcs_record );

			if ( mx_status.code != MXE_SUCCESS ) {
				(void) mx_scan_restore_speeds( scan );
				return mx_status;
			}
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
	"Measurement type %ld is not supported for MCS quick scan '%s'.",
				scan->measurement.type, scan->record->name );
			break;
		}

#if DEBUG_TIMING
		MX_HRT_END( timing_measurement );
		MX_HRT_RESULTS( timing_measurement, fname,
			"reprogramming '%s' channel advance.",
			mcs_record->name );

		MX_HRT_START( timing_measurement );
#endif
	}

	/* If the clock record is a pulse generator, then reprogram it too. */

	if ( scan->measurement.type == MXM_PRESET_PULSE_PERIOD ) {

#if DEBUG_TIMING
		MX_HRT_START( timing_measurement );
#endif

#if 1 /* WML: FIXME - This is a "temporary" kludge. */
		{
			MX_RECORD *kludge_record;
			long pulse_mode;

			kludge_record = mx_get_record(clock_record,
							"mx_pulse_tweak");

			if ( kludge_record == NULL ) {
				pulse_mode = MXF_PGN_PULSE;
			} else {
				mx_status = mx_get_long_variable( kludge_record,
								&pulse_mode );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
			
			mx_status = mx_pulse_generator_set_mode( clock_record,
								pulse_mode );
		}
#else /* WML */
		mx_status = mx_pulse_generator_set_mode( clock_record,
							MXF_PGN_PULSE );
#endif /* WML */

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_pulse_generator_set_pulse_period( clock_record,
							measurement_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_pulse_generator_set_pulse_width( clock_record,
						0.01 * measurement_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Add one extra pulse to start the first measurement. */

		mx_status = mx_pulse_generator_set_num_pulses( clock_record,
				quick_scan->actual_num_measurements + 1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if DEBUG_TIMING
		MX_HRT_END( timing_measurement );
		MX_HRT_RESULTS( timing_measurement, fname,
			"reprogram pulse generator '%s'.", clock_record->name );
#endif
	}

	/**** Allocate memory for the motor position array. ****/

#if DEBUG_TIMING
	MX_HRT_START( timing_measurement );
#endif

	dimension[0] = scan->num_motors;
	dimension[1] = quick_scan->actual_num_measurements;

	element_size[0] = sizeof(double);
	element_size[1] = sizeof(double *);

	mcs_quick_scan->motor_position_array =
		mx_allocate_array( MXFT_DOUBLE, 2, dimension, element_size );

	if ( mcs_quick_scan->motor_position_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld by %ld array of "
		"motor position values for scan '%s'.",
			dimension[0], dimension[1], scan->record->name );
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"allocate memory for the motor position array" );

	MX_HRT_START( timing_measurement );
#endif

	/* If needed allocate memory for the alternate motor position arrays. */

	if ( scan->datafile.num_x_motors > 0 ) {
		dimension[0] = scan->datafile.num_x_motors;
		dimension[1] = quick_scan->actual_num_measurements;

		element_size[0] = sizeof(double);
		element_size[1] = sizeof(double *);

		scan->datafile.x_position_array = (double **)
			mx_allocate_array( MXFT_DOUBLE,
				2, dimension, element_size );

		if ( scan->datafile.x_position_array == (double **) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld by %ld array of "
		"alternate position values for scan '%s'.",
				dimension[0], dimension[1], scan->record->name);
		}
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
		"allocate memory for the alternate datafile position arrays." );

	MX_HRT_START( timing_measurement );
#endif

	if ( scan->plot.num_x_motors > 0 ) {
		dimension[0] = scan->plot.num_x_motors;
		dimension[1] = quick_scan->actual_num_measurements;

		element_size[0] = sizeof(double);
		element_size[1] = sizeof(double *);

		scan->plot.x_position_array = (double **)
			mx_allocate_array( MXFT_DOUBLE,
				2, dimension, element_size );

		if ( scan->plot.x_position_array == (double **) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld by %ld array of "
		"alternate position values for scan '%s'.",
				dimension[0], dimension[1], scan->record->name);
		}
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
		"allocate memory for the alternate plot position arrays." );

	MX_HRT_START( timing_measurement );
#endif

	/* Is this scan supposed to be a synchronous motion mode scan? */

	synchronous_motion_mode_record = mx_get_record( scan->record,
					MX_SCAN_SYNCHRONOUS_MOTION_MODE );

	if ( synchronous_motion_mode_record == (MX_RECORD *) NULL ) {
		quick_scan->use_synchronous_motion_mode = FALSE;
	} else {
		mx_status = mx_get_bool_variable( synchronous_motion_mode_record,
				&(quick_scan->use_synchronous_motion_mode));
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"check for synchronous motion mode." );

	MX_HRT_START( timing_measurement );
#endif
	/**** If requested, set MCE position windows, ****/

	for ( i = 0; i < scan->num_motors; i++ ) {
		MX_RECORD *mce_record;
		mx_bool_type window_is_available, use_window;
		double window[ MXU_MTR_NUM_WINDOW_PARAMETERS ];
		long window_bytes;

		mce_record = mcs_quick_scan->mce_record_array[i];

		mx_status = mx_mce_get_window_is_available( mce_record,
							&window_is_available );

		if ( ( mx_status.code != MXE_SUCCESS )
		  || ( window_is_available == FALSE ) )
		{
			mcs_quick_scan->use_window[i] = FALSE;
			mcs_quick_scan->window[i][0] = 0.0;
			mcs_quick_scan->window[i][1] = 0.0;

			continue;  /* Go back to the top of the for() loop. */
		} else {
			mx_status = mx_mce_get_use_window( mce_record,
								&use_window );

			if ( ( mx_status.code != MXE_SUCCESS )
			  || ( use_window == FALSE ) )
			{
				mcs_quick_scan->use_window[i] = FALSE;
				mcs_quick_scan->window[i][0] = 0.0;
				mcs_quick_scan->window[i][1] = 0.0;

				/* Go back to the top of the for() loop. */
				continue;
			} else {
				window_bytes = MXU_MTR_NUM_WINDOW_PARAMETERS
							* sizeof(double);

				memset( window, 0, window_bytes );

				window[0] =
					mcs_quick_scan->real_start_position[i];
				window[1] =
					mcs_quick_scan->real_end_position[i];

				mx_status = mx_mce_set_window( mce_record,
					window, MXU_MTR_NUM_WINDOW_PARAMETERS );

				if ( mx_status.code != MXE_SUCCESS ) {
					mcs_quick_scan->use_window[i] = FALSE;
					mcs_quick_scan->window[i][0] = 0.0;
					mcs_quick_scan->window[i][1] = 0.0;

					/* Go back to the top
					 * of the for() loop.
					 */
					continue;
				}
			}
		}
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"check for position windowing." );

	MX_HRT_START( timing_measurement );
#endif

	/**** Initialize the datafile and plotting support. ****/

	mx_status = mx_standard_prepare_for_scan_start( scan );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_scan_restore_speeds( scan );
		FREE_MOTOR_POSITION_ARRAYS;
		return mx_status;
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
	    "mx_standard_prepare_for_scan_start() - datafile and plotting." );

	MX_HRT_START( timing_measurement );
#endif

	if ( mx_plotting_is_enabled( scan->record ) ) {
		mx_status = mx_plot_start_plot_section( &(scan->plot) );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_scan_restore_speeds( scan );
			FREE_MOTOR_POSITION_ARRAYS;
			return mx_status;
		}
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname, "start plot section" );

	MX_HRT_START( timing_measurement );
#endif

	/**** Switch to synchronous motion mode if requested. ****/

	if ( quick_scan->use_synchronous_motion_mode ) {
		for ( i = 0; i < scan->num_motors; i++ ) {
			motor_record = (scan->motor_record_array)[i];

			mx_status = mx_motor_set_synchronous_motion_mode(
						motor_record, TRUE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"set synchronous motion mode." );

	MX_HRT_START( timing_measurement );
#endif

	/**** Display the quick scan parameters. ****/

	for ( i = 0; i < scan->num_motors; i++ ) {
		motor_record = (scan->motor_record_array)[i];

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		mx_info("Motor '%s' will scan from %g %s to %g %s.",
			motor_record->name,
			quick_scan->start_position[i],
			motor->units,
			quick_scan->end_position[i],
			motor->units );
	}

	mx_info(" ");

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"display quick scan parameters" );
#endif

	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

static void
mxs_mcs_quick_scan_check_for_motor_errors( MX_SCAN *scan )
{
	MX_RECORD *motor_record;
	unsigned long i, motor_status;
	mx_status_type mx_status;

	for ( i = 0; i < scan->num_motors; i++ ) {
	    motor_record = scan->motor_record_array[i];

	    mx_status = mx_motor_get_status( motor_record,
						&motor_status );

	    if ( mx_status.code == MXE_SUCCESS ) {
		if ( motor_status & MXSF_MTR_ERROR ) {
		    mx_warning(
			"An error occurred for motor '%s' during scan '%s'.\n"
			"--> MX motor status = %#lx",
			motor_record->name, scan->record->name, motor_status );
		}
		if ( motor_status & MXSF_MTR_POSITIVE_LIMIT_HIT ) {
		    mx_warning( "Motor '%s' positive limit hit.",
			motor_record->name );
		}
		if ( motor_status & MXSF_MTR_NEGATIVE_LIMIT_HIT ) {
		    mx_warning( "Motor '%s' negative limit hit.",
			motor_record->name );
		}
		if ( motor_status & MXSF_MTR_FOLLOWING_ERROR ) {
		    mx_warning( "Motor '%s' following error.",
			motor_record->name );
		}
		if ( motor_status & MXSF_MTR_DRIVE_FAULT ) {
		    mx_warning( "Motor '%s' drive fault.",
			motor_record->name );
		}
		if ( motor_status & MXSF_MTR_AXIS_DISABLED ) {
		    mx_warning( "Motor '%s' axis disabled.",
			motor_record->name );
		}
		if ( motor_status & MXSF_MTR_OPEN_LOOP ) {
		    mx_warning( "Motor '%s' open loop.",
			motor_record->name );
		}
	    }
	}
	return;
}

#if DEBUG_SPEED

static mx_status_type
mxs_mcs_quick_scan_display_scan_parameters( MX_SCAN *scan,
					MX_QUICK_SCAN *quick_scan,
					MX_MCS_QUICK_SCAN *mcs_quick_scan )
{
	static const char fname[] =
		"mxs_mcs_quick_scan_display_scan_parameters()";

	MX_RECORD *motor_record;
	double position, speed, acceleration_time, acceleration_distance;
	double raw_acceleration_parameters[MX_MOTOR_NUM_ACCELERATION_PARAMS];
	mx_status_type mx_status;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed was NULL." );
	}
	if ( (scan->num_motors == 0)
	  || ( scan->motor_record_array == NULL ) )
	{
		mx_warning( "No motors are specified for scan '%s'.",
			scan->record->name );
		return MX_SUCCESSFUL_RESULT;
	}

	MX_DEBUG(-2,("%s: --------------------------------------------",fname));

	MX_DEBUG(-2,("%s: start_position = %g, end_position = %g",
		fname, quick_scan->start_position[0],
			quick_scan->end_position[0] ));

	MX_DEBUG(-2,("%s: requested # = %ld, actual # = %ld",
		fname, quick_scan->requested_num_measurements,
		quick_scan->actual_num_measurements));

	MX_DEBUG(-2,("%s: estimated_quick_scan_duration = %g",
		fname, quick_scan->estimated_quick_scan_duration));

	MX_DEBUG(-2,("%s: old_motor_speed = %g",
		fname, quick_scan->old_motor_speed[0]));

	MX_DEBUG(-2,("%s: motor_position_array[0] = %g",
		fname, *(mcs_quick_scan->motor_position_array[0]) ));

	MX_DEBUG(-2,("%s: premove_measurement_time = %g",
		fname, mcs_quick_scan->premove_measurement_time));

	MX_DEBUG(-2,("%s: acceleration_time = %g",
		fname, mcs_quick_scan->acceleration_time));

	MX_DEBUG(-2,("%s: scan_body_time = %g",
		fname, mcs_quick_scan->scan_body_time));

	MX_DEBUG(-2,("%s: deceleration_time = %g",
		fname, mcs_quick_scan->deceleration_time));

	MX_DEBUG(-2,("%s: postmove_measurement_time = %g",
		fname, mcs_quick_scan->postmove_measurement_time));

	MX_DEBUG(-2,("%s: real_start_position[0] = %g",
		fname, mcs_quick_scan->real_start_position[0]));

	MX_DEBUG(-2,("%s: real_end_position[0] = %g",
		fname, mcs_quick_scan->real_end_position[0]));

	MX_DEBUG(-2,("%s: backlash_position[0] = %g",
		fname, mcs_quick_scan->backlash_position[0]));

	MX_DEBUG(-2,("%s: --------------------------------------------",fname));

	motor_record = scan->motor_record_array[0];

	if ( motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The first motor record pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	/* If we are scanning energy, replace the energy motor record with
	 * the theta motor record.
	 */

	if ( strcmp(motor_record->name, "energy") == 0 ) {
		motor_record = mx_get_record( motor_record, "theta" );

		if ( motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_NOT_FOUND, fname,
			"The 'theta' motor was not found for energy scan '%s'.",
				scan->record->name );
		}
	}

	MX_DEBUG(-2,("%s: reporting parameters for motor '%s'",
		fname, motor_record->name));

	mx_status = mx_motor_get_position( motor_record, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: motor '%s' position = %g",
		fname, motor_record->name, position ));

	mx_status = mx_motor_get_speed( motor_record, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: motor '%s' speed = %g",
		fname, motor_record->name, speed ));

	mx_status = mx_motor_get_acceleration_time( motor_record,
						&acceleration_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: acceleration_time = %g",
		fname, acceleration_time));

	mx_status = mx_motor_get_acceleration_distance( motor_record,
						&acceleration_distance );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: acceleration_distance = %g",
		fname, acceleration_distance));

	mx_status = mx_motor_get_raw_acceleration_parameters( motor_record,
						raw_acceleration_parameters );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mx_get_debug_level() <= -2 ) {
		int i;

		fprintf(stderr,"%s: raw_acceleration_parameters =", fname);

		for ( i = 0; i < 3; i++ ) {
			fprintf(stderr, "  %g",
				raw_acceleration_parameters[i]);
		}

		fprintf(stderr,"\n");
	}

	MX_DEBUG(-2,("%s: --------------------------------------------",fname));

	return MX_SUCCESSFUL_RESULT;
}

#endif

/*--------------------------------------------------------------------------*/

static mx_status_type
mxs_mcs_quick_scan_readout_measurement( MX_SCAN * scan,
					MX_QUICK_SCAN *quick_scan,
					MX_MCS_QUICK_SCAN *mcs_quick_scan,
					long *data_values,
					double *motor_datafile_positions,
					double *motor_plot_positions,
					long *old_measurement_number )
{
	MX_RECORD *mcs_record = NULL;
	MX_RECORD *mce_record = NULL;
	MX_RECORD *input_device_record = NULL;
	MX_SCALER *scaler = NULL;
	MX_MCS_SCALER *mcs_scaler = NULL;
	long i, n, scaler_index;
	long mcs_measurement_number, scan_measurement_number;
	long first_new_measurement;
	long num_datafile_motors, num_plot_motors;
	unsigned long mask;
	double encoder_value, measurement_time;
	char output_buffer[250], value_buffer[30];
	mx_status_type mx_status;

	scan_measurement_number = LONG_MAX;

	for ( n = 0; n < mcs_quick_scan->num_mcs; n++ ) {
		mcs_record = mcs_quick_scan->mcs_record_array[n];

		mx_status = mx_mcs_get_measurement_number( mcs_record,
						&mcs_measurement_number );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( mcs_measurement_number < scan_measurement_number ) {
			scan_measurement_number = mcs_measurement_number;
		}
	}

	/* If a new measurement has not shown up, then we have nothing
	 * to do and should return.
	 */

	if ( scan_measurement_number <= *old_measurement_number ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, then we have measurements to read out. */

	/* FIXME: At the moment, we are not yet copying the necessary values to
	 * scan->datafile.x.position_array or to scan->plot.x_position_array,
	 * so alternate datafile and plot motors will not yet work correctly
	 * and will either print garbage or all zeros.
	 */

	measurement_time = mx_quick_scan_get_measurement_time( quick_scan );

	if ( scan->datafile.num_x_motors > 0 ) {
		num_datafile_motors = scan->datafile.num_x_motors;
	} else {
		num_datafile_motors = scan->num_motors;
	}

	if ( scan->plot.num_x_motors > 0 ) {
		num_plot_motors = scan->plot.num_x_motors;
	} else {
		num_plot_motors = scan->num_motors;
	}

	/*---*/

	first_new_measurement = *old_measurement_number + 1;

	for ( i = first_new_measurement; i <= scan_measurement_number; i++ ) {

#if DEBUG_READ_MEASUREMENT
		fprintf( stderr, "Scan '%s': Reading out measurement %ld: ",
			scan->record->name, i );
#endif

		/* Readout the motor positions.  Since some of the motors may
		 * not have multichannel encoders attached, it is important
		 * to get this information quickly.
		 */

		for ( n = 0; n < scan->num_motors; n++ ) {

			mce_record = mcs_quick_scan->mce_record_array[n];

			if ( mce_record == (MX_RECORD *) NULL ) {
				/* FIXME: At the moment, if the motor is
				 * not attached to an MCE, then we just
				 * skip that motor.  Ultimately, we may
				 * want to just readout that motor's
				 * position _now_.
				 */

				continue;	/* Cycle the for(n) loop. */
			}

			/* Get the recorded MCE position. */

			mx_status = mx_mce_read_measurement( mce_record,
							i, &encoder_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mcs_quick_scan->motor_position_array[n][i]
						= encoder_value;

#if DEBUG_READ_MEASUREMENT
			fprintf( stderr, "Encoder[%ld] = %g, ",
				n, encoder_value );
#endif
		}

		/* Then, we read out the MCS channel values from the array
		 * stored by the callback routine.
		 */

		for ( n = 0; n < mcs_quick_scan->num_mcs; n++ ) {
			mcs_record = mcs_quick_scan->mcs_record_array[n];

#if DEBUG_READ_MEASUREMENT
			fprintf( stderr, "MCS '%s' ", mcs_record->name );
#endif

			mx_status = mx_mcs_read_measurement( mcs_record,
							i, NULL, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

#if DEBUG_READ_MEASUREMENT
		fprintf( stderr, "\n" );
#endif
		/* Add the measurement to the datafile and the plot. */

		for ( n = 0; n < num_datafile_motors; n++ ) {
			if ( scan->datafile.num_x_motors > 0 ) {
				motor_datafile_positions[n] =
				    scan->datafile.x_position_array[n][i];
			} else {
				motor_datafile_positions[n] =
				    mcs_quick_scan->motor_position_array[n][i];
			}
		}

		for ( n = 0; n < num_plot_motors; n++ ) {
			if ( scan->plot.num_x_motors > 0 ) {
				motor_plot_positions[n] =
				    scan->plot.x_position_array[n][i];
			} else {
				motor_plot_positions[n] =
				    mcs_quick_scan->motor_position_array[n][i];
			}
		}

		for ( n = 0; n < scan->num_input_devices; n++ ) {

			input_device_record = scan->input_device_array[n];

			scaler = (MX_SCALER *)
				input_device_record->record_class_struct;

			mcs_scaler = (MX_MCS_SCALER *)
				input_device_record->record_type_struct;

			mcs_record = mcs_scaler->mcs_record;

			scaler_index = mcs_scaler->scaler_number;

			mx_status = mx_mcs_read_scaler_measurement( mcs_record,
					scaler_index, i, &(data_values[n]) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Subtract a dark current value if necessary. */

			mask = MXF_SCL_SUBTRACT_DARK_CURRENT
				| MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT;

			if ( scaler->scaler_flags & mask ) {
				data_values[n] -= mx_round(scaler->dark_current
							* measurement_time);
			}
		}

		/* Show the new motor positions and measurement to the user. */

		strlcpy( output_buffer, "", sizeof(output_buffer) );

		for ( n = 0; n < num_plot_motors; n++ ) {

			snprintf( value_buffer, sizeof(value_buffer),
				" %-8g", motor_plot_positions[n] );

			strlcat( output_buffer, value_buffer,
				sizeof(output_buffer) );
		}

		for ( n = 0; n < scan->num_input_devices; n++ ) {

			snprintf( value_buffer, sizeof(value_buffer),
				" %ld", data_values[n] );

			strlcat( output_buffer, value_buffer,
				sizeof(output_buffer) );
		}

		mx_info( "%s", output_buffer );

		/* Add the measurement to the data file. */

		mx_status = mx_add_array_to_datafile( &(scan->datafile),
			MXFT_DOUBLE, num_datafile_motors,
						motor_datafile_positions,
			MXFT_LONG, scan->num_input_devices, data_values );

		if ( mx_status.code != MXE_SUCCESS ) {

			/* If we cannot write the data to the datafile,
			 * then all is lost, so we abort.
			 */

			return mx_status;
		}

		/* Add the measurement to the plot. */

		if ( mx_plotting_is_enabled( scan->record ) ) {

			/* Failing to update the plot correctly is not
			 * a reason to abort, since that would interrupt
			 * the writing of the datafile.
			 */

			(void) mx_add_array_to_plot_buffer( &(scan->plot),
			    MXFT_DOUBLE, num_plot_motors, motor_plot_positions,
			    MXFT_LONG, scan->num_input_devices, data_values );
		}
	}

	/* Save the updated MCS measurement number for our next call. */

	*old_measurement_number = scan_measurement_number;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

/* WARNING!!!
 * Do not use FREE_MOTOR_POSITION_ARRAYS in the execute_scan_body() function
 * since the cleanup_after_scan_end() function will attempt to use the arrays
 * regardless of what error code that execute_scan_body() returns.
 */

#define MX_WAIT_FOR_MCS_TO_START	TRUE

MX_EXPORT mx_status_type
mxs_mcs_quick_scan_execute_scan_body( MX_SCAN *scan )
{
	static const char fname[] = "mxs_mcs_quick_scan_execute_scan_body()";

	MX_RECORD *clock_record;
	MX_MCS_TIMER *mcs_timer;
	MX_QUICK_SCAN *quick_scan;
	MX_MCS_QUICK_SCAN *mcs_quick_scan;
	MX_MEASUREMENT_PRESET_TIME *preset_time_struct;
	MX_MEASUREMENT_PRESET_PULSE_PERIOD *preset_pulse_period_struct;
	mx_bool_type busy;
	long i, n;
	unsigned long measurement_milliseconds;
	mx_bool_type readout_by_measurement;
	long old_measurement_number;

	long *data_values = NULL;
	double *motor_datafile_positions = NULL;
	double *motor_plot_positions = NULL;

#if MX_WAIT_FOR_MCS_TO_START
	MX_RECORD *mcs_record;
	unsigned long wait_ms, max_attempts;
	long j;
	int all_busy;
#endif
	mx_status_type mx_status;

#if DEBUG_TIMING
	MX_HRT_TIMING timing_measurement;
#endif

	MX_DEBUG( 2,("%s invoked.", fname));

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mcs_quick_scan->mcs_readout_preference
		== MXF_MCS_PREFER_READ_MEASUREMENT )
	{
		readout_by_measurement = TRUE;
	} else {
		readout_by_measurement = FALSE;
	}

#if DEBUG_SCAN_PROGRESS
	MX_DEBUG(-2,("%s: readout_by_measurement = %d",
		fname, (int) readout_by_measurement));
#endif

	if ( readout_by_measurement ) {
		/* If we are reading things out of the MCSs as they come in,
		 * we need some arrays to hold temporary values in.
		 */

		data_values = (long *)
			malloc( scan->num_input_devices * sizeof(long) );

		if ( data_values == (long *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
				"Cannot allocate a %ld element array "
				"of scaler values.",
					scan->num_input_devices );
		}

		motor_datafile_positions = (double *) malloc(scan->num_motors);

		if ( motor_datafile_positions == (double *) NULL ) {
			mx_free( data_values );
			return mx_error( MXE_OUT_OF_MEMORY, fname,
				"Cannot allocate a %ld element array "
				"of motor datafile positions.",
					scan->num_motors );
		}

		motor_plot_positions = (double *) malloc(scan->num_motors);

		if ( motor_plot_positions == (double *) NULL ) {
			mx_free( data_values );
			mx_free( motor_datafile_positions );
			return mx_error( MXE_OUT_OF_MEMORY, fname,
				"Cannot allocate a %ld element array "
				"of motor plot positions.",
					scan->num_motors );
		}
	}

#if DEBUG_SPEED
	mx_status = mxs_mcs_quick_scan_display_scan_parameters( scan,
						quick_scan, mcs_quick_scan );
#endif
	/* If we will be reading out the MCS values by measurement in
	 * this routine, then we need to update the dark current values
	 * before we start the hardware running.
	 */

	if ( readout_by_measurement ) {
		for ( n = 0; n < mcs_quick_scan->num_mcs; n++ ) {
			mx_status = mx_mcs_get_dark_current_array(
					mcs_quick_scan->mcs_record_array[n],
					-1, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_free( data_values );
				mx_free( motor_datafile_positions );
				mx_free( motor_plot_positions );
				return mx_status;
			}
		}
	}

	/* Reset any faults that may have occurred. */

	mx_status = mx_scan_reset_all_faults( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for permission to start the scan. */

	mx_status = mx_scan_wait_for_all_permits( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if DEBUG_TIMING
	MX_HRT_START( timing_measurement );
#endif

	/* Start the multichannel scalers. */

	if ( mcs_quick_scan->num_mcs == 1 ) {
		mx_info("Starting the multichannel scaler.");
	} else {
		mx_info("Starting the multichannel scalers.");
	}

	switch( scan->measurement.type ) {
	case MXM_PRESET_TIME:
		preset_time_struct = (MX_MEASUREMENT_PRESET_TIME *)
				scan->measurement.measurement_type_struct;

		clock_record = preset_time_struct->timer_record;

		mcs_timer = (MX_MCS_TIMER *) clock_record->record_type_struct;

		mx_status = mx_mcs_start( mcs_timer->mcs_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_free( data_values );
			mx_free( motor_datafile_positions );
			mx_free( motor_plot_positions );
			return mx_status;
		}
		break;
	case MXM_PRESET_PULSE_PERIOD:
		preset_pulse_period_struct =
				(MX_MEASUREMENT_PRESET_PULSE_PERIOD *)
				scan->measurement.measurement_type_struct;

		clock_record = 
			preset_pulse_period_struct->pulse_generator_record;

		mx_status = mx_pulse_generator_start( clock_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_free( data_values );
			mx_free( motor_datafile_positions );
			mx_free( motor_plot_positions );
			return mx_status;
		}
		break;
	case MXM_PRESET_COUNT:
		mx_free( data_values );
		mx_free( motor_datafile_positions );
		mx_free( motor_plot_positions );

		return mx_error( MXE_UNSUPPORTED, fname,
		"Preset count measurements are not currently supported "
		"for MCS quick scan '%s'.",
			scan->record->name );
	default:
		mx_free( data_values );
		mx_free( motor_datafile_positions );
		mx_free( motor_plot_positions );

		return mx_error( MXE_UNSUPPORTED, fname,
	"Measurement type '%ld' for MCS quick scan '%s' is unsupported.",
			scan->measurement.type, scan->record->name );
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"starting the multichannel scalers" );

	MX_HRT_START( timing_measurement );
#endif

	/* Wait several measurement times before starting the motors.
	 *
	 * This allows each MCS to record several points at the start
	 * position.  Since the absolute position of the motor at the
	 * start position is known from information provided by the
	 * motor controller, it is possible to calculate the motor
	 * position for each point in the scan by using the incremental
	 * encoder counts as an offset from the know absolute starting
	 * position.
	 */

	measurement_milliseconds = mx_round( 1000.0 *
			mx_quick_scan_get_measurement_time(quick_scan) );

	if ( measurement_milliseconds < 1L ) {
		measurement_milliseconds = 1L;
	}

	mx_msleep( measurement_milliseconds
				* MXS_SQ_MCS_NUM_PREMOVE_MEASUREMENTS );

#if MX_WAIT_FOR_MCS_TO_START

	wait_ms = 10;
	max_attempts = 100;

	for ( i = 0; i < max_attempts; i++ ) {
		all_busy = TRUE;

		for ( j = 0; j < mcs_quick_scan->num_mcs; j++ ) {
			mcs_record = mcs_quick_scan->mcs_record_array[j];

			mx_status = mx_mcs_is_busy( mcs_record, &busy );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_free( data_values );
				mx_free( motor_datafile_positions );
				mx_free( motor_plot_positions );
				return mx_status;
			}

			if ( busy == FALSE ) {
				all_busy = FALSE;

				break;		/* Exit the for(j) loop. */
			}
		}

		if ( all_busy ) {
			break;			/* Exit the for(i) loop. */
		}

		mx_msleep(wait_ms);
	}

#if DEBUG_WAIT_FOR_MCS
	MX_DEBUG(-2,("%s: waiting for mcs, i = %ld", fname, i));
#endif

	if ( i >= max_attempts ) {
		mx_free( data_values );
		mx_free( motor_datafile_positions );
		mx_free( motor_plot_positions );

		return mx_error( MXE_TIMED_OUT, fname,
		"Timed out after waiting %g seconds for MCS '%s' used by "
		"quick scan '%s' to start counting.",
			0.001 * (double) ( wait_ms * max_attempts ),
			mcs_quick_scan->mcs_record_array[j]->name,
			scan->record->name );
	}

#endif	/* MX_WAIT_FOR_MCS_TO_START */

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"waiting for the MCS to start." );

	MX_HRT_START( timing_measurement );
#endif

	/* Start the motors.
	 *
	 * We have already done backlash correction of the motors used by
	 * the quick scan by the time we get here, so this move command
	 * ignores backlash corrections.
	 *
	 * Note: We do not use mxs_mcs_quick_scan_move_absolute_and_wait()
	 * here, since the pause/retry logic in that function is not
	 * appropriate for the main move of a quick scan.  After all,
	 * how can you "retry" a quick scan without restarting it from
	 * the beginning?
	 */

	mx_info("Starting the motors.");

	mx_status = mx_motor_array_move_absolute(
			scan->num_motors,
			scan->motor_record_array,
			mcs_quick_scan->real_end_position,
			MXF_MTR_NOWAIT | MXF_MTR_IGNORE_BACKLASH );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free( data_values );
		mx_free( motor_datafile_positions );
		mx_free( motor_plot_positions );
		return mx_status;
	}

	mx_info("Quick scan is in progress...");

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname, "starting the motors" );

	MX_HRT_START( timing_measurement );
#endif

	old_measurement_number = -1;

	/* Wait for the counting to finish. */

	do {
		if ( mx_user_requested_interrupt() ) {
			/* Stop the motors. */

			for ( i = 0; i < scan->num_motors; i++ ) {
				(void) mx_motor_soft_abort(
					scan->motor_record_array[i] );
			}

			/* Stop all the MCSs. */

			for ( i = 0; i < mcs_quick_scan->num_mcs; i++ ) {
				(void) mx_mcs_stop(
					mcs_quick_scan->mcs_record_array[i] );
			}

			mx_free( data_values );
			mx_free( motor_datafile_positions );
			mx_free( motor_plot_positions );

			return mx_error( MXE_INTERRUPTED, fname,
			"Quick scan was interrupted." );
		}

		if ( readout_by_measurement ) {
			mx_status = mxs_mcs_quick_scan_readout_measurement(
					scan, quick_scan, mcs_quick_scan,
					data_values,
					motor_datafile_positions,
					motor_plot_positions,
					&old_measurement_number );

			if ( mx_status.code != MXE_SUCCESS ) {
				if ( scan->num_measurement_fault_handlers > 0 )
				{
					return mx_error( MXE_TRY_AGAIN, fname,
					"An error occurred while running "
					"this quick scan.  We will retry "
					"the scan after saving the data "
					"from it." );
				} else {
					mx_free( data_values );
					mx_free( motor_datafile_positions );
					mx_free( motor_plot_positions );
					return mx_status;
				}
			}
		}

		if ( scan->measurement.type == MXM_PRESET_TIME ) {
			mx_status = mx_timer_is_busy( clock_record, &busy );
		} else {
			mx_status = mx_pulse_generator_is_busy(
							clock_record, &busy );
		}

		if ( mx_status.code != MXE_SUCCESS ) {
			if ( scan->num_measurement_fault_handlers > 0 ) {
				return mx_error( MXE_TRY_AGAIN, fname,
					"An error occurred while running "
					"this quick scan.  We will retry "
					"the scan after saving the data "
					"from it." );
			} else {
				mx_free( data_values );
				mx_free( motor_datafile_positions );
				mx_free( motor_plot_positions );
				return mx_status;
			}
		}

		mx_msleep(100);

	} while ( busy );

	mx_info("Quick scan complete.");

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"waiting for the scan to complete." );

	MX_HRT_START( timing_measurement );
#endif
	mx_free( data_values );
	mx_free( motor_datafile_positions );
	mx_free( motor_plot_positions );

	/* See if any of the motors used by the scan generated any errors. */

	(void) mxs_mcs_quick_scan_check_for_motor_errors( scan );

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname, "check for motor errors" );

	MX_HRT_START( timing_measurement );
#endif

	/* Stop the motors in case they are still running. */

	for ( i = 0; i < scan->num_motors; i++ ) {
		(void) mx_motor_soft_abort( scan->motor_record_array[i] );
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname, "stop the motors" );

	MX_HRT_START( timing_measurement );
#endif

	/* Stop the multichannel scalers in case they are still counting. */

	for ( i = 0; i < mcs_quick_scan->num_mcs; i++ ) {
		(void) mx_mcs_stop( mcs_quick_scan->mcs_record_array[i] );
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"stop the multichannel scalers" );

	MX_HRT_START( timing_measurement );
#endif

	/* Make sure the motors have stopped so that it is safe to send
	 * them new commands.
	 */

	(void) mx_wait_for_motor_array_stop( scan->num_motors,
				scan->motor_record_array, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"wait for the motors to stop" );
#endif

	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

/* Please note that most error conditions are ignored in the
 * cleanup_after_scan_end() function since it is important that
 * the data be saved to disk in spite of any extraneous errors
 * that may occur.
 */

static mx_status_type
mxs_mcs_quick_scan_cl_read_scaler( MX_SCAN *scan )
{
	static const char fname[] =
		"mxs_mcs_quick_scan_cl_read_scaler()";

	MX_RECORD *input_device_record;
	MX_MCS *mcs;
	MX_SCALER *scaler;
	MX_MCS_SCALER *mcs_scaler;
	MX_QUICK_SCAN *quick_scan;
	MX_MCS_QUICK_SCAN *mcs_quick_scan;
	long **data_array;
	double *motor_datafile_positions;
	double *motor_plot_positions;
	double measurement_time;
	long *data_values;
	long i, j, scaler_index;
	long num_datafile_motors, num_plot_motors;
	unsigned long mask;
	char output_buffer[250], value_buffer[30];
	mx_status_type mx_status;

	mx_status_type (*compute_motor_positions_fn)( MX_SCAN *,
						MX_QUICK_SCAN *,
						MX_MCS_QUICK_SCAN *,
						MX_MCS * );

#if DEBUG_TIMING
	MX_HRT_TIMING timing_measurement;
#endif

	MX_DEBUG( 2,("%s invoked.", fname));

	/* Try to restore the old motor speeds and synchronous motion mode
	 * flags for this quick scan, no matter what happens.
	 */

#if DEBUG_TIMING
	MX_HRT_START( timing_measurement );
#endif

	(void) mx_scan_restore_speeds( scan );

	/* Get the other important pointers used by this function. */

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS ) {

		/* Can't go any farther if this fails, so we abort. */

		FREE_MOTOR_POSITION_ARRAYS;
		return mx_status;
	}

	MX_DEBUG( 2,("%s: motor speeds restored.", fname));

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname, "restoring motor speeds" );

	MX_HRT_START( timing_measurement );
#endif

	/* Calculate the positions of the motors depending on either
	 * incremental encoder counts sent as UP counts and DOWN counts
	 * to a pair of scaler channels or by using dead reckoning.
	 */

	compute_motor_positions_fn = mcs_quick_scan->compute_motor_positions_fn;

	if ( compute_motor_positions_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The compute_motor_positions_fn for scan '%s' is NULL.",
			scan->record->name );
	}

	mx_status = ( *compute_motor_positions_fn )(
			scan, quick_scan, mcs_quick_scan, NULL );

	if ( mx_status.code != MXE_SUCCESS ) {
		FREE_MOTOR_POSITION_ARRAYS;
		return mx_status;
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"computing motor positions" );

	MX_HRT_START( timing_measurement );
#endif

	/* Read the scaler values for the channels used by this scan. */

	if ( mcs_quick_scan->num_mcs == 1 ) {
		mx_info("Reading out the multichannel scaler.");
	} else {
		mx_info("Reading out the multichannel scalers.");
	}

	/* This version reads only the channels used by the scan. */

	for ( i = 0; i < scan->num_input_devices; i++ ) {

#if DEBUG_TIMING
		MX_HRT_START( timing_measurement );
#endif

		input_device_record = (scan->input_device_array)[i];

		scaler = (MX_SCALER *)input_device_record->record_class_struct;

		mcs_scaler = (MX_MCS_SCALER *)
				input_device_record->record_type_struct;

		mcs = (MX_MCS *) mcs_scaler->mcs_record->record_class_struct;

		scaler_index = mcs_scaler->scaler_number;

		mx_status = mx_mcs_read_scaler( mcs_scaler->mcs_record,
						scaler_index, NULL, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG( 2,("%s: scaler[%ld] '%s' values are:",
						fname, scaler_index,
						input_device_record->name ));

		if ( mx_get_debug_level() >= 2 ) {
			for ( j = 0; j < mcs->current_num_measurements; j++ ) {
				fprintf(stderr,"%ld ",
					(mcs->data_array)[ scaler_index ][j]);
			}
			fprintf(stderr,"\n");
		}

#if DEBUG_TIMING
		MX_HRT_END( timing_measurement );
		MX_HRT_RESULTS( timing_measurement, fname,
			"reading scaler %ld", i );

		MX_HRT_START( timing_measurement );
#endif

		/* Update the dark current value. */

		mx_status = mx_mcs_get_dark_current( mcs_scaler->mcs_record,
						scaler_index, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if DEBUG_TIMING
		MX_HRT_END( timing_measurement );
		MX_HRT_RESULTS( timing_measurement, fname,
			"getting dark current for scaler %ld", i );
#endif
	}

#if DEBUG_TIMING
	MX_HRT_START( timing_measurement );
#endif

	/* Allocate an array to contain the scaler channel values for
	 * one measurement.
	 */

	data_values = (long *)
			malloc( scan->num_input_devices * sizeof(long) );

	if ( data_values == NULL ) {
		FREE_MOTOR_POSITION_ARRAYS;
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate a %ld element array of scaler values.",
			scan->num_input_devices );
	}

	/* Allocate arrays for the motor datafile and plot positions. */

	motor_datafile_positions = (double *) malloc( scan->num_motors );

	if ( motor_datafile_positions == (double *) NULL ) {
		mx_free( data_values );
		FREE_MOTOR_POSITION_ARRAYS;
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate a %ld element of motor datafile positions.",
			scan->num_motors );
	}

	motor_plot_positions = (double *) malloc( scan->num_motors );

	if ( motor_plot_positions == (double *) NULL ) {
		mx_free( data_values );
		mx_free( motor_datafile_positions );
		FREE_MOTOR_POSITION_ARRAYS;
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate a %ld element of motor plot positions.",
			scan->num_motors );
	}

	/*---*/

	measurement_time = mx_quick_scan_get_measurement_time( quick_scan );

	if ( scan->datafile.num_x_motors > 0 ) {
		num_datafile_motors = scan->datafile.num_x_motors;
	} else {
		num_datafile_motors = scan->num_motors;
	}

	if ( scan->plot.num_x_motors > 0 ) {
		num_plot_motors = scan->plot.num_x_motors;
	} else {
		num_plot_motors = scan->num_motors;
	}

#if 0
	if ( use_window ) {
		mx_warning( "MCE position windows are not yet fully "
		"implemented, so we are ignoring the window." );
	}
#endif

	for ( i = 0; i < quick_scan->actual_num_measurements; i++ ) {

		if ( scan->datafile.num_x_motors > 0 ) {
			for ( j = 0; j < num_datafile_motors; j++ ) {
				motor_datafile_positions[j] =
				    scan->datafile.x_position_array[j][i];
			}
		} else {
			for ( j = 0; j < num_datafile_motors; j++ ) {
				motor_datafile_positions[j] = 
				    mcs_quick_scan->motor_position_array[j][i];
			}
		}

#if 0
		for ( j = 0; j < num_datafile_motors; j++ ) {
			MX_DEBUG(0,
			("%s: #%ld, motor_datafile_positions[%ld] = %g",
			 	fname, i, j, motor_datafile_positions[j]));
		}
#endif

		if ( scan->plot.num_x_motors > 0 ) {
			for ( j = 0; j < num_plot_motors; j++ ) {
				motor_plot_positions[j] =
				    scan->plot.x_position_array[j][i];
			}
		} else {
			for ( j = 0; j < num_plot_motors; j++ ) {
				motor_plot_positions[j] = 
				    mcs_quick_scan->motor_position_array[j][i];
			}
		}

#if 0
		for ( j = 0; j < num_plot_motors; j++ ) {
			MX_DEBUG(0,
			("%s: #%ld, motor_plot_positions[%ld] = %g",
			 	fname, i, j, motor_plot_positions[j]));
		}
#endif

		for ( j = 0; j < scan->num_input_devices; j++ ) {

			input_device_record = (scan->input_device_array)[j];

			scaler = (MX_SCALER *)
				input_device_record->record_class_struct;

			mcs_scaler = (MX_MCS_SCALER *)
				input_device_record->record_type_struct;

			mcs = (MX_MCS *)
				mcs_scaler->mcs_record->record_class_struct;

			data_array = mcs->data_array;

			scaler_index = mcs_scaler->scaler_number;

			data_values[j] = data_array[ scaler_index ][i];

			/* Subtract a dark current value if necessary. */

			mask = MXF_SCL_SUBTRACT_DARK_CURRENT
				| MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT;

			if ( scaler->scaler_flags & mask ) {
				data_values[j] -= mx_round(scaler->dark_current
							* measurement_time);
			}
		}

		strlcpy( output_buffer, "", sizeof(output_buffer) );

		for ( j = 0; j < num_plot_motors; j++ ) {

			snprintf( value_buffer, sizeof(value_buffer),
					" %-8g", motor_plot_positions[j] );

			strlcat( output_buffer, value_buffer,
					sizeof(output_buffer) );
		}

		strlcat( output_buffer, " -", sizeof(output_buffer) );

		for ( j = 0; j < scan->num_input_devices; j++ ) {

			snprintf( value_buffer, sizeof(value_buffer),
					" %ld", data_values[j] );

			strlcat( output_buffer, value_buffer,
					sizeof(output_buffer) );
		}

		mx_info( "%s", output_buffer );

		MX_DEBUG( 8,("%s: Copying measurement %ld to data file.",
				fname, i));

		mx_status = mx_add_array_to_datafile( &(scan->datafile),
			MXFT_DOUBLE, num_datafile_motors,
						motor_datafile_positions,
			MXFT_LONG, scan->num_input_devices, data_values );

		if ( mx_status.code != MXE_SUCCESS ) {

			/* If we cannot write the data to the datafile,
			 * all is lost, so we abort.
			 */

			mx_free(data_values);
			mx_free(motor_datafile_positions);
			mx_free(motor_plot_positions);
			FREE_MOTOR_POSITION_ARRAYS;
			return mx_status;
		}

		if ( mx_plotting_is_enabled( scan->record ) ) {
			MX_DEBUG( 8,(
			    "%s: Adding measurement %ld to plot buffer.",
				fname, i));

			/* Failing to update the plot correctly is not
			 * a reason to abort since that would interrupt
			 * the writing of the datafile.
			 */

			(void) mx_add_array_to_plot_buffer( &(scan->plot),
			    MXFT_DOUBLE, num_plot_motors, motor_plot_positions,
			    MXFT_LONG, scan->num_input_devices, data_values );
		}
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"adding measurements to the datafile and plot" );

	MX_HRT_START( timing_measurement );
#endif

	if ( mx_plotting_is_enabled( scan->record ) ) {
		MX_DEBUG( 2,("%s: displaying the plot.", fname));

		/* Attempt to display the plot, but do not abort on an error.*/

		(void) mx_display_plot( &(scan->plot) );
	}

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"displaying the plot" );

	MX_HRT_START( timing_measurement );
#endif

	/* Close the datafile and shut down the plot. */

	MX_DEBUG( 2,("%s: Invoking standard cleanup function.", fname));

	mx_status = mx_standard_cleanup_after_scan_end( scan );

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"mx_standard_cleanup_after_scan_end()" );

	MX_HRT_START( timing_measurement );
#endif

	/* Free all of the data arrays that were allocated in the routine
	 * mxs_mcs_quick_scan_prepare_for_scan_start().
	 */

	MX_DEBUG( 2,("%s: freeing the arrays.", fname));

	mx_free(data_values);
	mx_free(motor_datafile_positions);
	mx_free(motor_plot_positions);
	FREE_MOTOR_POSITION_ARRAYS;

	MX_DEBUG( 2,("%s complete.", fname));

#if DEBUG_TIMING
	MX_HRT_END( timing_measurement );
	MX_HRT_RESULTS( timing_measurement, fname,
			"data arrays freed" );
#endif

	return mx_status;
}

static mx_status_type
mxs_mcs_quick_scan_cl_read_measurement( MX_SCAN *scan )
{
#if DEBUG_SCAN_PROGRESS
	static const char fname[] = "mxs_mcs_quick_scan_cl_read_measurement()";
#endif

#if DEBUG_SCAN_PROGRESS
	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, scan->record->name ));
#endif

	/* Close the datafile and shut down the plot. */

	(void) mx_standard_cleanup_after_scan_end( scan );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_mcs_quick_scan_cleanup_after_scan_end( MX_SCAN *scan )
{
	static const char fname[] =
		"mxs_mcs_quick_scan_cleanup_after_scan_end()";

	MX_QUICK_SCAN *quick_scan = NULL;
	MX_MCS_QUICK_SCAN *mcs_quick_scan = NULL;
	mx_bool_type readout_by_measurement;
	mx_status_type mx_status;

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mcs_quick_scan->mcs_readout_preference
		== MXF_MCS_PREFER_READ_MEASUREMENT )
	{
		readout_by_measurement = TRUE;
	} else {
		readout_by_measurement = FALSE;
	}

#if DEBUG_SCAN_PROGRESS
	MX_DEBUG(-2,("%s: readout_by_measurement = %d",
		fname, (int) readout_by_measurement));
#endif

	if ( readout_by_measurement ) {
		mx_status = mxs_mcs_quick_scan_cl_read_measurement( scan );
	} else {
		mx_status = mxs_mcs_quick_scan_cl_read_scaler( scan );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxs_mcs_quick_scan_get_parameter( MX_SCAN *scan )
{
	static const char fname[] = "mxs_mcs_quick_scan_get_parameter()";

	MX_QUICK_SCAN *quick_scan = NULL;
	MX_MCS_QUICK_SCAN *mcs_quick_scan = NULL;
	mx_status_type mx_status;

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scan->parameter_type ) {
	case MXLV_SCN_ESTIMATED_SCAN_DURATION:
		scan->estimated_scan_duration =
			quick_scan->requested_num_measurements
				* mx_scan_get_measurement_time( scan );
		break;

	default:
		return mx_scan_default_get_parameter_handler( scan );
		break;
	}
	
	return MX_SUCCESSFUL_RESULT;
}

