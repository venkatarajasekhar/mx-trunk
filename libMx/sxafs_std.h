/*
 * Name:    sxafs_std.h
 *
 * Purpose: Header file for standard XAFS scan.
 * 
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __SXAFS_STD_H__
#define __SXAFS_STD_H__

typedef struct {
	int dummy;
} MX_XAFS_STANDARD_SCAN;

extern long mxs_xafs_std_scan_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxs_xafs_std_scan_def_ptr;

#endif /* __SXAFS_STD_H__ */

