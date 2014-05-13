/*
 * Name:    d_newport_xps.h
 *
 * Purpose: Header file for motors controlled by Newport XPS controllers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NEWPORT_XPS_H__
#define __D_NEWPORT_XPS_H__

/* Values for the axis_number variable. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *newport_xps_record;
	unsigned long axis_number;
} MX_NEWPORT_XPS_MOTOR;

MX_API mx_status_type mxd_newport_xps_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_newport_xps_open( MX_RECORD *record );

MX_API mx_status_type mxd_newport_xps_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_xps_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_newport_xps_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_newport_xps_motor_function_list;

extern long mxd_newport_xps_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_newport_xps_rfield_def_ptr;

#define MXD_NEWPORT_XPS_MOTOR_STANDARD_FIELDS \
  {-1, -1, "newport_xps_record", MXFT_RECORD, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NEWPORT_XPS_MOTOR, newport_xps_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "axis_number", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS_MOTOR, axis_number), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_NEWPORT_XPS_H__ */