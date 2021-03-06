/*
 * Name: xineos_gige.c
 *
 * Purpose: Module wrapper for the DALSA Xineos GigE series of detectors.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2013, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_module.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_area_detector.h"
#include "d_xineos_gige.h"

MX_DRIVER xineos_gige_driver_table[] = {

{"xineos_gige", -1,	MXC_AREA_DETECTOR, MXR_DEVICE,
				&mxd_xineos_gige_record_function_list,
				NULL,
				NULL,
				&mxd_xineos_gige_num_record_fields,
				&mxd_xineos_gige_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"xineos_gige",
	MX_VERSION,
	xineos_gige_driver_table,
	NULL,
	NULL
};

