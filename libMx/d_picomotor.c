/*
 * Name:    d_picomotor.c
 *
 * Purpose: MX driver for New Focus Picomotor controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PICOMOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "i_picomotor.h"
#include "d_picomotor.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_picomotor_record_function_list = {
	NULL,
	mxd_picomotor_create_record_structures,
	mxd_picomotor_finish_record_initialization,
	NULL,
	mxd_picomotor_print_structure,
	NULL,
	NULL,
	mxd_picomotor_open,
	NULL,
	NULL,
	mxd_picomotor_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_picomotor_motor_function_list = {
	NULL,
	mxd_picomotor_move_absolute,
	mxd_picomotor_get_position,
	mxd_picomotor_set_position,
	mxd_picomotor_soft_abort,
	mxd_picomotor_immediate_abort,
	NULL,
	NULL,
	mxd_picomotor_find_home_position,
	mxd_picomotor_constant_velocity_move,
	mxd_picomotor_get_parameter,
	mxd_picomotor_set_parameter,
	mxd_picomotor_simultaneous_start,
	mxd_picomotor_get_status
};

/* Picomotor motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_picomotor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_PICOMOTOR_STANDARD_FIELDS
};

mx_length_type mxd_picomotor_num_record_fields
		= sizeof( mxd_picomotor_record_field_defaults )
			/ sizeof( mxd_picomotor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_picomotor_rfield_def_ptr
			= &mxd_picomotor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_picomotor_get_pointers( MX_MOTOR *motor,
			MX_PICOMOTOR **picomotor,
			MX_PICOMOTOR_CONTROLLER **picomotor_controller,
			const char *calling_fname )
{
	static const char fname[] = "mxd_picomotor_get_pointers()";

	MX_PICOMOTOR *picomotor_pointer;
	MX_RECORD *picomotor_controller_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	picomotor_pointer = (MX_PICOMOTOR *)
				motor->record->record_type_struct;

	if ( picomotor_pointer == (MX_PICOMOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PICOMOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( picomotor != (MX_PICOMOTOR **) NULL ) {
		*picomotor = picomotor_pointer;
	}

	if ( picomotor_controller != (MX_PICOMOTOR_CONTROLLER **) NULL ) {
		picomotor_controller_record =
			picomotor_pointer->picomotor_controller_record;

		if ( picomotor_controller_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The picomotor_controller_record pointer for record '%s' is NULL.",
				motor->record->name );
		}

		*picomotor_controller = (MX_PICOMOTOR_CONTROLLER *)
				picomotor_controller_record->record_type_struct;

		if ( *picomotor_controller == (MX_PICOMOTOR_CONTROLLER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PICOMOTOR_CONTROLLER pointer for record '%s' is NULL.",
				motor->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_picomotor_motor_command( MX_PICOMOTOR_CONTROLLER *picomotor_controller,
			MX_PICOMOTOR *picomotor,
			char *command,
			char *response,
			size_t max_response_length,
			int debug_flag )
{
	mx_status_type mx_status;

	/* If the target is a Model 8753 open-loop driver, set the
	 * motor channel first.
	 */

