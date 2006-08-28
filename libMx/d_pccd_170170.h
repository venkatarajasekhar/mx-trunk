/*
 * Name:    d_pccd_170170.h
 *
 * Purpose: MX driver header for the Aviex PCCD-170170 CCD detector.
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

#ifndef __D_PCCD_170170_H__
#define __D_PCCD_170170_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *video_input_record;
} MX_PCCD_170170;


#define MXD_PCCD_170170_STANDARD_FIELDS \
  {-1, -1, "video_input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, video_input_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_pccd_170170_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_open( MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_close( MX_RECORD *record );

MX_API mx_status_type mxd_pccd_170170_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_busy( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_get_status( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_get_frame( MX_AREA_DETECTOR *ad,
						MX_IMAGE_FRAME **frame );
MX_API mx_status_type mxd_pccd_170170_get_sequence( MX_AREA_DETECTOR *ad,
						MX_IMAGE_SEQUENCE **sequence );
MX_API mx_status_type mxd_pccd_170170_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_set_parameter( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_pccd_170170_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_pccd_170170_ad_function_list;

extern long mxd_pccd_170170_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pccd_170170_rfield_def_ptr;

#endif /* __D_PCCD_170170_H__ */

