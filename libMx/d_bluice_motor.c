/*
 * Name:    d_bluice_motor.c
 *
 * Purpose: MX driver for Blu-Ice controlled motors.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_BLUICE_MOTOR_DEBUG	TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_socket.h"
#include "mx_mutex.h"

#include "mx_bluice.h"
#include "d_bluice_motor.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_bluice_motor_record_function_list = {
	NULL,
	mxd_bluice_motor_create_record_structures,
	mxd_bluice_motor_finish_record_initialization,
	NULL,
	mxd_bluice_motor_print_structure,
	NULL,
	NULL,
	mxd_bluice_motor_open,
	NULL,
	NULL,
	mxd_bluice_motor_open
};

MX_MOTOR_FUNCTION_LIST mxd_bluice_motor_motor_function_list = {
	NULL,
	mxd_bluice_motor_move_absolute,
	mxd_bluice_motor_get_position,
	mxd_bluice_motor_set_position,
	mxd_bluice_motor_soft_abort,
	mxd_bluice_motor_immediate_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_bluice_motor_get_parameter,
	mxd_bluice_motor_set_parameter,
	NULL,
	mxd_bluice_motor_get_status
};

/* Blu-Ice motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_bluice_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_BLUICE_MOTOR_STANDARD_FIELDS
};

long mxd_bluice_motor_num_record_fields
		= sizeof( mxd_bluice_motor_record_field_defaults )
			/ sizeof( mxd_bluice_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bluice_motor_rfield_def_ptr
			= &mxd_bluice_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_bluice_motor_get_pointers( MX_MOTOR *motor,
			MX_BLUICE_MOTOR **bluice_motor,
			MX_BLUICE_SERVER **bluice_server,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bluice_motor_get_pointers()";

	MX_BLUICE_MOTOR *bluice_motor_ptr;
	MX_RECORD *bluice_server_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( bluice_motor == (MX_BLUICE_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bluice_motor_ptr = (MX_BLUICE_MOTOR *)
				motor->record->record_type_struct;

	if ( bluice_motor_ptr == (MX_BLUICE_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( bluice_motor != (MX_BLUICE_MOTOR **) NULL ) {
		*bluice_motor = bluice_motor_ptr;
	}

	bluice_server_record = bluice_motor_ptr->bluice_server_record;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'bluice_server_record' pointer for record '%s' "
		"is NULL.", motor->record->name );
	}

	switch( bluice_server_record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
	case MXN_BLUICE_DHS_SERVER:
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Blu-Ice server record '%s' should be either of type "
		"'bluice_dcss_server' or 'bluice_dhs_server'.  Instead, it is "
		"of type '%s'.",
			bluice_server_record->name,
			mx_get_driver_name( bluice_server_record ) );
		break;
	}

	if ( bluice_server != (MX_BLUICE_SERVER **) NULL ) {
		*bluice_server = (MX_BLUICE_SERVER *)
				bluice_server_record->record_class_struct;

		if ( (*bluice_server) == (MX_BLUICE_SERVER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BLUICE_SERVER pointer for Blu-Ice server "
			"record '%s' used by record '%s' is NULL.",
				bluice_server_record->name,
				motor->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_bluice_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_bluice_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_BLUICE_MOTOR *bluice_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	bluice_motor = (MX_BLUICE_MOTOR *) malloc( sizeof(MX_BLUICE_MOTOR) );

	if ( bluice_motor == (MX_BLUICE_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_BLUICE_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = bluice_motor;
	record->class_specific_function_list =
			&mxd_bluice_motor_motor_function_list;

	motor->record = record;
	bluice_motor->record = record;

	/* A Blu-Ice controlled motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* The Blu-Ice reports acceleration time in seconds. */

	motor->acceleration_type = MXF_MTR_ACCEL_TIME;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_bluice_motor_print_structure()";

	MX_MOTOR *motor;
	MX_BLUICE_MOTOR *bluice_motor;
	double position, move_deadband, speed;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_bluice_motor_get_pointers( motor,
					&bluice_motor, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type         = Blu-Ice motor.\n\n");

	fprintf(file, "  name               = %s\n", record->name);
	fprintf(file, "  server             = %s\n",
				    bluice_motor->bluice_server_record->name);
	fprintf(file, "  Blu-Ice name       = %s\n",
					bluice_motor->bluice_name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
		}
	
	fprintf(file, "  position           = %g %s  (%g)\n",
			motor->position, motor->units,
			motor->raw_position.analog );
	fprintf(file, "  scale              = %g %s per step.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset             = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash           = %g %s  (%g)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );
	
	fprintf(file, "  negative limit     = %g %s  (%g)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive limit     = %g %s  (%g)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband      = %g %s  (%g)\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	mx_status = mx_motor_get_speed( record, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  speed              = %g %s/s  (%g steps/s)\n",
		speed, motor->units,
		motor->raw_speed );

	fprintf(file, "\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_bluice_motor_open()";

	MX_MOTOR *motor;
	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	void *device_ptr;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_bluice_motor_get_pointers( motor,
					&bluice_motor, &bluice_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_bluice_get_device_pointer( bluice_motor->bluice_name,
						bluice_server->motor_array,
						bluice_server->num_motors,
						&device_ptr );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bluice_motor->foreign_motor = (MX_BLUICE_FOREIGN_MOTOR *) device_ptr;

	MX_DEBUG(-2,("%s: Motor '%s', bluice_motor->foreign_motor = %p",
		fname, record->name, bluice_motor->foreign_motor));

	if ( bluice_motor->foreign_motor == (MX_BLUICE_FOREIGN_MOTOR *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The MX_BLUICE_FOREIGN_MOTOR pointer for motor '%s' "
		"has not yet been initialized.", record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_bluice_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_move_absolute()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor,
					&bluice_motor, &bluice_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( bluice_server->record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
		sprintf( command, "gtos_start_motor_move %s %g",
				bluice_motor->bluice_name,
				motor->raw_destination.analog );
		break;
	case MXN_BLUICE_DHS_SERVER:
		sprintf( command, "stoh_start_motor_move %s %g",
				bluice_motor->bluice_name,
				motor->raw_destination.analog );
		break;
	}

	bluice_motor->foreign_motor->move_in_progress = TRUE;

	mx_status = mx_bluice_send_message( bluice_server->record,
					command, NULL, 0, -1, TRUE );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_get_position()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor,
					&bluice_motor, &bluice_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.analog = -1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_set_position()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor,
					&bluice_motor, &bluice_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( bluice_server->record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
		sprintf( command, "gtos_set_motor_position %s %g",
				bluice_motor->bluice_name,
				motor->raw_set_position.analog );
		break;
	case MXN_BLUICE_DHS_SERVER:
		sprintf( command, "stoh_set_motor_position %s %g",
				bluice_motor->bluice_name,
				motor->raw_set_position.analog );
		break;
	}

	mx_status = mx_bluice_send_message( bluice_server->record,
					command, NULL, 0, -1, TRUE );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_soft_abort()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor,
					&bluice_motor, &bluice_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( bluice_server->record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
		strcpy( command, "gtos_abort_all soft" );
		break;
	case MXN_BLUICE_DHS_SERVER:
		strcpy( command, "stoh_abort_all soft" );
		break;
	}

	mx_status = mx_bluice_send_message( bluice_server->record,
					command, NULL, 0, -1, TRUE );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_immediate_abort()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor,
					&bluice_motor, &bluice_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( bluice_server->record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
		strcpy( command, "gtos_abort_all hard" );
		break;
	case MXN_BLUICE_DHS_SERVER:
		strcpy( command, "stoh_abort_all hard" );
		break;
	}

	mx_status = mx_bluice_send_message( bluice_server->record,
					command, NULL, 0, -1, TRUE );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_get_parameter()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_MOTOR *foreign_motor;
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor,
					&bluice_motor, &bluice_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	foreign_motor = bluice_motor->foreign_motor;

	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));


	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		motor->raw_speed = 
			foreign_motor->scale_factor * foreign_motor->speed;
		break;
	case MXLV_MTR_ACCELERATION_TIME:
		motor->acceleration_time = foreign_motor->acceleration_time;
		break;
	default:
		return mx_motor_default_get_parameter_handler( motor );
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_set_parameter()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_MOTOR *foreign_motor;
	char command[200], command_name[40];
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor,
					&bluice_motor, &bluice_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	foreign_motor = bluice_motor->foreign_motor;

	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	if ( foreign_motor->is_pseudo ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Setting motor parameters for Blu-Ice pseudomotor '%s' "
		"used by record '%s' is not supported.",
			bluice_motor->bluice_name,
			motor->record->name );
	}

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		foreign_motor->speed = mx_divide_safely( motor->raw_speed,
						foreign_motor->scale_factor );
		break;
	default:
		return mx_motor_default_set_parameter_handler( motor );
		break;
	}

	switch( bluice_server->record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
		strcpy( command_name, "gtos_configure_device" );
		break;
	case MXN_BLUICE_DHS_SERVER:
		strcpy( command_name, "stoh_configure_device" );
		break;
	}

	sprintf( command, "%s %s %g %g %g %g %g %g %g %d %d %d %d %d",
		command_name,
		foreign_motor->name,
		foreign_motor->position,
		foreign_motor->upper_limit,
		foreign_motor->lower_limit,
		foreign_motor->scale_factor,
		foreign_motor->speed,
		foreign_motor->acceleration_time,
		foreign_motor->backlash,
		foreign_motor->lower_limit_on,
		foreign_motor->upper_limit_on,
		foreign_motor->motor_lock_on,
		foreign_motor->backlash_on,
		foreign_motor->reverse_on );

	mx_status = mx_bluice_send_message( bluice_server->record,
					command, NULL, 0, -1, TRUE );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_motor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_bluice_motor_get_status()";

	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_MOTOR *foreign_motor;
	mx_status_type mx_status;

	mx_status = mxd_bluice_motor_get_pointers( motor,
					&bluice_motor, &bluice_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	foreign_motor = bluice_motor->foreign_motor;

	if ( foreign_motor->move_in_progress ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	} else {
		motor->status &= ( ~ MXSF_MTR_IS_BUSY );
	}

	MX_DEBUG( 2,("%s: MX status word = %#lx", fname, motor->status));

	return MX_SUCCESSFUL_RESULT;
}

