/*
 * Name:    d_icplus_dio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          Oxford Danfysik IC PLUS digital I/O ports.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ICPLUS_DIO_H__
#define __D_ICPLUS_DIO_H__

#include "mx_digital_input.h"
#include "mx_digital_output.h"

/* ===== IC PLUS digital input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *icplus_record;
	int32_t port_number;
} MX_ICPLUS_DINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *icplus_record;
	int32_t port_number;
} MX_ICPLUS_DOUTPUT;

#define MXD_ICPLUS_DINPUT_STANDARD_FIELDS \
  {-1, -1, "icplus_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS_DINPUT, icplus_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_number", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS_DINPUT, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_ICPLUS_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "icplus_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS_DOUTPUT, icplus_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_number", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ICPLUS_DOUTPUT, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_icplus_din_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_icplus_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_icplus_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_icplus_din_digital_input_function_list;

extern mx_length_type mxd_icplus_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_icplus_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_icplus_dout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_icplus_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_icplus_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_icplus_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_icplus_dout_digital_output_function_list;

extern mx_length_type mxd_icplus_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_icplus_dout_rfield_def_ptr;

#endif /* __D_ICPLUS_DIO_H__ */

