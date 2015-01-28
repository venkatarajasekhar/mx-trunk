/*
 * Name:    d_network_mcs.h 
 *
 * Purpose: Header file to support MX network multichannel scalers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003-2004, 2010, 2014-2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_MCS_H__
#define __D_NETWORK_MCS_H__

#include "mx_mcs.h"

/* ===== MX network mcs record data structures ===== */

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD busy_nf;
	MX_NETWORK_FIELD clear_nf;
	MX_NETWORK_FIELD current_num_measurements_nf;
	MX_NETWORK_FIELD current_num_scalers_nf;
	MX_NETWORK_FIELD dark_current_nf;
	MX_NETWORK_FIELD dark_current_array_nf;
	MX_NETWORK_FIELD external_channel_advance_nf;
	MX_NETWORK_FIELD external_prescale_nf;
	MX_NETWORK_FIELD measurement_counts_nf;
	MX_NETWORK_FIELD measurement_data_nf;
	MX_NETWORK_FIELD measurement_index_nf;
	MX_NETWORK_FIELD measurement_number_nf;
	MX_NETWORK_FIELD measurement_time_nf;
	MX_NETWORK_FIELD mode_nf;
	MX_NETWORK_FIELD readout_preference_nf;
	MX_NETWORK_FIELD scaler_data_nf;
	MX_NETWORK_FIELD scaler_index_nf;
	MX_NETWORK_FIELD start_nf;
	MX_NETWORK_FIELD stop_nf;
	MX_NETWORK_FIELD timer_data_nf;
	MX_NETWORK_FIELD timer_name_nf;
} MX_NETWORK_MCS;

#define MXD_NETWORK_MCS_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_NETWORK_MCS, server_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_MCS, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

/* Define all of the interface functions. */

MX_API mx_status_type mxd_network_mcs_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_network_mcs_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_mcs_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_mcs_print_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_network_mcs_open( MX_RECORD *record );

MX_API mx_status_type mxd_network_mcs_start( MX_MCS *mcs );
MX_API mx_status_type mxd_network_mcs_stop( MX_MCS *mcs );
MX_API mx_status_type mxd_network_mcs_clear( MX_MCS *mcs );
MX_API mx_status_type mxd_network_mcs_busy( MX_MCS *mcs );
MX_API mx_status_type mxd_network_mcs_read_all( MX_MCS *mcs );
MX_API mx_status_type mxd_network_mcs_read_scaler( MX_MCS *mcs );
MX_API mx_status_type mxd_network_mcs_read_measurement( MX_MCS *mcs );
MX_API mx_status_type mxd_network_mcs_read_timer( MX_MCS *mcs );
MX_API mx_status_type mxd_network_mcs_get_parameter( MX_MCS *mcs );
MX_API mx_status_type mxd_network_mcs_set_parameter( MX_MCS *mcs );

extern MX_RECORD_FUNCTION_LIST mxd_network_mcs_record_function_list;
extern MX_MCS_FUNCTION_LIST mxd_network_mcs_mcs_function_list;

extern long mxd_network_mcs_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_mcs_rfield_def_ptr;

#endif /* __D_NETWORK_MCS_H__ */

