/*
 * Name:    mx_mca.c
 *
 * Purpose: MX function library for multichannel analyzers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2004-2007, 2010, 2012, 2015-2016
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_MCA_NEW_DATA_AVAILABLE	FALSE

#include <stdio.h>
#include <limits.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_callback.h"
#include "mx_mca.h"

/*=======================================================================*/

/* This function is a utility function to consolidate all of the pointer
 * mangling that often has to happen at the beginning of an mx_mca_...
 * function.  The parameter 'calling_fname' is passed so that error messages
 * will appear with the name of the calling function.
 */

MX_EXPORT mx_status_type
mx_mca_get_pointers( MX_RECORD *mca_record,
			MX_MCA **mca,
			MX_MCA_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_mca_get_pointers()";

	if ( mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The mca_record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( mca_record->mx_class != MXC_MULTICHANNEL_ANALYZER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"The record '%s' passed by '%s' is not an MCA.",
			mca_record->name, calling_fname );
	}

	if ( mca != (MX_MCA **) NULL ) {
		*mca = (MX_MCA *) (mca_record->record_class_struct);

		if ( *mca == (MX_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCA pointer for record '%s' passed by '%s' is NULL",
				mca_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_MCA_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_MCA_FUNCTION_LIST *)
				(mca_record->class_specific_function_list);

		if ( *function_list_ptr == (MX_MCA_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_MCA_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				mca_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mca_initialize_driver( MX_DRIVER *driver,
			long *maximum_num_channels_varargs_cookie,
			long *maximum_num_rois_varargs_cookie,
			long *num_soft_rois_varargs_cookie )
{
	static const char fname[] = "mx_mca_initialize_driver()";

	MX_RECORD_FIELD_DEFAULTS *record_field_defaults_array;
	MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
	mx_status_type mx_status;

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}
	if ( maximum_num_channels_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"The maximum_num_channels_varargs_cookie pointer passed was NULL." );
	}
	if ( maximum_num_rois_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"The maximum_num_rois_varargs_cookie pointer passed was NULL." );
	}
	if ( num_soft_rois_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"The num_soft_rois_varargs_cookie pointer passed was NULL." );
	}

	record_field_defaults_array = *(driver->record_field_defaults_ptr);

	if (record_field_defaults_array == (MX_RECORD_FIELD_DEFAULTS *) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD_FIELD_DEFAULTS array for driver '%s' is NULL.",
			driver->name );
	}

	/*** Construct a varargs cookie for 'maximum_num_channels'. ***/

	mx_status = mx_find_record_field_defaults_index( driver,
						"maximum_num_channels",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
					maximum_num_channels_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*** Construct a varargs cookie for 'maximum_num_rois'. ***/

	mx_status = mx_find_record_field_defaults_index( driver,
						"maximum_num_rois",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
				maximum_num_rois_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*** Construct a varargs cookie for 'num_soft_rois'. ***/

	mx_status = mx_find_record_field_defaults_index( driver,
						"num_soft_rois",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
				num_soft_rois_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*** 'channel_array' depends on 'maximum_num_channels'. ***/

	mx_status = mx_find_record_field_defaults( driver,
						"channel_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *maximum_num_channels_varargs_cookie;

	/*
	 * 'roi_array' and 'roi_integral_array' depend on 'maximum_num_rois'.
	 */

	mx_status = mx_find_record_field_defaults( driver,
						"roi_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *maximum_num_rois_varargs_cookie;

	mx_status = mx_find_record_field_defaults( driver,
						"roi_integral_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *maximum_num_rois_varargs_cookie;

	/*
	 * 'soft_roi_array' and 'soft_roi_integral_array'
	 * depend on 'num_soft_rois'.
	 */

	mx_status = mx_find_record_field_defaults( driver,
						"soft_roi_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *num_soft_rois_varargs_cookie;

	mx_status = mx_find_record_field_defaults( driver,
					"soft_roi_integral_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *num_soft_rois_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mca_finish_record_initialization( MX_RECORD *mca_record )
{
	static const char fname[] = "mx_mca_finish_record_initialization()";

	MX_MCA *mca;
	MX_RECORD_FIELD *channel_array_field;
	long i;
	mx_status_type mx_status;

	if ( mca_record == ( MX_RECORD * ) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}
	mca = (MX_MCA *) mca_record->record_class_struct;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MCA pointer for record '%s' is NULL.",
			mca_record->name );
	}

	/* Initialize some fields in the MX_MCA structure. */

	mca->start = 0;
	mca->stop = 0;
	mca->clear = 0;
	mca->busy = FALSE;
	mca->new_data_available = TRUE;
	mca->mca_flags = 0;

#if DEBUG_MCA_NEW_DATA_AVAILABLE
	MX_DEBUG(-2,("%s: mca '%s' new_data_available = %d",
		fname, mca_record->name, (int) mca->new_data_available));
#endif

	mca->last_measurement_interval = -1.0;

	mca->channel_number = 0;
	mca->channel_value = 0;
	mca->roi_number = 0;

	/* Since 'channel_array' is a varying length field, but is not
	 * specified in the record description, we must fix up the
	 * dimension[0] value for the field by hand.
	 */

	mx_status = mx_find_record_field( mca_record, "channel_array",
						&channel_array_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	channel_array_field->dimension[0] = (long) mca->maximum_num_channels;

	/* Save a direct pointer to the 'new_data_available' field.
	 * This will be used by mx_mca_is_busy() if it decides that
	 * it needs to force a callback for 'new_data_available'.
	 */

	mx_status = mx_find_record_field( mca_record, "new_data_available",
					&(mca->new_data_available_field_ptr) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mca->busy_start_interval_enabled = FALSE;
	mca->busy_start_interval = -1;

	/* Initialize regions of interest. */

	mca->roi[0] = 0;
	mca->roi[1] = 0;

	mca->roi_integral = 0;

	for ( i = 0; i < mca->maximum_num_rois; i++ ) {
		mca->roi_array[i][0] = 0;
		mca->roi_array[i][1] = 0;
		mca->roi_integral_array[i] = 0;
	}

	mca->soft_roi[0] = 0;
	mca->soft_roi[1] = 0;

	mca->soft_roi_integral = 0;

	for ( i = 0; i < mca->num_soft_rois; i++ ) {
		mca->soft_roi_array[i][0] = 0;
		mca->soft_roi_array[i][1] = 0;
		mca->soft_roi_integral_array[i] = 0;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_mca_start( MX_RECORD *mca_record )
{
	static const char fname[] = "mx_mca_start()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *start_fn ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_fn = function_list->start;

	if ( start_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"start function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->busy = TRUE;
	mca->new_data_available = TRUE;

#if DEBUG_MCA_NEW_DATA_AVAILABLE
	MX_DEBUG(-2,("%s: mca '%s' new_data_available = %d",
		fname, mca_record->name, (int) mca->new_data_available));
#endif

	mca->last_start_tick = mx_current_clock_tick();

	mx_status = (*start_fn)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_stop( MX_RECORD *mca_record )
{
	static const char fname[] = "mx_mca_stop()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *stop_fn ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop_fn = function_list->stop;

	if ( stop_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"stop function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->last_start_tick.high_order = 0;
	mca->last_start_tick.low_order  = 0;

	mx_status = (*stop_fn)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_read( MX_RECORD *mca_record,
		unsigned long *num_channels,
		unsigned long **channel_array )
{
	static const char fname[] = "mx_mca_read()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *read_fn ) ( MX_MCA * );
	mx_bool_type read_new_data;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_fn = function_list->read;

	if ( read_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"read function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	/* Decide whether or not to read new data from the MCA or just
	 * report back the data that is already in our local variable
	 * mca->channel_array.
	 */

	if ( mca->mca_flags & MXF_MCA_NO_READ_OPTIMIZATION ) {
		read_new_data = TRUE;

	} else {
#if 1
		mx_status = mx_mca_is_new_data_available( mca->record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
#endif
		if ( mca->new_data_available ) {
			read_new_data = TRUE;
		} else {
			read_new_data = FALSE;
		}
	}

#if DEBUG_MCA_NEW_DATA_AVAILABLE
	MX_DEBUG(-2,
	("%s: (before read) mca->new_data_available = %d, mca->busy = %d",
		fname, (int) mca->new_data_available, (int) mca->busy));

	MX_DEBUG(-2,("%s: mca '%s' read_new_data = %d",
		fname, mca_record->name, read_new_data));
#endif

	if ( read_new_data ) {

		mx_status = (*read_fn)( mca );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( mca->busy == FALSE ) {
			mca->new_data_available = FALSE;
		}
	}

#if DEBUG_MCA_NEW_DATA_AVAILABLE
	MX_DEBUG(-2,
	("%s: (after read) mca->new_data_available = %d, mca->busy = %d",
		fname, (int) mca->new_data_available, (int) mca->busy));
#endif

	if ( num_channels != NULL ) {
		*num_channels = mca->current_num_channels;
	}

	if ( channel_array != NULL ) {
		*channel_array = mca->channel_array;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mca_clear( MX_RECORD *mca_record )
{
	static const char fname[] = "mx_mca_clear()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *clear_fn ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	clear_fn = function_list->clear;

	if ( clear_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"clear function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mx_status = (*clear_fn)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_is_busy( MX_RECORD *mca_record, mx_bool_type *busy )
{
	static const char fname[] = "mx_mca_is_busy()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *busy_fn ) ( MX_MCA * );
	mx_bool_type old_busy;
	MX_RECORD_FIELD *new_data_available_field;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	busy_fn = function_list->busy;

	if ( busy_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"busy function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	old_busy = mca->busy;

	mx_status = (*busy_fn)( mca );

	if ( mca->busy_start_interval_enabled ) {
		mx_bool_type busy_start_set;

		mx_status = mx_mca_check_busy_start_interval( mca_record,
							&busy_start_set );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( busy_start_set ) {
			mca->busy = TRUE;
		}
	}

	if ( busy != NULL ) {
		*busy = mca->busy;
	}

	/* If the MCA has just transitioned from 'busy' to 'not busy' and
	 * there is a callback list installed for the 'new_data_available'
	 * field, then we must set the value of 'new_data_available' to
	 * TRUE, set the field's 'value_has_changed_manual_override' flag
	 * to TRUE, and then force value changed callbacks for all clients
	 * that are monitoring this field.
	 */

	if ( (old_busy == TRUE) && (mca->busy == FALSE) ) {

#if 0
		MX_DEBUG(-2,("%s: MCA '%s' busy just went from TRUE to FALSE.",
			fname, mca_record->name));
#endif

		new_data_available_field = mca->new_data_available_field_ptr;

		if ( new_data_available_field == (MX_RECORD_FIELD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The new_data_available_field_ptr for MCA '%s' is NULL",
				mca_record->name );
		}

		if ( new_data_available_field->callback_list != NULL ) {
		    mca->new_data_available = TRUE;

		    new_data_available_field->value_has_changed_manual_override
			= TRUE;

		    mx_status = mx_local_field_invoke_callback_list(
						new_data_available_field,
						MXCBT_VALUE_CHANGED );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_is_new_data_available( MX_RECORD *mca_record,
				mx_bool_type *new_data_available )
{
	static const char fname[] = "mx_mca_is_new_data_available()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->parameter_type = MXLV_MCA_NEW_DATA_AVAILABLE;

	mx_status = (*get_parameter_fn)( mca );

	if ( new_data_available != NULL ) {
		*new_data_available = mca->new_data_available;
	}

#if DEBUG_MCA_NEW_DATA_AVAILABLE
	MX_DEBUG(-2,("%s: mca '%s' new_data_available = %d",
		fname, mca_record->name, (int) mca->new_data_available));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_start_without_preset( MX_RECORD *mca_record )
{
	static const char fname[] = "mx_mca_start_without_preset()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *start_fn ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_fn = function_list->start;

	if ( start_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"start function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->busy = TRUE;
	mca->new_data_available = TRUE;

#if DEBUG_MCA_NEW_DATA_AVAILABLE
	MX_DEBUG(-2,("%s: mca '%s' new_data_available = %d",
		fname, mca_record->name, (int) mca->new_data_available));
#endif

	mca->last_start_tick = mx_current_clock_tick();

	mca->preset_type = MXF_MCA_PRESET_NONE;

	mx_status = (*start_fn)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_start_with_preset( MX_RECORD *mca_record,
				long preset_type,
				double preset_value )
{
	static const char fname[] = "mx_mca_start_with_preset()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *start_fn ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_fn = function_list->start;

	if ( start_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"start function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->busy = TRUE;
	mca->new_data_available = TRUE;

#if DEBUG_MCA_NEW_DATA_AVAILABLE
	MX_DEBUG(-2,("%s: mca '%s' new_data_available = %d",
		fname, mca_record->name, (int) mca->new_data_available));
#endif

	mca->preset_type = preset_type;
	mca->last_measurement_interval = preset_value;

	switch( preset_type ) {
	case MXF_MCA_PRESET_LIVE_TIME:
		mca->preset_live_time = preset_value;
		break;
	case MXF_MCA_PRESET_REAL_TIME:
		mca->preset_real_time = preset_value;
		break;
	case MXF_MCA_PRESET_COUNT:
		mca->preset_count = mx_round( preset_value );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal preset type %ld for MCA '%s'.",
			preset_type, mca_record->name );
	}

	mca->last_start_tick = mx_current_clock_tick();

	mx_status = (*start_fn)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_start_for_preset_live_time( MX_RECORD *mca_record,
					double preset_seconds )
{
	static const char fname[] = "mx_mca_start_for_preset_live_time()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *start_fn ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_fn = function_list->start;

	if ( start_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"start function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->busy = TRUE;
	mca->new_data_available = TRUE;

#if DEBUG_MCA_NEW_DATA_AVAILABLE
	MX_DEBUG(-2,("%s: mca '%s' new_data_available = %d",
		fname, mca_record->name, (int) mca->new_data_available));
#endif

	mca->preset_type = MXF_MCA_PRESET_LIVE_TIME;
	mca->last_measurement_interval = preset_seconds;

	mca->preset_live_time = preset_seconds;

	mca->last_start_tick = mx_current_clock_tick();

	mx_status = (*start_fn)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_start_for_preset_real_time( MX_RECORD *mca_record,
					double preset_seconds )
{
	static const char fname[] = "mx_mca_start_for_preset_real_time()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *start_fn ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_fn = function_list->start;

	if ( start_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"start function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->busy = TRUE;
	mca->new_data_available = TRUE;

#if DEBUG_MCA_NEW_DATA_AVAILABLE
	MX_DEBUG(-2,("%s: mca '%s' new_data_available = %d",
		fname, mca_record->name, (int) mca->new_data_available));
#endif

	mca->preset_type = MXF_MCA_PRESET_REAL_TIME;
	mca->last_measurement_interval = preset_seconds;

	mca->preset_real_time = preset_seconds;

	mca->last_start_tick = mx_current_clock_tick();

	mx_status = (*start_fn)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_start_for_preset_count( MX_RECORD *mca_record,
					unsigned long preset_count )
{
	static const char fname[] = "mx_mca_start_for_preset_count()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *start_fn ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_fn = function_list->start;

	if ( start_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"start function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->busy = TRUE;
	mca->new_data_available = TRUE;

#if DEBUG_MCA_NEW_DATA_AVAILABLE
	MX_DEBUG(-2,("%s: mca '%s' new_data_available = %d",
		fname, mca_record->name, (int) mca->new_data_available));
#endif

	mca->preset_type = MXF_MCA_PRESET_COUNT;
	mca->last_measurement_interval = (double) preset_count;

	mca->preset_count = preset_count;

	mca->last_start_tick = mx_current_clock_tick();

	mx_status = (*start_fn)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_preset_type( MX_RECORD *mca_record, long *preset_type )
{
	static const char fname[] = "mx_mca_get_preset_type()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name);
	}

	mca->parameter_type = MXLV_MCA_PRESET_TYPE;

	mx_status = (*get_parameter)( mca );

	if ( preset_type != NULL ) {
		*preset_type = mca->preset_type;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_set_preset_type( MX_RECORD *mca_record, long preset_type )
{
	static const char fname[] = "mx_mca_set_preset_type()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_parameter function ptr for record '%s' is NULL.",
			mca_record->name);
	}

	mca->parameter_type = MXLV_MCA_PRESET_TYPE;

	mca->preset_type = preset_type;

	mx_status = (*set_parameter)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_preset_real_time( MX_RECORD *mca_record,
				double *preset_real_time )
{
	static const char fname[] = "mx_mca_set_preset_real_time()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name);
	}

	mca->parameter_type = MXLV_MCA_PRESET_REAL_TIME;

	mx_status = (*get_parameter)( mca );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( preset_real_time != (double *) NULL ) {
		*preset_real_time = mca->preset_real_time;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mca_get_preset_live_time( MX_RECORD *mca_record,
				double *preset_live_time )
{
	static const char fname[] = "mx_mca_set_preset_live_time()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name);
	}

	mca->parameter_type = MXLV_MCA_PRESET_LIVE_TIME;

	mx_status = (*get_parameter)( mca );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( preset_live_time != (double *) NULL ) {
		*preset_live_time = mca->preset_live_time;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mca_get_preset_count( MX_RECORD *mca_record,
				unsigned long *preset_count )
{
	static const char fname[] = "mx_mca_set_preset_count()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name);
	}

	mca->parameter_type = MXLV_MCA_PRESET_COUNT;

	mx_status = (*get_parameter)( mca );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( preset_count != (unsigned long *) NULL ) {
		*preset_count = mca->preset_count;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mca_set_preset_real_time( MX_RECORD *mca_record,
				double preset_real_time )
{
	static const char fname[] = "mx_mca_set_preset_real_time()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_parameter function ptr for record '%s' is NULL.",
			mca_record->name);
	}

	mca->parameter_type = MXLV_MCA_PRESET_REAL_TIME;

	mca->preset_real_time = preset_real_time;

	mx_status = (*set_parameter)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_set_preset_live_time( MX_RECORD *mca_record,
				double preset_live_time )
{
	static const char fname[] = "mx_mca_set_preset_live_time()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_parameter function ptr for record '%s' is NULL.",
			mca_record->name);
	}

	mca->parameter_type = MXLV_MCA_PRESET_LIVE_TIME;

	mca->preset_live_time = preset_live_time;

	mx_status = (*set_parameter)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_set_preset_count( MX_RECORD *mca_record,
				unsigned long preset_count )
{
	static const char fname[] = "mx_mca_set_preset_count()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_parameter function ptr for record '%s' is NULL.",
			mca_record->name);
	}

	mca->parameter_type = MXLV_MCA_PRESET_COUNT;

	mca->preset_count = preset_count;

	mx_status = (*set_parameter)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_preset_count_region( MX_RECORD *mca_record,
			unsigned long *preset_count_region )
{
	static const char fname[] = "mx_mca_get_preset_count_region()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name);
	}

	mca->parameter_type = MXLV_MCA_PRESET_COUNT_REGION;

	mx_status = (*get_parameter)( mca );

	if ( preset_count_region != NULL ) {
		preset_count_region[0] = mca->preset_count_region[0];
		preset_count_region[1] = mca->preset_count_region[1];
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_set_preset_count_region( MX_RECORD *mca_record,
			unsigned long *preset_count_region )
{
	static const char fname[] = "mx_mca_set_preset_count_region()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	if ( preset_count_region == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The preset_count_region pointer passed was NULL." );
	}

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->parameter_type = MXLV_MCA_PRESET_COUNT_REGION;

	mca->preset_count_region[0] = preset_count_region[0];
	mca->preset_count_region[1] = preset_count_region[1];

	mx_status = (*set_parameter)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_set_busy_start_interval( MX_RECORD *mca_record,
				double busy_start_interval_in_seconds )
{
	static const char fname[] = "mx_mca_set_busy_start_interval()";

	MX_MCA *mca;

	if ( mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The motor record pointer passed was NULL." );
	}

	mca = (MX_MCA *) mca_record->record_class_struct;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCA pointer for record '%s' is NULL.",
			mca_record->name );
	}

	mca->busy_start_interval = busy_start_interval_in_seconds;

	if ( busy_start_interval_in_seconds < 0.0 ) {
		mca->busy_start_interval_enabled = FALSE;
	} else {
		mca->busy_start_interval_enabled = TRUE;

		mca->busy_start_ticks =
	    mx_convert_seconds_to_clock_ticks( busy_start_interval_in_seconds );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* If the difference between the current time and the time of the last start
 * command is less than the value of 'busy_start_interval', then the flag
 * busy_start_set will be set to TRUE.
 *
 * There exist MCAs that have the misfeature that when you send them a
 * 'start' command, there may be a short time after the start of the 
 * acquisition where the controller reports that the MCA is _not_ busy.
 *
 * While this may technically be true, it is not useful for higher level logic
 * that is trying to figure out when a acquisition sequence has completed.
 * MX handles this by reporting that the MCA is 'busy' for a period after a
 * start command equal to the value of 'mca->busy_start_interval' in seconds.
 * This is only done if the busy start feature is enabled.
 */

MX_EXPORT mx_status_type
mx_mca_check_busy_start_interval( MX_RECORD *mca_record,
				mx_bool_type *busy_start_set )
{
	static const char fname[] = "mx_mca_check_busy_start_interval()";

	MX_MCA *mca;
	MX_CLOCK_TICK start_tick, busy_start_ticks;
	MX_CLOCK_TICK finish_tick, current_tick;
	int comparison;

	if ( mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The motor record pointer passed was NULL." );
	}
	if ( busy_start_set == (mx_bool_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The busy_start_set pointer passed was NULL." );
	}

	mca = (MX_MCA *) mca_record->record_class_struct;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCA pointer for record '%s' is NULL.",
			mca_record->name );
	}

	if ( mca->busy_start_interval_enabled == FALSE ) {
		*busy_start_set = FALSE;
		return MX_SUCCESSFUL_RESULT;
	}

	mca->last_start_time =
		mx_convert_clock_ticks_to_seconds( mca->last_start_tick );

	start_tick = mca->last_start_tick;

	busy_start_ticks = mca->busy_start_ticks;

	finish_tick = mx_add_clock_ticks( start_tick, busy_start_ticks );

	current_tick = mx_current_clock_tick();

	comparison = mx_compare_clock_ticks( current_tick, finish_tick );

	if ( comparison <= 0 ) {
		*busy_start_set = TRUE;
	} else {
		*busy_start_set = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mca_get_roi( MX_RECORD *mca_record,
			unsigned long roi_number,
			unsigned long *roi )
{
	static const char fname[] = "mx_mca_get_roi()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( roi_number >= mca->current_num_rois ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Requested ROI number %lu is outside the allowed range "
			"of (0-%ld) for MCA '%s'.", roi_number,
			mca->current_num_rois - 1, mca_record->name );
	}

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name);
	}

	mca->roi_number = roi_number;

	mca->parameter_type = MXLV_MCA_ROI;

	mx_status = (*get_parameter)( mca );

	mca->roi_array[ roi_number ][0] = mca->roi[0];
	mca->roi_array[ roi_number ][1] = mca->roi[1];

	if ( roi != NULL ) {
		roi[0] = mca->roi[0];
		roi[1] = mca->roi[1];
	}

	MX_DEBUG( 2,("%s: mca->roi[0] = %lu", fname, mca->roi[0]));
	MX_DEBUG( 2,("%s: mca->roi[1] = %lu", fname, mca->roi[1]));

	MX_DEBUG( 2,("%s: mca->roi_array[%lu][0] = %lu",
	  fname, roi_number, mca->roi_array[ roi_number ][0]));
	MX_DEBUG( 2,("%s: mca->roi_array[%lu][1] = %lu",
	  fname, roi_number, mca->roi_array[ roi_number ][1]));

	if ( mca->roi[0] > mca->roi[1] ) {
		return mx_error( MXE_HARDWARE_CONFIGURATION_ERROR, fname,
		"For MCA '%s', ROI %lu, the ROI lower limit %lu has a larger "
		"index than the ROI upper limit %lu.",
			mca->record->name, roi_number,
			mca->roi[0], mca->roi[1] );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_set_roi( MX_RECORD *mca_record,
			unsigned long roi_number,
			unsigned long *roi )
{
	static const char fname[] = "mx_mca_set_roi()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	if ( roi == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The roi pointer passed was NULL." );
	}

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( roi_number >= mca->current_num_rois ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"Requested ROI number %lu is outside the allowed range "
			"of (0-%ld) for MCA '%s'.", roi_number,
			mca->current_num_rois - 1, mca_record->name );
	}

	if ( roi[1] >= mca->current_num_channels ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"Requested ROI upper limit %lu is outside the allowed "
			"range of (0-%ld) for MCA '%s'.", roi[1],
			mca->current_num_channels - 1, mca_record->name );
	}

	if ( roi[0] > roi[1] ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested ROI lower limit %lu is larger than "
			"the requested ROI upper limit %lu for MCA '%s'.  "
			"This is not allowed.", roi[0], roi[1],
			mca->record->name );
	}

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_roi function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->roi_number = roi_number;

	mca->parameter_type = MXLV_MCA_ROI;

	mca->roi_array[ roi_number ][0] = roi[0];
	mca->roi_array[ roi_number ][1] = roi[1];

	mca->roi[0] = roi[0];
	mca->roi[1] = roi[1];

	MX_DEBUG( 2,("%s: mca->roi[0] = %lu", fname, mca->roi[0]));
	MX_DEBUG( 2,("%s: mca->roi[1] = %lu", fname, mca->roi[1]));

	MX_DEBUG( 2,("%s: mca->roi_array[%lu][0] = %lu",
	  fname, roi_number, mca->roi_array[ roi_number ][0]));
	MX_DEBUG( 2,("%s: mca->roi_array[%lu][1] = %lu",
	  fname, roi_number, mca->roi_array[ roi_number ][1]));

	mx_status = (*set_parameter)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_roi_integral( MX_RECORD *mca_record,
			unsigned long roi_number,
			unsigned long *roi_integral )
{
	static const char fname[] = "mx_mca_get_roi_integral()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( roi_number >= mca->current_num_rois ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Requested ROI number %lu is outside the allowed range "
			"of (0-%ld) for MCA '%s'.", roi_number,
			mca->current_num_rois - 1, mca_record->name );
	}

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->roi_number = roi_number;

	mca->parameter_type = MXLV_MCA_ROI_INTEGRAL;

	mx_status = (*get_parameter)( mca );

	mca->roi_integral_array[ roi_number ] = mca->roi_integral;

	if ( roi_integral != NULL ) {
		*roi_integral = mca->roi_integral;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_roi_array( MX_RECORD *mca_record,
			unsigned long num_rois,
			unsigned long **roi_array )
{
	static const char fname[] = "mx_mca_get_roi_array()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	unsigned long i;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_rois > mca->maximum_num_rois ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Requested number of ROIs %lu is outside the allowed range "
		"of (0-%ld) for MCA '%s'.", num_rois,
			mca->maximum_num_rois, mca_record->name );
	}

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name);
	}

	mca->current_num_rois = (long) num_rois;

	mca->parameter_type = MXLV_MCA_ROI_ARRAY;

	mx_status = (*get_parameter)( mca );

	if ( roi_array != NULL ) {
		for ( i = 0; i < mca->current_num_rois; i++ ) {
			roi_array[i][0] = mca->roi_array[i][0];
			roi_array[i][1] = mca->roi_array[i][1];
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_set_roi_array( MX_RECORD *mca_record,
			unsigned long num_rois,
			unsigned long **roi_array )
{
	static const char fname[] = "mx_mca_set_roi_array()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	unsigned long i;
	mx_status_type ( *set_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	if ( roi_array != NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The roi_array pointer passed was NULL." );
	}

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_rois >= mca->maximum_num_rois ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Requested number of ROIs %lu is outside the allowed range "
		"of (0-%ld) for MCA '%s'.", num_rois,
			mca->maximum_num_rois, mca_record->name );
	}

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_roi function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->current_num_rois = (long) num_rois;

	mca->parameter_type = MXLV_MCA_ROI_ARRAY;

	for ( i = 0; i < num_rois; i++ ) {
		if ( roi_array[i][1] > mca->current_num_channels ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"Requested ROI upper limit %lu for ROI %lu is outside "
			"the allowed range of (0-%ld) for MCA '%s'.",
				roi_array[i][1], i,
				mca->current_num_channels - 1,
				mca->record->name );
		}
		if ( roi_array[i][0] > roi_array[i][1] ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested ROI lower limit %lu for ROI %lu is "
			"larger than the requested ROI upper limit %lu "
			"for MCA '%s'.  This is not allowed.",
			roi_array[i][0], i, roi_array[i][1],
			mca->record->name );
		}

		mca->roi_array[i][0] = roi_array[i][0];
		mca->roi_array[i][1] = roi_array[i][1];
	}

	mx_status = (*set_parameter)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_roi_integral_array( MX_RECORD *mca_record,
			unsigned long num_rois,
			unsigned long *roi_integral_array )
{
	static const char fname[] = "mx_mca_get_roi_integral_array()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	unsigned long i;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_rois > mca->maximum_num_rois ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Requested number of ROIs %lu is outside the allowed range "
		"of (0-%ld) for MCA '%s'.", num_rois,
			mca->maximum_num_rois, mca_record->name );
	}

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->current_num_rois = (long) num_rois;

	mca->parameter_type = MXLV_MCA_ROI_INTEGRAL_ARRAY;

	mx_status = (*get_parameter)( mca );

	if ( roi_integral_array != NULL ) {
		for ( i = 0; i < mca->current_num_rois; i++ ) {
			roi_integral_array[i] = mca->roi_integral_array[i];
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_num_channels( MX_RECORD *mca_record,
			unsigned long *num_channels )
{
	static const char fname[] = "mx_mca_get_num_channels()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	if ( num_channels == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"num_channels pointer passed is NULL." );
	}

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->parameter_type = MXLV_MCA_CURRENT_NUM_CHANNELS;

	mx_status = (*get_parameter)( mca );

	*num_channels = mca->current_num_channels;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_set_num_channels( MX_RECORD *mca_record,
			unsigned long num_channels )
{
	static const char fname[] = "mx_mca_set_num_channels()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_channels > mca->maximum_num_channels ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested number of MCA channels, %lu, is greater than "
		"the maximum allowed number of MCA channels, %ld.",
			num_channels, mca->maximum_num_channels );
	}

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->parameter_type = MXLV_MCA_CURRENT_NUM_CHANNELS;

	mca->current_num_channels = (long) num_channels;

	mx_status = (*set_parameter)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_channel( MX_RECORD *mca_record,
			unsigned long channel_number,
			unsigned long *channel_value )
{
	static const char fname[] = "mx_mca_get_channel()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_MCA * );
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	/* Set the channel number to be read. */

	mca->parameter_type = MXLV_MCA_CHANNEL_NUMBER;

	mca->channel_number = channel_number;

	mx_status = (*set_parameter)( mca );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now read back the channel value. */

	mca->parameter_type = MXLV_MCA_CHANNEL_VALUE;

	mx_status = (*get_parameter)( mca );

	*channel_value = mca->channel_value;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_real_time( MX_RECORD *mca_record, double *real_time )
{
	static const char fname[] = "mx_mca_get_real_time()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->parameter_type = MXLV_MCA_REAL_TIME;

	mx_status = (*get_parameter)( mca );

	if ( real_time != NULL ) {
		*real_time = mca->real_time;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_live_time( MX_RECORD *mca_record, double *live_time )
{
	static const char fname[] = "mx_mca_get_live_time()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->parameter_type = MXLV_MCA_LIVE_TIME;

	mx_status = (*get_parameter)( mca );

	if ( live_time != NULL ) {
		*live_time = mca->live_time;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_counts( MX_RECORD *mca_record, unsigned long *counts )
{
	static const char fname[] = "mx_mca_get_counts()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->parameter_type = MXLV_MCA_COUNTS;

	mx_status = (*get_parameter)( mca );

	if ( counts != NULL ) {
		*counts = mca->counts;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_soft_roi( MX_RECORD *mca_record,
			unsigned long soft_roi_number,
			unsigned long *soft_roi )
{
	static const char fname[] = "mx_mca_get_soft_roi()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca->num_soft_rois == 0 ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Soft ROIs are not available for MCA '%s'.",
		mca_record->name );
	}

	if ( soft_roi_number >= mca->num_soft_rois ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Requested soft ROI number %lu is outside the allowed range "
		"of (0-%ld) for MCA '%s'.", soft_roi_number,
			mca->num_soft_rois - 1, mca_record->name );
	}

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name);
	}

	mca->soft_roi_number = soft_roi_number;

	mca->parameter_type = MXLV_MCA_SOFT_ROI;

	mx_status = (*get_parameter)( mca );

	mca->soft_roi_array[ soft_roi_number ][0] = mca->soft_roi[0];
	mca->soft_roi_array[ soft_roi_number ][1] = mca->soft_roi[1];

	if ( soft_roi != NULL ) {
		soft_roi[0] = mca->soft_roi[0];
		soft_roi[1] = mca->soft_roi[1];
	}

	MX_DEBUG( 2,("%s: mca->soft_roi[0] = %lu", fname, mca->soft_roi[0]));
	MX_DEBUG( 2,("%s: mca->soft_roi[1] = %lu", fname, mca->soft_roi[1]));

	MX_DEBUG( 2,("%s: mca->soft_roi_array[%lu][0] = %lu",
	  fname, soft_roi_number, mca->soft_roi_array[ soft_roi_number ][0]));
	MX_DEBUG( 2,("%s: mca->soft_roi_array[%lu][1] = %lu",
	  fname, soft_roi_number, mca->soft_roi_array[ soft_roi_number ][1]));

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_set_soft_roi( MX_RECORD *mca_record,
			unsigned long soft_roi_number,
			unsigned long *soft_roi )
{
	static const char fname[] = "mx_mca_set_soft_roi()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	if ( soft_roi == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The soft_roi pointer passed was NULL." );
	}

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca->num_soft_rois == 0 ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Soft ROIs are not available for MCA '%s'.",
		mca_record->name );
	}

	if ( soft_roi_number >= mca->num_soft_rois ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Requested soft ROI number %lu is outside the allowed range "
		"of (0-%ld) for MCA '%s'.", soft_roi_number,
			mca->num_soft_rois - 1, mca_record->name );
	}

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_soft_roi function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->soft_roi_number = soft_roi_number;

	mca->parameter_type = MXLV_MCA_SOFT_ROI;

	mca->soft_roi_array[ soft_roi_number ][0] = soft_roi[0];
	mca->soft_roi_array[ soft_roi_number ][1] = soft_roi[1];

	mca->soft_roi[0] = soft_roi[0];
	mca->soft_roi[1] = soft_roi[1];

	MX_DEBUG( 2,("%s: mca->soft_roi[0] = %lu", fname, mca->soft_roi[0]));
	MX_DEBUG( 2,("%s: mca->soft_roi[1] = %lu", fname, mca->soft_roi[1]));

	MX_DEBUG( 2,("%s: mca->soft_roi_array[%lu][0] = %lu",
	  fname, soft_roi_number, mca->soft_roi_array[ soft_roi_number ][0]));
	MX_DEBUG( 2,("%s: mca->soft_roi_array[%lu][1] = %lu",
	  fname, soft_roi_number, mca->soft_roi_array[ soft_roi_number ][1]));

	mx_status = (*set_parameter)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_soft_roi_integral( MX_RECORD *mca_record,
			unsigned long soft_roi_number,
			unsigned long *soft_roi_integral )
{
	static const char fname[] = "mx_mca_get_soft_roi_integral()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca->num_soft_rois == 0 ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Soft ROIs are not available for MCA '%s'.",
		mca_record->name );
	}

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->soft_roi_number = soft_roi_number;

	mca->parameter_type = MXLV_MCA_SOFT_ROI_INTEGRAL;

	mx_status = (*get_parameter)( mca );

	mca->soft_roi_integral_array[ soft_roi_number ]
						= mca->soft_roi_integral;

	MX_DEBUG( 2,("%s: Soft ROI %lu integral = %lu",
		fname, mca->soft_roi_number, mca->soft_roi_integral));

	if ( soft_roi_integral != NULL ) {
		*soft_roi_integral = mca->soft_roi_integral;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_soft_roi_integral_array( MX_RECORD *mca_record,
			unsigned long num_soft_rois,
			unsigned long *soft_roi_integral_array )
{
	static const char fname[] = "mx_mca_get_soft_roi_integral_array()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	unsigned long i;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	if ( soft_roi_integral_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"soft_roi_integral_array pointer passed is NULL." );
	}

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_soft_rois >= mca->num_soft_rois ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Requested number of soft ROIs %lu is outside the allowed range "
	"of (0-%ld) for MCA '%s'.", num_soft_rois,
			mca->num_soft_rois, mca_record->name );
	}

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->parameter_type = MXLV_MCA_SOFT_ROI_INTEGRAL_ARRAY;

	mx_status = (*get_parameter)( mca );

	if ( soft_roi_integral_array != NULL ) {
		for ( i = 0; i < num_soft_rois; i++ ) {
			soft_roi_integral_array[i] =
				mca->soft_roi_integral_array[i];
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_energy_scale( MX_RECORD *mca_record,
			double *energy_scale )
{
	static const char fname[] = "mx_mca_get_energy_scale()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->parameter_type = MXLV_MCA_ENERGY_SCALE;

	mx_status = (*get_parameter)( mca );

	if ( energy_scale != NULL ) {
		*energy_scale = mca->energy_scale;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_set_energy_scale( MX_RECORD *mca_record,
			double energy_scale )
{
	static const char fname[] = "mx_mca_set_energy_scale()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->parameter_type = MXLV_MCA_ENERGY_SCALE;

	mca->energy_scale = energy_scale;

	mx_status = (*set_parameter)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_energy_offset( MX_RECORD *mca_record,
			double *energy_offset )
{
	static const char fname[] = "mx_mca_get_energy_offset()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->parameter_type = MXLV_MCA_ENERGY_SCALE;

	mx_status = (*get_parameter)( mca );

	if ( energy_offset != NULL ) {
		*energy_offset = mca->energy_offset;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_set_energy_offset( MX_RECORD *mca_record,
			double energy_offset )
{
	static const char fname[] = "mx_mca_set_energy_offset()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->parameter_type = MXLV_MCA_ENERGY_SCALE;

	mca->energy_offset = energy_offset;

	mx_status = (*set_parameter)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_energy_axis_array( MX_RECORD *mca_record,
			unsigned long num_channels,
			double *energy_axis_array )
{
	static const char fname[] = "mx_mca_get_energy_axis_array()";

	MX_MCA *mca;
	unsigned long i;
	double energy_scale, energy_offset;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record, &mca, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( energy_axis_array == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The energy_axis_array pointer passed was NULL." );
	}

	mx_status = mx_mca_get_energy_scale( mca_record, &energy_scale );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mca_get_energy_offset( mca_record, &energy_offset );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < num_channels; i++ ) {
		energy_axis_array[i]
			= energy_offset + energy_scale * (double) i;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mca_get_input_count_rate( MX_RECORD *mca_record,
			double *input_count_rate )
{
	static const char fname[] = "mx_mca_get_input_count_rate()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->parameter_type = MXLV_MCA_INPUT_COUNT_RATE;

	mx_status = (*get_parameter)( mca );

	if ( input_count_rate != NULL ) {
		*input_count_rate = mca->input_count_rate;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_get_output_count_rate( MX_RECORD *mca_record,
			double *output_count_rate )
{
	static const char fname[] = "mx_mca_get_output_count_rate()";

	MX_MCA *mca;
	MX_MCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MCA * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s'", fname, mca_record->name));

	mx_status = mx_mca_get_pointers( mca_record,
					&mca, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			mca_record->name );
	}

	mca->parameter_type = MXLV_MCA_OUTPUT_COUNT_RATE;

	mx_status = (*get_parameter)( mca );

	if ( output_count_rate != NULL ) {
		*output_count_rate = mca->output_count_rate;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mca_default_get_parameter_handler( MX_MCA *mca )
{
	static const char fname[] = "mx_mca_default_get_parameter_handler()";

	unsigned long i, j, channel_value, integral;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	switch( mca->parameter_type ) {
	case MXLV_MCA_CURRENT_NUM_CHANNELS:
	case MXLV_MCA_CHANNEL_NUMBER:
	case MXLV_MCA_REAL_TIME:
	case MXLV_MCA_LIVE_TIME:
	case MXLV_MCA_COUNTS:
	case MXLV_MCA_PRESET_TYPE:
	case MXLV_MCA_PRESET_REAL_TIME:
	case MXLV_MCA_PRESET_LIVE_TIME:
	case MXLV_MCA_PRESET_COUNT:
	case MXLV_MCA_ENERGY_SCALE:
	case MXLV_MCA_ENERGY_OFFSET:
	case MXLV_MCA_NEW_DATA_AVAILABLE:

		/* None of these cases require any action since the value
		 * is already in the location it needs to be in.
		 */

		break;

	case MXLV_MCA_INPUT_COUNT_RATE:
		mca->input_count_rate = 0.0;
		break;

	case MXLV_MCA_OUTPUT_COUNT_RATE:
		mca->output_count_rate = 0.0;
		break;

	case MXLV_MCA_ROI:
		i = mca->roi_number;

		mca->roi[0] = mca->roi_array[i][0];
		mca->roi[1] = mca->roi_array[i][1];
		break;

	case MXLV_MCA_ROI_INTEGRAL:

		i = mca->roi_number;

		integral = 0L;

		for ( j = mca->roi_array[i][0]; j <= mca->roi_array[i][1]; j++ )
		{

			channel_value = mca->channel_array[j];

			/* If adding the value in the current channel
			 * would result in the integral exceeding ULONG_MAX,
			 * assign ULONG_MAX to the integral and break out
			 * of the loop.
			 */

			if ( integral > ( ULONG_MAX - channel_value ) ) {
				integral = ULONG_MAX;
				break;
			}

			integral += channel_value;
		}

		mca->roi_integral = integral;

		mca->roi_integral_array[i] = integral;
		break;

	case MXLV_MCA_SOFT_ROI:
		i = mca->soft_roi_number;

		mca->soft_roi[0] = mca->soft_roi_array[i][0];
		mca->soft_roi[1] = mca->soft_roi_array[i][1];
		break;

	case MXLV_MCA_SOFT_ROI_INTEGRAL:

		i = mca->soft_roi_number;

		MX_DEBUG( 2,("%s: soft_roi_number = %lu", fname, i));
		MX_DEBUG( 2,("%s: soft roi lower limit = %lu",
			fname, mca->soft_roi_array[i][0]));
		MX_DEBUG( 2,("%s: soft roi upper limit = %lu",
			fname, mca->soft_roi_array[i][1]));

		integral = 0L;

		for ( j = mca->soft_roi_array[i][0];
			j <= mca->soft_roi_array[i][1]; j++ )
		{

			channel_value = mca->channel_array[j];

			/* If adding the value in the current channel
			 * would result in the integral exceeding ULONG_MAX,
			 * assign ULONG_MAX to the integral and break out
			 * of the loop.
			 */

			if ( integral > ( ULONG_MAX - channel_value ) ) {
				integral = ULONG_MAX;
				break;
			}

			integral += channel_value;
		}

		mca->soft_roi_integral = integral;

		MX_DEBUG( 2,("%s: soft_roi_integral = %lu",
			fname, mca->soft_roi_integral));

		mca->soft_roi_integral_array[i] = integral;
		break;

	case MXLV_MCA_CHANNEL_VALUE:

		if ( mca->channel_number >= mca->maximum_num_channels ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"mca->channel_number (%lu) is greater than or equal to "
		"mca->maximum_num_channels (%ld).  This should not be "
		"able to happen, so if you see this message, please "
		"report the program bug to Bill Lavender.",
			mca->channel_number, mca->maximum_num_channels );
		}
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by this driver.",
			mca->parameter_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mca_default_set_parameter_handler( MX_MCA *mca )
{
	static const char fname[] = "mx_mca_default_set_parameter_handler()";

	unsigned long i, roi_start, roi_end, soft_roi_start, soft_roi_end;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	i = mca->roi_number;

	switch( mca->parameter_type ) {
	case MXLV_MCA_PRESET_TYPE:
	case MXLV_MCA_PRESET_REAL_TIME:
	case MXLV_MCA_PRESET_LIVE_TIME:
	case MXLV_MCA_PRESET_COUNT:
	case MXLV_MCA_ENERGY_SCALE:
	case MXLV_MCA_ENERGY_OFFSET:

		/* These cases do not require any special action since the
		 * correct value is already in the right place by the time
		 * we get to this function.
		 */

		break;

	case MXLV_MCA_ROI:

		/* The values are already in mca->roi_array.
		 * Check to see if they are legal values.
		 */

		roi_start = mca->roi_array[i][0];
		roi_end   = mca->roi_array[i][1];

		if ( roi_start >= mca->maximum_num_channels ) {

			roi_start = mca->maximum_num_channels - 1;

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The requested ROI start channel %lu is outside the allowed range of (0-%ld) "
"for the MCA '%s'.", roi_start, mca->maximum_num_channels-1,
					mca->record->name );
		}
		if ( roi_end >= mca->maximum_num_channels ) {

			roi_end = mca->maximum_num_channels - 1;

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The requested ROI end channel %lu is outside the allowed range of (%lu-%ld) "
"for the MCA '%s'.", roi_end, roi_start, mca->maximum_num_channels-1,
					mca->record->name );
		}
		if ( roi_start > roi_end ) {
			roi_end = roi_start;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested ROI start channel %lu is after "
			"the requested ROI end channel %lu for MCA '%s'.  "
			"This is not permitted.",
				roi_start, roi_end, mca->record->name );
		}

		mca->roi[0] = roi_start;
		mca->roi[1] = roi_end;

		mca->roi_array[i][0] = roi_start;
		mca->roi_array[i][1] = roi_end;

		break;

	case MXLV_MCA_SOFT_ROI:

		/* The values are already in mca->soft_roi_array.
		 * Check to see if they are legal values.
		 */

		soft_roi_start = mca->soft_roi_array[i][0];
		soft_roi_end   = mca->soft_roi_array[i][1];

		if ( soft_roi_start >= mca->maximum_num_channels ) {

			soft_roi_start = mca->maximum_num_channels - 1;

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The requested soft ROI start channel %lu is outside the allowed range "
"of (0-%ld) for the MCA '%s'.", soft_roi_start, mca->maximum_num_channels-1,
					mca->record->name );
		}
		if ( soft_roi_end >= mca->maximum_num_channels ) {

			soft_roi_end = mca->maximum_num_channels - 1;

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The requested soft ROI end channel %lu is outside the allowed range "
"of (%lu-%ld) for the MCA '%s'.", soft_roi_end, soft_roi_start,
					mca->maximum_num_channels-1,
					mca->record->name );
		}
		if ( soft_roi_start > soft_roi_end ) {
			soft_roi_end = soft_roi_start;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested soft ROI start channel %lu is after "
			"the requested soft ROI end channel %lu for MCA '%s'.  "
			"This is not permitted.",
				soft_roi_start, soft_roi_end,
				mca->record->name );
		}

		mca->soft_roi[0] = soft_roi_start;
		mca->soft_roi[1] = soft_roi_end;

		mca->soft_roi_array[i][0] = soft_roi_start;
		mca->soft_roi_array[i][1] = soft_roi_end;

		break;

	case MXLV_MCA_CHANNEL_NUMBER:

		/* The relevant number is already in mca->channel_number,
		 * so just check to see if it is a legal value.
		 */

		if ( mca->channel_number >= mca->maximum_num_channels ) {
			unsigned long channel_number;

			channel_number = mca->channel_number;
			mca->channel_number = 0;

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The requested channel number %lu is outside the allowed range of (0-%ld) "
"for the MCA '%s'.", channel_number, mca->maximum_num_channels-1,
				mca->record->name );
		}
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by this driver.",
			mca->parameter_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

