/*
 * Name:    mx_mce.c
 *
 * Purpose: MX function library for multichannel mces.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_mce.h"

MX_EXPORT mx_status_type
mx_mce_get_pointers( MX_RECORD *mce_record,
			MX_MCE **mce,
			MX_MCE_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_mce_get_pointers()";

	if ( mce_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The mce_record pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( mce_record->mx_class != MXC_MULTICHANNEL_ENCODER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The record '%s' passed by '%s' is not a MCE interface.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			mce_record->name, calling_fname,
			mce_record->mx_superclass,
			mce_record->mx_class,
			mce_record->mx_type );
	}

	if ( mce != (MX_MCE **) NULL ) {
		*mce = (MX_MCE *) (mce_record->record_class_struct);

		if ( *mce == (MX_MCE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_MCE pointer for record '%s' passed by '%s' is NULL.",
				mce_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_MCE_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_MCE_FUNCTION_LIST *)
				(mce_record->class_specific_function_list);

		if ( *function_list_ptr == (MX_MCE_FUNCTION_LIST *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_MCE_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				mce_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mce_initialize_type( long record_type,
			mx_length_type *num_record_fields,
			MX_RECORD_FIELD_DEFAULTS **record_field_defaults,
			mx_length_type *maximum_num_values_varargs_cookie )
{
	static const char fname[] = "mx_mce_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
	MX_RECORD_FIELD_DEFAULTS *field;
	mx_length_type referenced_field_index;
	mx_status_type mx_status;

	if ( num_record_fields == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"num_record_fields pointer passed was NULL." );
	}
	if ( record_field_defaults == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"record_field_defaults pointer passed was NULL." );
	}
	if ( maximum_num_values_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"maximum_num_values_varargs_cookie pointer passed was NULL." );
	}

	driver = mx_get_driver_by_type( record_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.",
			record_type );
	}

	record_field_defaults_ptr
			= driver->record_field_defaults_ptr;

	if (record_field_defaults_ptr == (MX_RECORD_FIELD_DEFAULTS **) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	*record_field_defaults = *record_field_defaults_ptr;

	if ( *record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	if ( driver->num_record_fields == (mx_length_type *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'num_record_fields' pointer for record type '%s' is NULL.",
			driver->name );
	}

	*num_record_fields = *(driver->num_record_fields);

	/* Set varargs cookies in 'data_array' that depend on the values
	 * of 'maximum_num_values' and 'maximum_num_measurements'.
	 */

	mx_status = mx_find_record_field_defaults( *record_field_defaults,
			*num_record_fields, "value_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults_index(
			*record_field_defaults, *num_record_fields,
			"maximum_num_values", &referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
					maximum_num_values_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *maximum_num_values_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

/* mx_mce_fixup_motor_record_array_field() should be invoked anytime
 * the value of mce->num_motors has changed.
 */

MX_EXPORT mx_status_type
mx_mce_fixup_motor_record_array_field( MX_MCE *mce )
{
	static const char fname[] = "mx_mce_fixup_motor_record_array_field()";

	MX_RECORD_FIELD *field;
	mx_status_type mx_status;

	/* Find the 'motor_record_array' field. */

	mx_status = mx_find_record_field( mce->record,
				"motor_record_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: field->num_dimensions = %ld",
			fname, (long) field->num_dimensions));

	field->dimension[0] = mce->num_motors;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mce_get_overflow_status( MX_RECORD *mce_record,
		bool *underflow_set, bool *overflow_set )
{
	static const char fname[] = "mx_mce_get_overflow_status()";

	MX_MCE *mce;
	MX_MCE_FUNCTION_LIST *function_list;
	mx_status_type ( *get_overflow_status_fn ) ( MX_MCE * );
	mx_status_type mx_status;

	mx_status = mx_mce_get_pointers( mce_record, &mce,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_overflow_status_fn = function_list->get_overflow_status;

	if ( get_overflow_status_fn == NULL ) {
		mce->overflow_set = FALSE;
		mce->underflow_set = FALSE;
	} else {
		mx_status = ( *get_overflow_status_fn )( mce );
	}

	*overflow_set = mce->overflow_set;
	*underflow_set = mce->underflow_set;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mce_reset_overflow_status( MX_RECORD *mce_record )
{
	static const char fname[] = "mx_mce_reset_overflow_status()";

	MX_MCE *mce;
	MX_MCE_FUNCTION_LIST *function_list;
	mx_status_type ( *reset_overflow_status_fn ) ( MX_MCE * );
	mx_status_type mx_status;

	mx_status = mx_mce_get_pointers( mce_record, &mce,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	reset_overflow_status_fn = function_list->reset_overflow_status;

	if ( reset_overflow_status_fn == NULL ) {
		mce->overflow_set = FALSE;
		mce->underflow_set = FALSE;
	} else {
		mx_status = ( *reset_overflow_status_fn )( mce );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mce_read( MX_RECORD *mce_record,
		mx_length_type *num_values,
		double **value_array )
{
	static const char fname[] = "mx_mce_read()";

	MX_MCE *mce;
	MX_MCE_FUNCTION_LIST *function_list;
	mx_status_type ( *read_fn ) ( MX_MCE * );
	mx_status_type mx_status;

	mx_status = mx_mce_get_pointers( mce_record, &mce,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_fn = function_list->read;

	if ( read_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"read function ptr for MX_MCE ptr 0x%p is NULL.",
			mce);
	}

	mx_status = ( *read_fn )( mce );

	if ( num_values != NULL ) {
		*num_values = mce->current_num_values;
	}
	if ( value_array != NULL ) {
		*value_array = mce->value_array;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mce_get_current_num_values( MX_RECORD *mce_record,
		mx_length_type *num_values )
{
	static const char fname[] = "mx_mce_get_current_num_values()";

	MX_MCE *mce;
	MX_MCE_FUNCTION_LIST *function_list;
	mx_status_type ( *get_current_num_values_fn ) ( MX_MCE * );
	mx_status_type mx_status;

	mx_status = mx_mce_get_pointers( mce_record, &mce,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_current_num_values_fn = function_list->get_current_num_values;

	if ( get_current_num_values_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_current_num_values function ptr for MX_MCE ptr 0x%p is NULL.",
			mce);
	}

	mx_status = ( *get_current_num_values_fn )( mce );

	if ( num_values != NULL ) {
		*num_values = mce->current_num_values;
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mx_mce_get_motor_record_array( MX_RECORD *mce_record,
				int *num_motors,
				MX_RECORD ***motor_record_array )
{
	static const char fname[] = "mx_mce_get_motor_record_array()";

	MX_MCE *mce;
	MX_MCE_FUNCTION_LIST *function_list;
	mx_status_type ( *get_motor_record_array_fn ) ( MX_MCE * );
	mx_status_type mx_status;

	mx_status = mx_mce_get_pointers( mce_record, &mce,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_motor_record_array_fn = function_list->get_motor_record_array;

	MX_DEBUG( 2,("%s: get_motor_record_array_fn = %p",
			fname, get_motor_record_array_fn ));

	if ( get_motor_record_array_fn != NULL ) {
		mx_status = ( *get_motor_record_array_fn ) ( mce );
	}

	if ( num_motors != NULL ) {
		*num_motors = mce->num_motors;
	}
	if ( motor_record_array != (MX_RECORD ***) NULL ) {
		*motor_record_array = mce->motor_record_array;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_mce_connect_mce_to_motor( MX_RECORD *mce_record,
				MX_RECORD *motor_record )
{
	static const char fname[] = "mx_mce_connect_mce_to_motor()";

	MX_MCE *mce;
	MX_MCE_FUNCTION_LIST *function_list;
	mx_status_type ( *connect_mce_to_motor_fn ) ( MX_MCE *, MX_RECORD * );

	mx_status_type mx_status;

	if ( motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	mx_status = mx_mce_get_pointers( mce_record, &mce,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( mce->selected_motor_name, motor_record->name,
				MXU_RECORD_NAME_LENGTH );

	connect_mce_to_motor_fn = function_list->connect_mce_to_motor;

	MX_DEBUG( 2,("%s: connect_mce_to_motor_fn = %p",
			fname, connect_mce_to_motor_fn ));

	if ( connect_mce_to_motor_fn == NULL ) {
		/* If the function pointer is NULL, then we assume that there
		 * is nothing that needs to be done to connect the motor to
		 * the multichannel encoder and exit with a success status
		 * code.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = ( *connect_mce_to_motor_fn ) ( mce, motor_record );

	return mx_status;
}

