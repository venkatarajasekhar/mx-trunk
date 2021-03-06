/*
 * Name:    d_mar345.h
 *
 * Purpose: MX driver header for the Mar 345 image plate detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MAR345_H__
#define __D_MAR345_H__

#define MXU_MAR345_PORT_TYPE_LENGTH	20

/* Values for the port_type field. */

#define MXT_MAR345_TCP		1
#define MXT_MAR345_SCAN345	2

typedef struct {
	MX_RECORD *record;
	long port_type;

	char port_type_name[MXU_MAR345_PORT_TYPE_LENGTH+1];
	char port_args_1[MXU_HOSTNAME_LENGTH+1];
	char port_args_2[MXU_FILENAME_LENGTH+1];
	char port_args_3[MXU_FILENAME_LENGTH+1];

	/* Parameters used by the 'tcpip' port type. */

	MX_SOCKET *mar345_socket;
	char *hostname;
	long port_number;

	/* Parameters used by the 'scan345' port type. */

	char *scan345_command;
	unsigned long scan345_process_id;

	char *command_filename;
	char *response_filename;

	FILE *command_file;
	FILE *response_file;
} MX_MAR345;

#define MXD_MAR345_STANDARD_FIELDS \
  {-1, -1, "port_type_name", MXFT_STRING, NULL, \
				1, {MXU_MAR345_PORT_TYPE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MAR345, port_type_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_args_1", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MAR345, port_args_1), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_args_2", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MAR345, port_args_2), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_args_3", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MAR345, port_args_3), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_mar345_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_mar345_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mar345_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_mar345_open( MX_RECORD *record );
MX_API mx_status_type mxd_mar345_close( MX_RECORD *record );

MX_API mx_status_type mxd_mar345_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mar345_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mar345_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mar345_get_last_frame_number( MX_AREA_DETECTOR *ad);
MX_API mx_status_type mxd_mar345_get_total_num_frames( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mar345_get_status( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mar345_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mar345_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mar345_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mar345_set_parameter( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_mar345_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_mar345_ad_function_list;

extern long mxd_mar345_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mar345_rfield_def_ptr;

#endif /* __D_MAR345_H__ */

