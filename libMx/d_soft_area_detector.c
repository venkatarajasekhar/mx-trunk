/*
 * Name:    d_soft_area_detector.c
 *
 * Purpose: MX driver for a software emulated area detector.  It uses an
 *          MX video input record as the source of the frame images.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SOFT_AREA_DETECTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "mx_motor.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_area_detector.h"
#include "d_soft_area_detector.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_soft_area_detector_record_function_list = {
	mxd_soft_area_detector_initialize_driver,
	mxd_soft_area_detector_create_record_structures,
	mx_area_detector_finish_record_initialization,
	NULL,
	NULL,
	mxd_soft_area_detector_open
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_soft_area_detector_ad_function_list = {
	mxd_soft_area_detector_arm,
	mxd_soft_area_detector_trigger,
	NULL,
	mxd_soft_area_detector_stop,
	mxd_soft_area_detector_abort,
	NULL,
	NULL,
	NULL,
	mxd_soft_area_detector_get_extended_status,
	mxd_soft_area_detector_readout_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_soft_area_detector_get_parameter,
	mxd_soft_area_detector_set_parameter,
	NULL,
	NULL,
	NULL,
	mx_area_detector_trigger_unsafe_oscillation
};

MX_RECORD_FIELD_DEFAULTS mxd_soft_area_detector_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_SOFT_AREA_DETECTOR_STANDARD_FIELDS
};

long mxd_soft_area_detector_num_record_fields
		= sizeof( mxd_soft_area_detector_rf_defaults )
		/ sizeof( mxd_soft_area_detector_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_area_detector_rfield_def_ptr
			= &mxd_soft_area_detector_rf_defaults[0];

/*---*/

