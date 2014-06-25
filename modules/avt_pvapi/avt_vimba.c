/*
 * Name:    avt_vimba.c
 *
 * Purpose: Module wrapper for the Vimba C API used by cameras
 *          from Allied Vision Technologies.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
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

#include "i_avt_vimba.h"
#include "d_avt_vimba.h"

MX_DRIVER avt_vimba_driver_table[] = {

{"avt_vimba", -1, MXI_CONTROLLER, MXR_INTERFACE,
		&mxi_avt_vimba_record_function_list,
		NULL,
		NULL,
		&mxi_avt_vimba_num_record_fields,
		&mxi_avt_vimba_rfield_def_ptr},

{"avt_vimba_camera", -1, MXC_VIDEO_INPUT, MXR_DEVICE,
		&mxd_avt_vimba_record_function_list,
		NULL,
		NULL,
		&mxd_avt_vimba_num_record_fields,
		&mxd_avt_vimba_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
        "avt_vimba",
        MX_VERSION,
        avt_vimba_driver_table,
        NULL,
        NULL
};

