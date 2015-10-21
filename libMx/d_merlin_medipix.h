/*
 * Name:    d_merlin_medipix.h
 *
 * Purpose: Header file for the Merlin Medipix series of detectors
 *          from Quantum Detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MERLIN_MEDIPIX_H__
#define __D_MERLIN_MEDIPIX_H__

typedef struct {
	MX_RECORD *record;

	char hostname[MXU_HOSTNAME_LENGTH+1];
	unsigned long command_port;
	unsigned long data_port;
	mx_bool_type merlin_debug_flag;

	MX_SOCKET *command_socket;
	MX_SOCKET *data_socket;
} MX_MERLIN_MEDIPIX;


#define MXD_MERLIN_MEDIPIX_STANDARD_FIELDS \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, hostname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "command_port", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, command_port), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "data_port", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, data_port), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "merlin_debug_flag", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_MERLIN_MEDIPIX, merlin_debug_flag), \
	{0}, NULL, 0}

MX_API mx_status_type mxd_merlin_medipix_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_merlin_medipix_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_merlin_medipix_open( MX_RECORD *record );

MX_API mx_status_type mxd_merlin_medipix_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_get_extended_status(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_transfer_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_load_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_save_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_copy_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_set_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_measure_correction(
							MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_merlin_medipix_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_merlin_medipix_ad_function_list;

extern long mxd_merlin_medipix_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_merlin_medipix_rfield_def_ptr;

MX_API mx_status_type mxd_merlin_medipix_command( MX_MERLIN_MEDIPIX *pilatus,
					char *command,
					char *response,
					size_t response_buffer_length,
					unsigned long debug_flag );

#endif /* __D_MERLIN_MEDIPIX_H__ */
