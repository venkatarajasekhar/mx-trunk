/*
 * Name:    d_aviex_pccd_16080.h
 *
 * Purpose: Header file definitions that are specific to the Aviex
 *          PCCD-16080 detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AVIEX_PCCD_16080_H__
#define __D_AVIEX_PCCD_16080_H__

/*-------------------------------------------------------------*/

struct mx_aviex_pccd;

typedef struct {
	unsigned long base;

	unsigned long fpga_version;
	unsigned long control;
	unsigned long vidbin;
	unsigned long vshbin;
	unsigned long roilines;
	unsigned long nof;
	unsigned long vread;
	unsigned long vdark;
	unsigned long hlead;
	unsigned long hpix;
	unsigned long tpre;
	unsigned long shutter;
	unsigned long tpost;
	unsigned long hdark;
	unsigned long hbin;
	unsigned long roioffs;

	unsigned long spare1;
	unsigned long spare2;

	unsigned long xaoffs;
	unsigned long xboffs;
	unsigned long xcoffs;
	unsigned long xdoffs;
	unsigned long yaoffs;
	unsigned long yboffs;
	unsigned long ycoffs;
	unsigned long ydoffs;
} MX_AVIEX_PCCD_16080_DETECTOR_HEAD;

extern long mxd_aviex_pccd_16080_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_aviex_pccd_16080_rfield_def_ptr;

/*-------------------------------------------------------------*/

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_16080_read_register( struct mx_aviex_pccd *aviex_pccd,
					unsigned long register_address,
					unsigned long *register_value );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_16080_write_register( struct mx_aviex_pccd *aviex_pccd,
					unsigned long register_address,
					unsigned long register_value );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_16080_initialize_detector( MX_RECORD *,
					MX_AREA_DETECTOR *,
					struct mx_aviex_pccd *,
					MX_VIDEO_INPUT * );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_16080_set_binsize( MX_AREA_DETECTOR *, struct mx_aviex_pccd * );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_16080_descramble_raw_data( uint16_t *,
					uint16_t ***, long, long );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_16080_descramble_streak_camera( MX_AREA_DETECTOR *,
						struct mx_aviex_pccd *,
						MX_IMAGE_FRAME *,
						MX_IMAGE_FRAME * );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_16080_configure_for_sequence( MX_AREA_DETECTOR *,
						struct mx_aviex_pccd * );

/*-------------------------------------------------------------*/

#define MXLV_AVIEX_PCCD_16080_DH_FPGA_VERSION	(MXLV_AVIEX_PCCD_DH_BASE + 0)

#define MXLV_AVIEX_PCCD_16080_DH_CONTROL	(MXLV_AVIEX_PCCD_DH_BASE + 1)

#define MXLV_AVIEX_PCCD_16080_DH_VIDBIN		(MXLV_AVIEX_PCCD_DH_BASE + 2)

#define MXLV_AVIEX_PCCD_16080_DH_VSHBIN		(MXLV_AVIEX_PCCD_DH_BASE + 3)

#define MXLV_AVIEX_PCCD_16080_DH_ROILINES	(MXLV_AVIEX_PCCD_DH_BASE + 4)

#define MXLV_AVIEX_PCCD_16080_DH_NOF		(MXLV_AVIEX_PCCD_DH_BASE + 5)

#define MXLV_AVIEX_PCCD_16080_DH_VREAD		(MXLV_AVIEX_PCCD_DH_BASE + 6)

/* VREAD also uses register 7 */

#define MXLV_AVIEX_PCCD_16080_DH_VDARK		(MXLV_AVIEX_PCCD_DH_BASE + 8)

#define MXLV_AVIEX_PCCD_16080_DH_HLEAD		(MXLV_AVIEX_PCCD_DH_BASE + 9)

#define MXLV_AVIEX_PCCD_16080_DH_HPIX		(MXLV_AVIEX_PCCD_DH_BASE + 10)

/* HPIX also uses register 11 */

#define MXLV_AVIEX_PCCD_16080_DH_TPRE		(MXLV_AVIEX_PCCD_DH_BASE + 12)

/* TPRE also uses register 13 */

#define MXLV_AVIEX_PCCD_16080_DH_SHUTTER	(MXLV_AVIEX_PCCD_DH_BASE + 14)

/* SHUTTER also uses register 15 */

#define MXLV_AVIEX_PCCD_16080_DH_TPOST		(MXLV_AVIEX_PCCD_DH_BASE + 16)

/* TPOST also uses register 17 */

