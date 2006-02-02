/*
 * Name:    mx_digital_input.h
 *
 * Purpose: MX header file for digital input devices.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_DIGITAL_INPUT_H__
#define __MX_DIGITAL_INPUT_H__

#include "mx_record.h"

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
                            * to this device.
                            */
	uint32_t value;
} MX_DIGITAL_INPUT;

#define MXLV_DIN_VALUE	4001

#define MX_DIGITAL_INPUT_STANDARD_FIELDS \
  {MXLV_DIN_VALUE, -1, "value", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_DIGITAL_INPUT, value), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/*
 * The structure type MX_DIGITAL_INPUT_FUNCTION_LIST contains a list of
 * pointers to functions that vary from digital input type to digital
 * input type.
 */

typedef struct {
	mx_status_type ( *read ) ( MX_DIGITAL_INPUT *dinput );
} MX_DIGITAL_INPUT_FUNCTION_LIST;

MX_API mx_status_type mx_digital_input_read( MX_RECORD *record,
						uint32_t *value );

#endif /* __MX_DIGITAL_INPUT_H__ */