static mx_status_type
mxd_soft_area_detector_get_pointers( MX_AREA_DETECTOR *ad,
			MX_SOFT_AREA_DETECTOR **soft_area_detector,
			const char *calling_fname )
{
	static const char fname[] = "mxd_soft_area_detector_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (soft_area_detector == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_SOFT_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*soft_area_detector = (MX_SOFT_AREA_DETECTOR *)
				ad->record->record_type_struct;

	if ( *soft_area_detector == (MX_SOFT_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_SOFT_AREA_DETECTOR pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_soft_area_detector_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_soft_area_detector_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_SOFT_AREA_DETECTOR *soft_area_detector;

	/* Use calloc() here instead of malloc(), so that a bunch of
	 * fields that are never touched will be initialized to 0.
	 */

	ad = (MX_AREA_DETECTOR *) calloc( 1, sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	soft_area_detector = (MX_SOFT_AREA_DETECTOR *)
				calloc( 1, sizeof(MX_SOFT_AREA_DETECTOR) );

	if ( soft_area_detector == (MX_SOFT_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_SOFT_AREA_DETECTOR structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = soft_area_detector;
	record->class_specific_function_list = 
			&mxd_soft_area_detector_ad_function_list;

	ad->record = record;
	soft_area_detector->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_area_detector_open()";

	MX_AREA_DETECTOR *ad;
	MX_SOFT_AREA_DETECTOR *soft_area_detector = NULL;
	MX_RECORD *video_input_record;
	long i;
	unsigned long mask;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	video_input_record = soft_area_detector->video_input_record;

	ad->header_length = 0;

	ad->sequence_parameters.sequence_type = MXT_SQ_ONE_SHOT;
	ad->sequence_parameters.num_parameters = 1;
	ad->sequence_parameters.parameter_array[0] = 1.0;

	/* Set the default file formats. */

	ad->datafile_load_format   = MXT_IMAGE_FILE_SMV;
	ad->datafile_save_format   = MXT_IMAGE_FILE_SMV;
	ad->correction_load_format = MXT_IMAGE_FILE_SMV;
	ad->correction_save_format = MXT_IMAGE_FILE_SMV;

	/* Set the maximum framesize to the initial framesize of the
	 * video input.
	 */

	mx_status = mx_video_input_get_framesize( video_input_record,
					&(ad->maximum_framesize[0]),
					&(ad->maximum_framesize[1]) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->framesize[0] = ad->maximum_framesize[0];
	ad->framesize[1] = ad->maximum_framesize[1];

	ad->binsize[0] = 1;
	ad->binsize[1] = 1;

	/* Copy other needed parameters from the video input record to
	 * the area detector record.
	 */

	mx_status = mx_video_input_get_image_format( video_input_record,
							&(ad->image_format) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_image_get_image_format_name_from_type(
						ad->image_format,
						ad->image_format_name,
						sizeof(ad->image_format_name) );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_byte_order( video_input_record,
							&(ad->byte_order) );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_pixel( video_input_record,
							&(ad->bytes_per_pixel));
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bits_per_pixel( video_input_record,
							&(ad->bits_per_pixel));
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_frame( video_input_record,
							&(ad->bytes_per_frame));
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the video input's initial trigger mode (internal/external/etc) */

	mx_status = mx_video_input_set_trigger_mode( video_input_record,
			(long) soft_area_detector->initial_trigger_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Zero out the ROI boundaries. */

	for ( i = 0; i < ad->maximum_num_rois; i++ ) {
		ad->roi_array[i][0] = 0;
		ad->roi_array[i][1] = 0;
		ad->roi_array[i][2] = 0;
		ad->roi_array[i][3] = 0;
	}

	/* Load the image correction files. */

	mx_status = mx_area_detector_load_correction_files( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure automatic saving or readout of image frames. */

	mask = MXF_AD_SAVE_FRAME_AFTER_ACQUISITION
		| MXF_AD_READOUT_FRAME_AFTER_ACQUISITION;

	if ( ad->area_detector_flags & mask ) {
		mx_status =
		    mx_area_detector_setup_datafile_management( record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_arm()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	mx_status = mx_video_input_arm(soft_area_detector->video_input_record);

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_trigger()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	mx_status = mx_video_input_trigger(
			soft_area_detector->video_input_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_stop()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_stop(soft_area_detector->video_input_record);

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_abort()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_abort(
			soft_area_detector->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_soft_area_detector_get_extended_status()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector = NULL;
	long vinput_last_frame_number;
	long vinput_total_num_frames;
	unsigned long vinput_status;
	mx_status_type mx_status;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mx_video_input_get_extended_status(
				soft_area_detector->video_input_record,
				&vinput_last_frame_number,
				&vinput_total_num_frames,
				&vinput_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->last_frame_number = vinput_last_frame_number;
	ad->total_num_frames = vinput_total_num_frames;

	ad->status = 0;

	if ( vinput_status & MXSF_VIN_IS_BUSY ) {

		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: last_frame_number = %ld, "
	"total_num_frames = %ld, status = %#lx",
		fname, ad->last_frame_number,
		ad->total_num_frames, ad->status));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_readout_frame()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mx_video_input_get_frame(
		soft_area_detector->video_input_record,
		ad->readout_frame, &(ad->image_frame) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_get_parameter()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector = NULL;
	MX_RECORD *video_input_record;
	mx_status_type mx_status;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif
	video_input_record = soft_area_detector->video_input_record;

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
		mx_status = mx_video_input_get_framesize( video_input_record,
				&(ad->framesize[0]), &(ad->framesize[1]) );
		break;
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		mx_status = mx_video_input_get_image_format( video_input_record,
						&(ad->image_format) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_image_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		mx_status = mx_video_input_get_bytes_per_frame(
				video_input_record, &(ad->bytes_per_frame) );
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
		mx_status = mx_video_input_get_bytes_per_pixel(
				video_input_record, &(ad->bytes_per_pixel) );
		break;

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_video_input_get_trigger_mode(
				video_input_record, &(ad->trigger_mode) );
		break;

	case MXLV_AD_BITS_PER_PIXEL:
		mx_status = mx_video_input_get_bits_per_pixel(
				video_input_record, &(ad->bits_per_pixel) );
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		break;

	default:
		mx_status = mx_area_detector_default_get_parameter_handler(ad);
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_set_parameter()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector = NULL;
	MX_RECORD *video_input_record;
	mx_status_type mx_status;

	static long allowed_binsize[] = { 1, 2, 4, 8, 16, 32, 64 };

	static int num_allowed_binsizes = sizeof( allowed_binsize )
						/ sizeof( allowed_binsize[0] );

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif
	video_input_record = soft_area_detector->video_input_record;

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
		mx_status = mx_area_detector_compute_new_binning( ad,
							ad->parameter_type,
							num_allowed_binsizes,
							allowed_binsize );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Tell the video input to change its framesize. */

		mx_status = mx_video_input_set_framesize(
					video_input_record,
					ad->framesize[0], ad->framesize[1] );
		break;

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_video_input_set_trigger_mode(
				video_input_record, ad->trigger_mode );
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		mx_status = mx_video_input_set_sequence_parameters(
					video_input_record,
					&(ad->sequence_parameters) );
		break; 

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Changing parameter '%s' for area detector '%s' is not supported.",
			mx_get_field_label_string( ad->record,
				ad->parameter_type ), ad->record->name );
	default:
		mx_status = mx_area_detector_default_set_parameter_handler(ad);
		break;
	}

	return mx_status;
}