#if 0
	if ( picomotor->driver_type == MXF_PICOMOTOR_8753_DRIVER ) {
		char buffer[40];

		sprintf( buffer, "CHL %s=%d",
				picomotor->driver_name,
				picomotor->motor_number );

		mx_status = mxi_picomotor_command( picomotor_controller,
				buffer, NULL, 0, debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
#endif

	/* Then send the real command to the controller. */

	mx_status = mxi_picomotor_command( picomotor_controller,
			command, response, max_response_length, debug_flag );

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mxd_picomotor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_picomotor_create_record_structures()";

	MX_MOTOR *motor;
	MX_PICOMOTOR *picomotor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	picomotor = (MX_PICOMOTOR *)
				malloc( sizeof(MX_PICOMOTOR) );

	if ( picomotor == (MX_PICOMOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_PICOMOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = picomotor;
	record->class_specific_function_list
				= &mxd_picomotor_motor_function_list;

	motor->record = record;

	/* A Picomotor motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* The Picomotor reports acceleration in steps/sec**2. */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_picomotor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	MX_PICOMOTOR *picomotor;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_picomotor_print_structure()";

	MX_MOTOR *motor;
	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	double position, move_deadband, speed;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type        = PICOMOTOR motor.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  controller name   = %s\n",
					picomotor_controller->record->name);
	fprintf(file, "  driver name       = %s\n",
					picomotor->driver_name);
	fprintf(file, "  motor number      = %d\n",
					picomotor->motor_number);
	fprintf(file, "  flags             = %#lx\n",
					(unsigned long) picomotor->flags);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
		}
	
	fprintf(file, "  position          = %g %s  (%ld steps)\n",
			motor->position, motor->units,
			(long) motor->raw_position.stepper );
	fprintf(file, "  scale             = %g %s per step.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset            = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash          = %g %s  (%ld steps)\n",
		motor->backlash_correction, motor->units,
		(long) motor->raw_backlash_correction.stepper );
	
	fprintf(file, "  negative limit    = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		(long) motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit    = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		(long) motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband     = %g %s  (%ld steps)\n",
		move_deadband, motor->units,
		(long) motor->raw_move_deadband.stepper );

	mx_status = mx_motor_get_speed( record, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  speed             = %g %s/s  (%g steps/s)\n",
		speed, motor->units,
		motor->raw_speed );

	fprintf(file, "\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_picomotor_open()";

	MX_MOTOR *motor;
	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char response[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char *ptr;
	int num_items, raw_driver_type;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out what kind of driver is used by this motor. */

	strcpy( command, "DRT" );

	mx_status = mxi_picomotor_command( picomotor_controller, command,
					response, sizeof(response),
					MXD_PICOMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptr = strchr( response, '=' );

	if ( ptr == (char *) NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No equals sign seen in response to command '%s' by "
		"Picomotor '%s'.  Response = '%s'",
			command, record->name, response );
	}

	ptr++;

	num_items = sscanf( ptr, "%d", &raw_driver_type );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No driver type seen in response to '%s' command.  "
		"Response seen = '%s'", command, response );
	}

	switch( raw_driver_type ) {
	case 1:
		picomotor->driver_type = MXF_PICOMOTOR_8753_DRIVER;
		break;
	case 2:
		picomotor->driver_type = MXF_PICOMOTOR_8751_DRIVER;
		break;
	default:
		picomotor->driver_type = MXF_PICOMOTOR_UNKNOWN_DRIVER;

		mx_warning( "Unrecognized driver type %d returned "
			"for '%s' command sent to Picomotor controller '%s'.  "
			"Response = '%s'.",
			    raw_driver_type, command, record->name, response );
		break;
	}

#if 1
	mx_status = mxi_picomotor_command( picomotor_controller, "CHL",
					response, sizeof(response),
					MXD_PICOMOTOR_DEBUG );
#endif

	/* The Model 8753 only supports relative positioning, so for that
	 * model we must store the current absolute position ourselves.
	 */

	if ( picomotor->driver_type == MXF_PICOMOTOR_8753_DRIVER ) {
		picomotor->position_at_start_of_last_move = 0;
	} else {
		picomotor->position_at_start_of_last_move = -1;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_picomotor_resynchronize()";

	MX_PICOMOTOR *picomotor;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	picomotor = (MX_PICOMOTOR *) record->record_type_struct;

	if ( picomotor == (MX_PICOMOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_PICOMOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxi_picomotor_resynchronize(
			picomotor->picomotor_controller_record );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_picomotor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_move_absolute()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	int32_t position_change;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("****** %s invoked for motor '%s' ******", 
		fname, motor->record->name));

	switch( picomotor->driver_type ) {
	case MXF_PICOMOTOR_8753_DRIVER:
		/* The Model 8753 _only_ supports relative moves
		 * so we must compute the position difference between
		 * the current destination and the current position.
		 */

		mx_status = mxd_picomotor_get_position( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		position_change = motor->raw_destination.stepper
				- motor->raw_position.stepper;

		MX_DEBUG( 2,("%s: motor->raw_destination.stepper = %ld",
			fname, (long) motor->raw_destination.stepper));

		MX_DEBUG( 2,("%s: motor->raw_position.stepper = %ld",
		 	fname, (long) motor->raw_position.stepper));

		MX_DEBUG( 2,("%s: position_change = %ld",
			fname, (long) position_change));

		sprintf( command, "REL %s %ld G",
			picomotor->driver_name,
			(long) position_change );

		/* Update picomotor->position_at_start_of_last_move by
		 * setting it to what we think is the current position.
		 */

		picomotor->position_at_start_of_last_move
				= motor->raw_position.stepper;

		MX_DEBUG( 2,
		("%s: NEW picomotor->position_at_start_of_last_move = %ld",
		 	fname, picomotor->position_at_start_of_last_move));
		break;
	default:
		/* For all others, use absolute positioning. */

		sprintf( command, "ABS %s %ld G",
			picomotor->driver_name,
			(long) motor->raw_destination.stepper );
	}

	mx_status = mxd_picomotor_motor_command( picomotor_controller,
					picomotor, command,
					NULL, 0, MXD_PICOMOTOR_DEBUG );

	MX_DEBUG( 2,("****** %s complete for motor '%s' ******", 
		fname, motor->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_get_position()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char response[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	long motor_steps;
	int num_items;
	char *ptr;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("++++++ %s invoked for motor '%s' ++++++", 
		fname, motor->record->name));

	sprintf( command, "POS %s", picomotor->driver_name );

	mx_status = mxd_picomotor_motor_command( picomotor_controller,
						picomotor, command,
						response, sizeof response,
						MXD_PICOMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptr = strchr( response, '=' );

	if ( ptr == (char *) NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No equals sign seen in response to command '%s' by "
		"Picomotor '%s'.  Response = '%s'",
			command, motor->record->name, response );
	}

	ptr++;

	num_items = sscanf( ptr, "%ld", &motor_steps );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No position value seen in response to '%s' command.  "
		"Response seen = '%s'", command, response );
	}

	switch( picomotor->driver_type ) {
	case MXF_PICOMOTOR_8753_DRIVER:
		/* The Model 8753 _only_ reports relative positions,
		 * namely, the number of pulses sent since the last
		 * motion command.  Thus, we use the reported number
		 * of motor steps as an offset from the position at
		 * the start of the last move.
		 */

		MX_DEBUG( 2,
		("%s: picomotor->position_at_start_of_last_move = %ld",
		 	fname, picomotor->position_at_start_of_last_move));

		MX_DEBUG( 2,("%s: motor_steps = %ld", fname, motor_steps));

		motor->raw_position.stepper =
		    picomotor->position_at_start_of_last_move + motor_steps;

		MX_DEBUG( 2,("%s: motor->raw_position.stepper = %ld",
			fname, (long) motor->raw_position.stepper));
		break;
	default:
		/* For all others, use absolute positioning. */

		motor->raw_position.stepper = motor_steps;
		break;
	}

	MX_DEBUG( 2,("++++++ %s complete for motor '%s' ++++++", 
		fname, motor->record->name));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_set_position()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[100];
	long position_difference;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("!!!!!! %s invoked for motor '%s' !!!!!!", 
		fname, motor->record->name));

	if ( picomotor->driver_type == MXF_PICOMOTOR_8751_DRIVER ) {

		/* The Model 8751-C knows the absolute position
		 * of the motor.
		 */

		sprintf( command, "POS %s=%ld", picomotor->driver_name,
				(long) motor->raw_set_position.stepper );

		mx_status = mxi_picomotor_command( picomotor_controller,
				command, NULL, 0, MXD_PICOMOTOR_DEBUG );
	} else {
		/* For the 8753, all we can do is redefine the value
		 * of picomotor->position_at_start_of_last_move.
		 */

		mx_status = mxd_picomotor_get_position( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		position_difference = motor->raw_set_position.stepper
					- motor->raw_position.stepper;

		MX_DEBUG( 2,("%s: motor->raw_set_position.stepper = %ld",
			fname, (long) motor->raw_set_position.stepper));

		MX_DEBUG( 2,("%s: motor->raw_position.stepper = %ld",
			fname, (long) motor->raw_position.stepper));

		MX_DEBUG( 2,("%s: position_difference = %ld",
			fname, position_difference));

		/* Increment the value of position_at_start_of_last_move
		 * by the position difference.
		 */

		MX_DEBUG( 2,
		("%s: OLD picomotor->position_at_start_of_last_move = %ld",
		 	fname, picomotor->position_at_start_of_last_move));

		picomotor->position_at_start_of_last_move
			+= position_difference;

		MX_DEBUG( 2,
		("%s: NEW picomotor->position_at_start_of_last_move = %ld",
		 	fname, picomotor->position_at_start_of_last_move));
	}

	MX_DEBUG( 2,("!!!!!! %s complete for motor '%s' !!!!!!", 
		fname, motor->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_soft_abort()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "HAL %s", picomotor->driver_name );

	mx_status = mxd_picomotor_motor_command( picomotor_controller,
					picomotor, command,
					NULL, 0, MXD_PICOMOTOR_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_immediate_abort()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "STO %s", picomotor->driver_name );

	mx_status = mxd_picomotor_motor_command( picomotor_controller,
					picomotor, command,
					NULL, 0, MXD_PICOMOTOR_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_find_home_position()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( picomotor->flags & MXF_PICOMOTOR_HOME_TO_LIMIT_SWITCH ) {
		if ( motor->home_search >= 0 ) {
			sprintf( command, "FLI %s", picomotor->driver_name );
		} else {
			sprintf( command, "RLI %s", picomotor->driver_name );
		}
	} else {
		if ( motor->home_search >= 0 ) {
			sprintf( command, "FIN %s", picomotor->driver_name );
		} else {
			sprintf( command, "RIN %s", picomotor->driver_name );
		}
	}

	mx_status = mxd_picomotor_motor_command( picomotor_controller,
					picomotor, command,
					NULL, 0, MXD_PICOMOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_constant_velocity_move()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the direction of the move. */

	if ( motor->constant_velocity_move >= 0 ) {
		sprintf( command, "FOR %s G", picomotor->driver_name );
	} else {
		sprintf( command, "REV %s G", picomotor->driver_name );
	}

	mx_status = mxi_picomotor_command( picomotor_controller, command,
					NULL, 0, MXD_PICOMOTOR_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_get_parameter()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char response[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char *ptr;
	int num_items;
	unsigned long ulong_value;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		sprintf( command, "VEL %s %d",
				picomotor->driver_name,
				picomotor->motor_number );

		mx_status = mxi_picomotor_command( picomotor_controller,
				command, response, sizeof(response),
				MXD_PICOMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ptr = strchr( response, '=' );

		if ( ptr == (char *) NULL ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No equals sign seen in response to command '%s' by "
			"Picomotor '%s'.  Response = '%s'",
				command, motor->record->name, response );
		}

		ptr++;

		num_items = sscanf( ptr, "%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to get the speed for motor '%s' "
			"in the response to the command '%s'.  "
			"Response = '%s'",
				motor->record->name, command, response );
		}

		motor->raw_speed = (double) ulong_value;
		break;
	case MXLV_MTR_BASE_SPEED:
		if ( picomotor->driver_type != MXF_PICOMOTOR_8753_DRIVER ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Picomotor '%s' does not use a Model 8753 "
			"open-loop driver, so querying the base speed "
			"is not supported.", motor->record->name );
		}

		sprintf( command, "MPV %s %d",
				picomotor->driver_name,
				picomotor->motor_number );

		mx_status = mxi_picomotor_command( picomotor_controller,
				command, response, sizeof(response),
				MXD_PICOMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ptr = strchr( response, '=' );

		if ( ptr == (char *) NULL ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No equals sign seen in response to command '%s' by "
			"Picomotor '%s'.  Response = '%s'",
				command, motor->record->name, response );
		}

		ptr++;

		num_items = sscanf( ptr, "%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to get the base speed for motor '%s' "
			"in the response to the command '%s'.  "
			"Response = '%s'",
				motor->record->name, command, response );
		}

		motor->raw_base_speed = (double) ulong_value;
		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		sprintf( command, "ACC %s %d",
				picomotor->driver_name,
				picomotor->motor_number );

		mx_status = mxi_picomotor_command( picomotor_controller,
				command, response, sizeof(response),
				MXD_PICOMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ptr = strchr( response, '=' );

		if ( ptr == (char *) NULL ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No equals sign seen in response to command '%s' by "
			"Picomotor '%s'.  Response = '%s'",
				command, motor->record->name, response );
		}

		ptr++;

		num_items = sscanf( ptr, "%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to get the acceleration for motor '%s' "
			"in the response to the command '%s'.  "
			"Response = '%s'",
				motor->record->name, command, response );
		}

		motor->raw_acceleration_parameters[0] = (double) ulong_value;
		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;
		break;

	case MXLV_MTR_MAXIMUM_SPEED:

		/* As documented in the manual. */

		motor->raw_maximum_speed = 4095500;  /* steps per second */
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_set_parameter()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	unsigned long ulong_value;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		ulong_value = (unsigned long) mx_round( motor->raw_speed );

		sprintf( command, "VEL %s %d=%lu",
				picomotor->driver_name,
				picomotor->motor_number,
				ulong_value );
		
		mx_status = mxi_picomotor_command( picomotor_controller,
				command, NULL, 0, MXD_PICOMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;
	case MXLV_MTR_BASE_SPEED:
		if ( picomotor->driver_type != MXF_PICOMOTOR_8753_DRIVER ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Picomotor '%s' does not use a Model 8753 "
			"open-loop driver, so setting the base speed "
			"is not supported.", motor->record->name );
		}

		ulong_value = (unsigned long) mx_round( motor->raw_base_speed );

		sprintf( command, "MPV %s %d=%lu",
				picomotor->driver_name,
				picomotor->motor_number,
				ulong_value );
		
		mx_status = mxi_picomotor_command( picomotor_controller,
				command, NULL, 0, MXD_PICOMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		ulong_value = (unsigned long)
			mx_round( motor->raw_acceleration_parameters[0] );

		sprintf( command, "ACC %s %d=%lu",
				picomotor->driver_name,
				picomotor->motor_number,
				ulong_value );
		
		mx_status = mxi_picomotor_command( picomotor_controller,
				command, NULL, 0, MXD_PICOMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;

	case MXLV_MTR_MAXIMUM_SPEED:

		motor->raw_maximum_speed = 4095500;  /* steps per second */

		return MX_SUCCESSFUL_RESULT;
		break;

	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			motor->axis_enable = 1;
			sprintf( command, "MON %s", picomotor->driver_name );
		} else {
			sprintf( command, "MOF %s", picomotor->driver_name );
		}

		mx_status = mxi_picomotor_command( picomotor_controller,
				command, NULL, 0, MXD_PICOMOTOR_DEBUG );
		break;

	case MXLV_MTR_CLOSED_LOOP:
		if ( picomotor->driver_type != MXF_PICOMOTOR_8751_DRIVER ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Picomotor '%s' does not use a Model 8751 "
			"closed-loop driver, so enabling and disabling "
			"closed-loop mode is not supported.",
				motor->record->name );
		}

		if ( motor->closed_loop ) {
			motor->closed_loop = 1;
			sprintf( command, "SER %s", picomotor->driver_name );
		} else {
			sprintf( command, "NOS %s", picomotor->driver_name );
		}

		mx_status = mxi_picomotor_command( picomotor_controller,
				command, NULL, 0, MXD_PICOMOTOR_DEBUG );
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_simultaneous_start( mx_length_type num_motor_records,
				MX_RECORD **motor_record_array,
				double *position_array,
				mx_hex_type flags )
{
	static const char fname[] = "mxd_picomotor_simultaneous_start()";

	MX_RECORD *current_motor_record, *current_controller_record;
	MX_MOTOR *current_motor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	MX_PICOMOTOR_CONTROLLER *current_picomotor_controller;
	MX_PICOMOTOR *current_picomotor;
	mx_length_type i;
	char command[80];

	mx_status_type mx_status;

	picomotor_controller = NULL;

	for ( i = 0; i < num_motor_records; i++ ) {
		current_motor_record = motor_record_array[i];

		current_motor = (MX_MOTOR *)
			current_motor_record->record_class_struct;

		if ( current_motor_record->mx_type != MXT_MTR_PICOMOTOR ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Cannot perform a simultaneous start since motors "
			"'%s' and '%s' are not the same type of motors.",
				motor_record_array[0]->name,
				current_motor_record->name );
		}

		current_picomotor = (MX_PICOMOTOR *)
				current_motor_record->record_type_struct;

		current_controller_record
			= current_picomotor->picomotor_controller_record;

		current_picomotor_controller = (MX_PICOMOTOR_CONTROLLER *)
			current_controller_record->record_type_struct;

		if ( picomotor_controller == (MX_PICOMOTOR_CONTROLLER *) NULL )
		{
			picomotor_controller = current_picomotor_controller;
		}

		if ( picomotor_controller != current_picomotor_controller ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		"Cannot perform a simultaneous start for motors '%s' and '%s' "
		"since they are controlled by different Picomotor "
		"interfaces, namely '%s' and '%s'.",
				motor_record_array[0]->name,
				current_motor_record->name,
				picomotor_controller->record->name,
				current_controller_record->name );
		}
	}

	/* Start the move. */

	for ( i = 0; i < num_motor_records; i++ ) {
		current_motor_record = motor_record_array[i];

		current_motor = (MX_MOTOR *)
			current_motor_record->record_class_struct;

		current_picomotor = (MX_PICOMOTOR *)
				current_motor_record->record_type_struct;

		sprintf( command, "ABS %s=%ld",
				current_picomotor->driver_name,
				(long) current_motor->raw_destination.stepper );

		mx_status = mxi_picomotor_command( picomotor_controller,
					command, NULL, 0,
					MXD_PICOMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mxi_picomotor_command( picomotor_controller,
					"GO", NULL, 0,
					MXD_PICOMOTOR_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_picomotor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_picomotor_get_status()";

	MX_PICOMOTOR *picomotor;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char response[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char *ptr;
	unsigned char status_byte;
	int in_diagnostic_mode;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_get_pointers( motor, &picomotor,
						&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "STA %s", picomotor->driver_name );

	mx_status = mxi_picomotor_command( picomotor_controller, command,
			response, sizeof response,
			MXD_PICOMOTOR_DEBUG | MXF_PICOMOTOR_NO_STATUS_CHAR );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptr = strchr( response, '=' );

	if ( ptr == (char *) NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No equals sign seen in response to command '%s' by "
		"Picomotor '%s'.  Response = '%s'",
			command, motor->record->name, response );
	}

	ptr++;

	status_byte = (unsigned char) mx_hex_string_to_unsigned_long( ptr );

	MX_DEBUG( 2,("%s: Picomotor status byte = %#x", fname, status_byte));

	/* Check all of the status bits that we are interested in. */

	motor->status = 0;

	switch( picomotor->driver_type ) {
	case MXF_PICOMOTOR_8751_DRIVER:
		in_diagnostic_mode = FALSE;

		/* Bit 0: Move done */

		if ( ( status_byte & 0x1 ) == 0 ) {
			motor->status |= MXSF_MTR_IS_BUSY;
		}

		/* Bit 1: Checksum error */

		if ( status_byte & 0x2 ) {
			motor->status |= MXSF_MTR_ERROR;
		}

		/* Bit 2: No motor */

		if ( status_byte & 0x4 ) {
			motor->status |= MXSF_MTR_DRIVE_FAULT;
		}

		if ( in_diagnostic_mode == FALSE ) {
			/* Bit 3: Power on */

			if ( ( status_byte & 0x8 ) == 0 ) {
				motor->status |= MXSF_MTR_DRIVE_FAULT;
			}
		}

		/* Bit 4: Position error */

		if ( status_byte & 0x10 ) {
			motor->status |= MXSF_MTR_FOLLOWING_ERROR;
		}

		if ( in_diagnostic_mode == FALSE ) {
			/* Bit 5: Reverse limit */

			if ( status_byte & 0x20 ) {
				motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
			}

			/* Bit 6: Forward limit */

			if ( status_byte & 0x40 ) {
				motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
			}
		}

		/* Bit 7: Home in progress */

		if ( ( status_byte & 0x80 ) == 0 ) {
			motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
		}
		break;
	case MXF_PICOMOTOR_8753_DRIVER:
		/* Bit 0: Motor is moving */

		if ( status_byte & 0x1 ) {
			motor->status |= MXSF_MTR_IS_BUSY;
		}

		/* Bit 1: Checksum error */

		if ( status_byte & 0x2 ) {
			motor->status |= MXSF_MTR_ERROR;
		}

		/* Bit 2: Motor is on */

		if ( ( status_byte & 0x4 ) == 0 ) {
			motor->status |= MXSF_MTR_AXIS_DISABLED;
		}

		/* Bit 3: Motor selector status */

		if ( ( status_byte & 0x8 ) == 0 ) {
			motor->status |= MXSF_MTR_DRIVE_FAULT;
		}

		/* Bit 4: At commanded velocity */

		if ( ( status_byte & 0x10 ) == 0 ) {
			motor->status |= MXSF_MTR_FOLLOWING_ERROR;
		}
		 
		/* Not sure what to do with the rest of these:
		 *
		 * Bit 5: Velocity profile mode
		 * Bit 6: Position (trapezoidal) profile mode
		 * Bit 7: Reserved
		 */

		break;
	default:
		break;
	}

	MX_DEBUG( 2,("%s: MX status word = %#lx",
		fname, (unsigned long) motor->status));

	return MX_SUCCESSFUL_RESULT;
}

