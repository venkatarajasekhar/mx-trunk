/*
 * Name:    d_v4l2_input.h
 *
 * Purpose: MX driver header for Video4Linux2 video input.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_V4L2_INPUT_H__
#define __D_V4L2_INPUT_H__

typedef struct {
	MX_RECORD *record;

	char device_name[MXU_FILENAME_LENGTH+1];
	long input_number;

	int fd;		/* File descriptor for the video device. */
	int num_inputs;
} MX_V4L2_INPUT;

#define MXD_V4L2_INPUT_STANDARD_FIELDS \
  {-1, -1, "device_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_V4L2_INPUT, device_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "input_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_V4L2_INPUT, input_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_v4l2_input_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_v4l2_input_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_v4l2_input_open( MX_RECORD *record );
MX_API mx_status_type mxd_v4l2_input_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxd_v4l2_input_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST mxd_v4l2_input_video_input_function_list;

extern long mxd_v4l2_input_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_v4l2_input_rfield_def_ptr;

#endif /* __D_V4L2_INPUT_H__ */