#define MXLV_AVIEX_PCCD_16080_DH_HDARK		(MXLV_AVIEX_PCCD_DH_BASE + 18)

#define MXLV_AVIEX_PCCD_16080_DH_HBIN		(MXLV_AVIEX_PCCD_DH_BASE + 19)

#define MXLV_AVIEX_PCCD_16080_DH_ROIOFFS	(MXLV_AVIEX_PCCD_DH_BASE + 20)

/* ROIOFFS also uses register 21 */

#define MXLV_AVIEX_PCCD_16080_DH_SPARE1		(MXLV_AVIEX_PCCD_DH_BASE + 22)

#define MXLV_AVIEX_PCCD_16080_DH_SPARE2		(MXLV_AVIEX_PCCD_DH_BASE + 23)

#define MXLV_AVIEX_PCCD_16080_DH_XAOFFS		(MXLV_AVIEX_PCCD_DH_BASE + 24)

/* XAOFFS also uses register 25 */

#define MXLV_AVIEX_PCCD_16080_DH_XBOFFS		(MXLV_AVIEX_PCCD_DH_BASE + 26)

/* XBOFFS also uses register 27 */

#define MXLV_AVIEX_PCCD_16080_DH_XCOFFS		(MXLV_AVIEX_PCCD_DH_BASE + 28)

/* XCOFFS also uses register 29 */

#define MXLV_AVIEX_PCCD_16080_DH_XDOFFS		(MXLV_AVIEX_PCCD_DH_BASE + 30)

/* XDOFFS also uses register 31 */

#define MXLV_AVIEX_PCCD_16080_DH_YAOFFS		(MXLV_AVIEX_PCCD_DH_BASE + 32)

/* YAOFFS also uses register 33 */

#define MXLV_AVIEX_PCCD_16080_DH_YBOFFS		(MXLV_AVIEX_PCCD_DH_BASE + 34)

/* YBOFFS also uses register 35 */

#define MXLV_AVIEX_PCCD_16080_DH_YCOFFS		(MXLV_AVIEX_PCCD_DH_BASE + 36)

/* YCOFFS also uses register 37 */

#define MXLV_AVIEX_PCCD_16080_DH_YDOFFS		(MXLV_AVIEX_PCCD_DH_BASE + 38)

/* YDOFFS also uses register 39 */


/*-------------------------------------------------------------*/

#define MXD_AVIEX_PCCD_16080_STANDARD_FIELDS \
  {-1, -1, "dh_base", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.base), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_FPGA_VERSION, \
		-1, "dh_fpga_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.fpga_version), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_CONTROL, \
		-1, "dh_control", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.control), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_VIDBIN, \
		-1, "dh_vidbin", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.vidbin), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_VSHBIN, \
		-1, "dh_vshbin", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.vshbin), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_ROILINES, \
		-1, "dh_roilines", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.roilines), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_NOF, \
		-1, "dh_nof", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.nof), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_VREAD, \
		-1, "dh_vread", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.vread), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_VDARK, \
		-1, "dh_vdark", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.vdark), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_HLEAD, \
		-1, "dh_hlead", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.hlead), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_HPIX, \
		-1, "dh_hpix", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.hpix), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_TPRE, \
		-1, "dh_tpre", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.tpre), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_SHUTTER, \
		-1, "dh_shutter", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.shutter), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_TPOST, \
		-1, "dh_tpost", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.tpost), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_HDARK, \
		-1, "dh_hdark", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.hdark), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_HBIN, \
		-1, "dh_hbin", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.hbin), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_ROIOFFS, \
		-1, "dh_roioffs", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.roioffs), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_SPARE1, \
		-1, "dh_spare1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.spare1), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_SPARE2, \
		-1, "dh_spare2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.spare2), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_XAOFFS, \
		-1, "dh_xaoffs", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.xaoffs), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_XBOFFS, \
		-1, "dh_xboffs", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.xboffs), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_XCOFFS, \
		-1, "dh_xcoffs", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.xcoffs), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_XDOFFS, \
		-1, "dh_xdoffs", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.xdoffs), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_YAOFFS, \
		-1, "dh_yaoffs", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.yaoffs), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_YBOFFS, \
		-1, "dh_yboffs", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.yboffs), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_YCOFFS, \
		-1, "dh_ycoffs", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.ycoffs), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_16080_DH_YDOFFS, \
		-1, "dh_ydoffs", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.ydoffs), \
	{0}, NULL, 0}

#endif /* __D_AVIEX_PCCD_16080_H__ */

