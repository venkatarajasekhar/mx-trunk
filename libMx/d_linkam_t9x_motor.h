/*
 * Name:    d_linkam_t9x_motor.h
 *
 * Purpose: Header file for the motion control part of
 *          Linkam T9x series cooling system controllers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_LINKAM_T9X_MOTOR_H__
#define __D_LINKAM_T9X_MOTOR_H__

/* ============ Motor channels ============ */

typedef struct {
	MX_RECORD *record;
	MX_RECORD *linkam_t9x_record;
	char axis_name;
} MX_LINKAM_T9X_MOTOR;

MX_API mx_status_type mxd_linkam_t9x_motor_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_linkam_t9x_motor_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_linkam_t9x_motor_get_position( MX_MOTOR *motor);
MX_API mx_status_type mxd_linkam_t9x_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_linkam_t9x_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_linkam_t9x_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_linkam_t9x_motor_find_home_position( MX_MOTOR *motor);
MX_API mx_status_type mxd_linkam_t9x_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_linkam_t9x_motor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_linkam_t9x_motor_get_status( MX_MOTOR *motor);

extern MX_RECORD_FUNCTION_LIST mxd_linkam_t9x_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_linkam_t9x_motor_motor_function_list;

extern long mxd_linkam_t9x_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_linkam_t9x_motor_rfield_def_ptr;

#define MXD_LINKAM_T9X_MOTOR_STANDARD_FIELDS \
  {-1, -1, "linkam_t9x_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINKAM_T9X_MOTOR, linkam_t9x_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "axis_name", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINKAM_T9X_MOTOR, axis_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* === Driver specific functions === */

#endif /* __D_LINKAM_T9X_MOTOR_H__ */

