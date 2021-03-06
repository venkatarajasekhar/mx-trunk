/*
 * Name:    d_epics_pmac_biocat.h 
 *
 * Purpose: Header file for MX motor driver for controlling a Delta Tau
 *          PMAC motor through the BioCAT version of Tom Coleman's EPICS
 *          interface.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------
 *
 * Copyright 2011-2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPICS_PMAC_BIOCAT_H__
#define __D_EPICS_PMAC_BIOCAT_H__

/* Define values for the 'motion_state' field below. */

#define MXF_EPB_NO_MOVE_IN_PROGRESS	1
#define MXF_EPB_START_DELAY_IN_PROGRESS	2
#define MXF_EPB_MOVE_IN_PROGRESS	3
#define MXF_EPB_END_DELAY_IN_PROGRESS	4

typedef struct {
	MX_RECORD *record;

	char beamline_name[ MXU_EPICS_PVNAME_LENGTH+1 ];
	char component_name[ MXU_EPICS_PVNAME_LENGTH+1 ];
	char assembly_name[ MXU_EPICS_PVNAME_LENGTH+1 ];
	char calib_name[ MXU_EPICS_PVNAME_LENGTH+1 ];
	char dpram_name[ MXU_EPICS_PVNAME_LENGTH+1 ];
	char device_name[ MXU_EPICS_PVNAME_LENGTH+1 ];

	double start_delay;
	double end_delay;

	mx_bool_type is_raw_motor;

	long motion_state;

	MX_CLOCK_TICK start_delay_ticks;
	MX_CLOCK_TICK end_delay_ticks;

	MX_CLOCK_TICK end_of_start_delay;
	MX_CLOCK_TICK end_of_end_delay;

	MX_EPICS_PV abort_pv;
	MX_EPICS_PV actpos_pv;
	MX_EPICS_PV ampena_pv;
	MX_EPICS_PV home_pv;
	MX_EPICS_PV nglimset_pv;
	MX_EPICS_PV opnlpmd_pv;
	MX_EPICS_PV pslimset_pv;
	MX_EPICS_PV rqspos_pv;
	MX_EPICS_PV runprg_pv;
	MX_EPICS_PV strcmd_pv;
	MX_EPICS_PV strrsp_pv;

	MX_EPICS_PV ix20_fan_pv;
	MX_EPICS_PV ix30_fan_pv;

	MX_EPICS_PV ix20_li_pv;
	MX_EPICS_PV ix20_lo_pv;
	MX_EPICS_PV ix22_ai_pv;
	MX_EPICS_PV ix22_ao_pv;
	MX_EPICS_PV ix30_li_pv;
	MX_EPICS_PV ix30_lo_pv;
	MX_EPICS_PV ix31_li_pv;
	MX_EPICS_PV ix31_lo_pv;
	MX_EPICS_PV ix32_li_pv;
	MX_EPICS_PV ix32_lo_pv;
	MX_EPICS_PV ix33_li_pv;
	MX_EPICS_PV ix33_lo_pv;
	MX_EPICS_PV ix34_bi_pv;
	MX_EPICS_PV ix34_bo_pv;
	MX_EPICS_PV ix35_li_pv;
	MX_EPICS_PV ix35_lo_pv;

	/* The following is used by the 'epics_scaler_mce' driver. */

	char *epics_position_pv_ptr;

} MX_EPICS_PMAC_BIOCAT;

#define MXD_EPICS_PMAC_BIOCAT_SECONDS_PER_MILLISECOND	0.001

MX_API mx_status_type mxd_epics_pmac_biocat_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_pmac_biocat_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_epics_pmac_biocat_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_pmac_biocat_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_pmac_biocat_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_pmac_biocat_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_pmac_biocat_positive_limit_hit(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_pmac_biocat_negative_limit_hit(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_pmac_biocat_raw_home_command(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_pmac_biocat_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_pmac_biocat_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_pmac_biocat_get_status( MX_MOTOR *motor );

MX_API mx_status_type mxd_epics_pmac_biocat_command(
					MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat,
					char *command,
					char *response,
					size_t max_response_length,
					int debug_flag );

MX_API mx_status_type mxd_epics_pmac_biocat_jog_command(
					MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat,
					char *command,
					char *response,
					size_t max_response_length,
					int debug_flag );

MX_API mx_status_type mxd_epics_pmac_biocat_get_motor_variable(
					MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat,
					long variable_number,
					long variable_type,
					double *double_ptr,
					int debug_flag );

MX_API mx_status_type mxd_epics_pmac_biocat_set_motor_variable(
					MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat,
					long variable_number,
					long variable_type,
					double double_value,
					int debug_flag );

MX_API mx_status_type mxd_epics_pmac_biocat_get_motor_number(
				MX_EPICS_PMAC_BIOCAT *epics_pmac_biocat );

/*----*/

extern MX_RECORD_FUNCTION_LIST mxd_epics_pmac_biocat_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_epics_pmac_biocat_motor_function_list;

extern long mxd_epics_pmac_biocat_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_pmac_biocat_rfield_def_ptr;

#define MXD_EPICS_PMAC_BIOCAT_STANDARD_FIELDS \
  {-1, -1, "beamline_name", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_PMAC_BIOCAT, beamline_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "component_name", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_PMAC_BIOCAT, component_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "assembly_name", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_PMAC_BIOCAT, assembly_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "calib_name", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_PMAC_BIOCAT, calib_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "dpram_name", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_PMAC_BIOCAT, dpram_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "device_name", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_PMAC_BIOCAT, device_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "start_delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_PMAC_BIOCAT, start_delay), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "end_delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_PMAC_BIOCAT, end_delay), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "epics_position_pv_ptr", MXFT_STRING, NULL, 1, {1}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_EPICS_PMAC_BIOCAT, epics_position_pv_ptr), \
	{0}, NULL, MXFF_READ_ONLY}

#endif /* __D_EPICS_PMAC_BIOCAT_H__ */

