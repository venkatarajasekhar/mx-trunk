/*
 * Name:    i_compumotor.h
 *
 * Purpose: Header for MX driver for Compumotor 6000 and 6K series
 *          motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2006, 2010, 2013-2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_COMPUMOTOR_H__
#define __I_COMPUMOTOR_H__

#include "mx_record.h"

#include "mx_motor.h"
#include "mx_rs232.h"

/* Flag values used by the compumotor_int driver. */

#define MXF_COMPUMOTOR_AUTO_ADDRESS_CONFIG		0x1
#define MXF_COMPUMOTOR_ECHO_ON				0x2
#define MXF_COMPUMOTOR_AUTO_COMMUNICATION_CONFIG	0x4

/* Suppress comments in debugging output. */
#define MXF_COMPUMOTOR_SUPPRESS_COMMENTS		0x1000

/*---*/

#define MXF_COMPUMOTOR_DEBUG_SERIAL			0x1000000
#define MXF_COMPUMOTOR_DEBUG_SERIAL_GETCHAR		0x2000000

/*---*/

#define MXF_COMPUMOTOR_AUTOMATIC_RESYNCHRONIZE		0x40000000
#define MXF_COMPUMOTOR_KILL_ON_STARTUP			0x80000000

/* Flag value used by mxi_compumotor_command() to prevent recursion. */

#define MXF_COMPUMOTOR_NO_RECURSION			0x2

/* Flag values used by the compumotor_lin and compumotor_trans drivers. */

#define MXF_COMPUMOTOR_SIMULTANEOUS_START		0x1

/* Define the data structures used by a Compumotor interface. */

#define MX_MAX_COMPUMOTOR_AXES		8

#define MX_COMPUMOTOR_MAX_COMMAND_LENGTH	200

#define MXT_COMPUMOTOR_UNKNOWN		0
#define MXT_COMPUMOTOR_6000_SERIES	0x1000000
#define MXT_COMPUMOTOR_6K_SERIES	0x2000000

#define MXT_COMPUMOTOR_ZETA_6000	( 0x1 | MXT_COMPUMOTOR_6000_SERIES )

#define MXT_COMPUMOTOR_6K		MXT_COMPUMOTOR_6K_SERIES

typedef struct {
	MX_RECORD *record;
	unsigned long interface_subtype;

	MX_RECORD *rs232_record;
	unsigned long interface_flags;

	long num_controllers;
	long *controller_number;
	long *num_axes;

	char startup_program[MXU_FILENAME_LENGTH+1];
	char shutdown_program[MXU_FILENAME_LENGTH+1];

	unsigned long *controller_type;
	MX_RECORD *(*motor_array)[MX_MAX_COMPUMOTOR_AXES];

	char command[MX_COMPUMOTOR_MAX_COMMAND_LENGTH+1];
	char response[MX_COMPUMOTOR_MAX_COMMAND_LENGTH+1];
} MX_COMPUMOTOR_INTERFACE;


#define MXLV_COMPUMOTOR_COMMAND			7001
#define MXLV_COMPUMOTOR_RESPONSE		7002
#define MXLV_COMPUMOTOR_COMMAND_WITH_RESPONSE	7003

#define MXI_COMPUMOTOR_INTERFACE_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR_INTERFACE, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "interface_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR_INTERFACE, interface_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "num_controllers", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR_INTERFACE, num_controllers), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "controller_number", MXFT_LONG, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR_INTERFACE, controller_number), \
	{sizeof(int)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  \
  {-1, -1, "num_axes", MXFT_LONG, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR_INTERFACE, num_axes), \
	{sizeof(int)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  \
  {-1, -1, "startup_program", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR_INTERFACE, startup_program),\
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "shutdown_program", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR_INTERFACE, shutdown_program),\
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "controller_type", MXFT_HEX, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_COMPUMOTOR_INTERFACE, controller_type), \
	{sizeof(unsigned long)}, NULL, MXFF_VARARGS }, \
  \
  {MXLV_COMPUMOTOR_COMMAND, -1, "command", MXFT_STRING,\
				NULL, 1, {MX_COMPUMOTOR_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR_INTERFACE, command), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_COMPUMOTOR_RESPONSE, -1, "response", MXFT_STRING, \
				NULL, 1, {MX_COMPUMOTOR_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR_INTERFACE, response), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_COMPUMOTOR_COMMAND_WITH_RESPONSE, -1, \
  			"command_with_response", MXFT_STRING,\
			NULL, 1, {MX_COMPUMOTOR_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_COMPUMOTOR_INTERFACE, command), \
	{sizeof(char)}, NULL, 0}

MX_API mx_status_type mxi_compumotor_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxi_compumotor_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_compumotor_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_compumotor_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_compumotor_open( MX_RECORD *record );
MX_API mx_status_type mxi_compumotor_close( MX_RECORD *record );
MX_API mx_status_type mxi_compumotor_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxi_compumotor_special_processing_setup(
						MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_compumotor_record_function_list;

extern long mxi_compumotor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_compumotor_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_compumotor_command(
	MX_COMPUMOTOR_INTERFACE *compumotor_interface,
	char *command, char *response, size_t response_buffer_length,
	int debug_flag );

MX_API mx_status_type mxi_compumotor_get_controller_index(
	MX_COMPUMOTOR_INTERFACE *compumotor_interface,
	long controller_number, long *controller_index );

MX_API mx_status_type mxi_compumotor_multiaxis_move(
	MX_COMPUMOTOR_INTERFACE *compumotor_interface, long controller_number,
	long num_motors, MX_RECORD **motor_record_array,
	double *motor_position_array, mx_bool_type simultaneous_start );

#endif /* __I_COMPUMOTOR_H__ */
