/*
 * Name:    d_newport_xps.c
 *
 * Purpose: MX driver for the Stanford Research Systems NEWPORT_XPS_MOTOR
 *          Analog PID Controller.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NEWPORT_XPS_MOTOR_DEBUG	FALSE

#define MXD_NEWPORT_XPS_MOTOR_ERROR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_motor.h"
#include "i_newport_xps.h"
#include "d_newport_xps.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_newport_xps_record_function_list = {
	NULL,
	mxd_newport_xps_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	NULL,
	mxd_newport_xps_open
};

MX_MOTOR_FUNCTION_LIST mxd_newport_xps_motor_function_list = {
	NULL,
	mxd_newport_xps_move_absolute,
	mxd_newport_xps_get_position,
	NULL,
	mxd_newport_xps_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_newport_xps_get_parameter,
	mxd_newport_xps_set_parameter,
	NULL,
	mxd_newport_xps_get_status
};

/* NEWPORT_XPS_MOTOR motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_newport_xps_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_NEWPORT_XPS_MOTOR_STANDARD_FIELDS
};

long mxd_newport_xps_num_record_fields
		= sizeof( mxd_newport_xps_record_field_defaults )
			/ sizeof( mxd_newport_xps_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_newport_xps_rfield_def_ptr
			= &mxd_newport_xps_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_newport_xps_get_pointers( MX_MOTOR *motor,
			MX_NEWPORT_XPS_MOTOR **newport_xps_motor,
			MX_NEWPORT_XPS **newport_xps,
			const char *calling_fname )
{
	static const char fname[] = "mxd_newport_xps_get_pointers()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor_ptr;
	MX_RECORD *newport_xps_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	newport_xps_motor_ptr = (MX_NEWPORT_XPS_MOTOR *)
				motor->record->record_type_struct;

	if ( newport_xps_motor_ptr == (MX_NEWPORT_XPS_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NEWPORT_XPS_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( newport_xps_motor != (MX_NEWPORT_XPS_MOTOR **) NULL ) {
		*newport_xps_motor = newport_xps_motor_ptr;
	}

	if ( newport_xps != (MX_NEWPORT_XPS **) NULL ) {
		newport_xps_record = newport_xps_motor_ptr->newport_xps_record;

		if ( newport_xps_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The newport_xps_record pointer for record '%s' "
			"is NULL.", motor->record->name );
		}

		*newport_xps = (MX_NEWPORT_XPS *)
				newport_xps_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_newport_xps_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_newport_xps_create_record_structures()";

	MX_MOTOR *motor;
	MX_NEWPORT_XPS_MOTOR *newport_xps_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	newport_xps_motor = (MX_NEWPORT_XPS_MOTOR *)
			malloc( sizeof(MX_NEWPORT_XPS_MOTOR) );

	if ( newport_xps_motor == (MX_NEWPORT_XPS_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_NEWPORT_XPS_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = newport_xps_motor;
	record->class_specific_function_list =
				&mxd_newport_xps_motor_function_list;

	motor->record = record;
	newport_xps_motor->record = record;

	/* A NEWPORT_XPS_MOTOR is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_xps_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_newport_xps_open()";

	MX_MOTOR *motor = NULL;
	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_newport_xps_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_move_absolute()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_newport_xps_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_get_position()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_xps_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_soft_abort()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_newport_xps_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_get_parameter()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NEWPORT_XPS_MOTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		break;

	case MXLV_MTR_EXTRA_GAIN:
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_xps_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_set_parameter()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NEWPORT_XPS_MOTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		break;

	case MXLV_MTR_AXIS_ENABLE:
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		break;

	case MXLV_MTR_EXTRA_GAIN:
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_newport_xps_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_get_status()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status = 0;

#if MXD_NEWPORT_XPS_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Motor '%s', status = %#lx",
		fname, motor->record->name, motor->status));
#endif

	return MX_SUCCESSFUL_RESULT;
}
