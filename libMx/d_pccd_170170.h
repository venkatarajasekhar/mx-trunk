/*
 * Name:    d_pccd_170170.h
 *
 * Purpose: MX driver header for the Aviex PCCD-170170 CCD detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PCCD_170170_H__
#define __D_PCCD_170170_H__

/* Values for 'pccd_170170_flags'. */

#define MXF_PCCD_170170_SUPPRESS_DESCRAMBLING		0x1
#define MXF_PCCD_170170_USE_DETECTOR_HEAD_SIMULATOR	0x2

#define MXF_PCCD_170170_CAMERA_IS_MASTER		0x8
#define MXF_PCCD_170170_USE_TEST_PATTERN		0x10

#define MXF_PCCD_170170_TEST_DEZINGER			0x100

#if 0
/* Scale factors for converting raw frame dimensions
 * into user frame dimensions.
 */

#define MXF_PCCD_170170_HORIZ_SCALE	4
#define MXF_PCCD_170170_VERT_SCALE	4

#endif

/*---*/

#define MXF_PCCD_170170_MAXIMUM_DETECTOR_HEAD_FRAMES	255

/*-------------------------------------------------------------*/

/* Control register bit definitions. */

/* Test mode (bit 0) */

#define MXF_PCCD_170170_TEST_MODE_ON			0x1

/* Low noise-high speed (bit 1) */

#define MXF_PCCD_170170_HIGH_SPEED			0x2

/* Automatic offset correction (bit 2) */

#define MXF_PCCD_170170_AUTOMATIC_OFFSET_CORRECTION_ON	0x4

/* Internal/External trigger mode (bit 3) */

#define MXF_PCCD_170170_EXTERNAL_TRIGGER		0x8

/* Edge or duration trigger (bit 4) */

#define MXF_PCCD_170170_EXTERNAL_DURATION_TRIGGER	0x10

/* Detector readout mode (bits 5-6) */

#define MXF_PCCD_170170_DETECTOR_READOUT_MASK		0x60

/*---- Full frame is both bits 0. */

#define MXF_PCCD_170170_SUBIMAGE_MODE			0x20
#define MXF_PCCD_170170_STREAK_CAMERA_MODE		0x60

/* Linearization (bit 7) */

#define MXF_PCCD_170170_LINEARIZATION_ON		0x80

/* Offset correction mode (bit 8) */

#define MXF_PCCD_170170_UNBINNED_PIXEL_AVERAGING	0x100

/* Dummy frame valid pulse (bit 9).
 *
 * The dummy frame valid pulse is to work around a "feature"
 * of the EPIX XCLIB cameras which ignore the first Frame Valid
 * signal sent to them by default.
 */

#define MXF_PCCD_170170_DUMMY_FRAME_VALID		0x200

/* Shutter disable (bit 10). */

#define MXF_PCCD_170170_SHUTTER_DISABLE			0x400

/* Over-exposure warning (bit 11). */

#define MXF_PCCD_170170_OVER_EXPOSURE_WARNING		0x800

/*-------------------------------------------------------------*/

typedef struct {
	unsigned long value;
	mx_bool_type read_only;
	mx_bool_type power_of_two;
	unsigned long minimum;
	unsigned long maximum;
} MX_PCCD_170170_REGISTER;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *video_input_record;
	MX_RECORD *camera_link_record;
	MX_RECORD *internal_trigger_record;
	unsigned long initial_trigger_mode;
	unsigned long pccd_170170_flags;
	char geometrical_spline_filename[MXU_FILENAME_LENGTH+1];
	char geometrical_mask_filename[MXU_FILENAME_LENGTH+1];

	mx_bool_type buffer_overrun;
	mx_bool_type use_top_half_of_detector;

	mx_bool_type first_dh_command;

	long vinput_normal_framesize[2];
	long normal_binsize[2];

	MX_IMAGE_FRAME *raw_frame;

	MX_IMAGE_FRAME *temp_frame;

	long old_framesize[2];

	long horiz_descramble_factor;
	long vert_descramble_factor;
	double pixel_clock_frequency;

	uint16_t ***sector_array;

	long num_registers;
	MX_PCCD_170170_REGISTER *register_array;

	unsigned long dh_base;

	unsigned long dh_control;
	unsigned long dh_overscanned_pixels_per_line;
	unsigned long dh_physical_lines_in_quadrant;
	unsigned long dh_physical_pixels_in_quadrant;

	unsigned long dh_lines_read_in_quadrant;
	unsigned long dh_pixels_read_in_quadrant;

	unsigned long dh_initial_delay_time;
	unsigned long dh_exposure_time;
	unsigned long dh_readout_delay_time;
	unsigned long dh_frames_per_sequence;
	unsigned long dh_gap_time;
	unsigned long dh_exposure_multiplier;
	unsigned long dh_gap_multiplier;
	unsigned long dh_controller_fpga_version;

	unsigned long dh_line_binning;
	unsigned long dh_pixel_binning;
	unsigned long dh_subframe_size;
	unsigned long dh_subimages_per_read;
	unsigned long dh_streak_mode_lines;

	unsigned long dh_comm_fpga_version;

	unsigned long dh_offset_a1;
	unsigned long dh_offset_a2;
	unsigned long dh_offset_a3;
	unsigned long dh_offset_a4;
	unsigned long dh_offset_b1;
	unsigned long dh_offset_b2;
	unsigned long dh_offset_b3;
	unsigned long dh_offset_b4;
	unsigned long dh_offset_c1;
	unsigned long dh_offset_c2;
	unsigned long dh_offset_c3;
	unsigned long dh_offset_c4;
	unsigned long dh_offset_d1;
	unsigned long dh_offset_d2;
	unsigned long dh_offset_d3;
	unsigned long dh_offset_d4;

	unsigned long dh_detector_readout_mode;
	unsigned long dh_readout_speed;
	unsigned long dh_test_mode;
	unsigned long dh_offset_correction;
	unsigned long dh_exposure_mode;
	unsigned long dh_linearization;
	unsigned long dh_dummy_frame_valid;
	unsigned long dh_shutter_disable;
	unsigned long dh_over_exposure_warning;
} MX_PCCD_170170;

#define MXLV_PCCD_170170_DH_BASE		100000

#define MXLV_PCCD_170170_DH_CONTROL 		(MXLV_PCCD_170170_DH_BASE + 0)

#define MXLV_PCCD_170170_DH_OVERSCANNED_PIXELS_PER_LINE \
				 		(MXLV_PCCD_170170_DH_BASE + 1)

#define MXLV_PCCD_170170_DH_PHYSICAL_LINES_IN_QUADRANT \
				 		(MXLV_PCCD_170170_DH_BASE + 2)

#define MXLV_PCCD_170170_DH_PHYSICAL_PIXELS_IN_QUADRANT \
				 		(MXLV_PCCD_170170_DH_BASE + 3)


#define MXLV_PCCD_170170_DH_LINES_READ_IN_QUADRANT \
				 		(MXLV_PCCD_170170_DH_BASE + 5)

#define MXLV_PCCD_170170_DH_PIXELS_READ_IN_QUADRANT \
				 		(MXLV_PCCD_170170_DH_BASE + 6)


#define MXLV_PCCD_170170_DH_INITIAL_DELAY_TIME	(MXLV_PCCD_170170_DH_BASE + 9)

#define MXLV_PCCD_170170_DH_EXPOSURE_TIME	(MXLV_PCCD_170170_DH_BASE + 10)

#define MXLV_PCCD_170170_DH_READOUT_DELAY_TIME  (MXLV_PCCD_170170_DH_BASE + 11)

#define MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE (MXLV_PCCD_170170_DH_BASE + 12)

#define MXLV_PCCD_170170_DH_GAP_TIME		(MXLV_PCCD_170170_DH_BASE + 13)

#define MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER	(MXLV_PCCD_170170_DH_BASE + 14)

#define MXLV_PCCD_170170_DH_GAP_MULTIPLIER	(MXLV_PCCD_170170_DH_BASE + 15)

#define MXLV_PCCD_170170_DH_CONTROLLER_FPGA_VERSION \
						(MXLV_PCCD_170170_DH_BASE + 16)


#define MXLV_PCCD_170170_DH_LINE_BINNING	(MXLV_PCCD_170170_DH_BASE + 18)

#define MXLV_PCCD_170170_DH_PIXEL_BINNING	(MXLV_PCCD_170170_DH_BASE + 19)

#define MXLV_PCCD_170170_DH_SUBFRAME_SIZE	(MXLV_PCCD_170170_DH_BASE + 20)

#define MXLV_PCCD_170170_DH_SUBIMAGES_PER_READ	(MXLV_PCCD_170170_DH_BASE + 21)

#define MXLV_PCCD_170170_DH_STREAK_MODE_LINES	(MXLV_PCCD_170170_DH_BASE + 22)

#define MXLV_PCCD_170170_DH_OFFSET_A1		(MXLV_PCCD_170170_DH_BASE + 124)
#define MXLV_PCCD_170170_DH_OFFSET_A2		(MXLV_PCCD_170170_DH_BASE + 125)
#define MXLV_PCCD_170170_DH_OFFSET_A3		(MXLV_PCCD_170170_DH_BASE + 126)
#define MXLV_PCCD_170170_DH_OFFSET_A4		(MXLV_PCCD_170170_DH_BASE + 127)
#define MXLV_PCCD_170170_DH_OFFSET_B1		(MXLV_PCCD_170170_DH_BASE + 128)
#define MXLV_PCCD_170170_DH_OFFSET_B2		(MXLV_PCCD_170170_DH_BASE + 129)
#define MXLV_PCCD_170170_DH_OFFSET_B3		(MXLV_PCCD_170170_DH_BASE + 130)
#define MXLV_PCCD_170170_DH_OFFSET_B4		(MXLV_PCCD_170170_DH_BASE + 131)
#define MXLV_PCCD_170170_DH_OFFSET_C1		(MXLV_PCCD_170170_DH_BASE + 132)
#define MXLV_PCCD_170170_DH_OFFSET_C2		(MXLV_PCCD_170170_DH_BASE + 133)
#define MXLV_PCCD_170170_DH_OFFSET_C3		(MXLV_PCCD_170170_DH_BASE + 134)
#define MXLV_PCCD_170170_DH_OFFSET_C4		(MXLV_PCCD_170170_DH_BASE + 135)
#define MXLV_PCCD_170170_DH_OFFSET_D1		(MXLV_PCCD_170170_DH_BASE + 136)
#define MXLV_PCCD_170170_DH_OFFSET_D2		(MXLV_PCCD_170170_DH_BASE + 137)
#define MXLV_PCCD_170170_DH_OFFSET_D3		(MXLV_PCCD_170170_DH_BASE + 138)
#define MXLV_PCCD_170170_DH_OFFSET_D4		(MXLV_PCCD_170170_DH_BASE + 139)

#define MXLV_PCCD_170170_DH_COMM_FPGA_VERSION	(MXLV_PCCD_170170_DH_BASE + 116)


#define MX_PCCD_170170_NUM_REGISTERS \
	(MXLV_PCCD_170170_DH_OFFSET_D4 - MXLV_PCCD_170170_DH_BASE + 1)

/* Define some pseudo registers to manipulate individual bits
 * in the control register.
 */

#define MXLV_PCCD_170170_DH_PSEUDO_BASE			200000

#define MXLV_PCCD_170170_DH_DETECTOR_READOUT_MODE	200000
#define MXLV_PCCD_170170_DH_READOUT_SPEED		200001
#define MXLV_PCCD_170170_DH_TEST_MODE			200002
#define MXLV_PCCD_170170_DH_OFFSET_CORRECTION		200003
#define MXLV_PCCD_170170_DH_EXPOSURE_MODE		200004
#define MXLV_PCCD_170170_DH_LINEARIZATION		200005
#define MXLV_PCCD_170170_DH_DUMMY_FRAME_VALID		200006
#define MXLV_PCCD_170170_DH_SHUTTER_DISABLE		200007
#define MXLV_PCCD_170170_DH_OVER_EXPOSURE_WARNING	200008

#define MXD_PCCD_170170_STANDARD_FIELDS \
  {-1, -1, "video_input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, video_input_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "camera_link_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, camera_link_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "internal_trigger_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, internal_trigger_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "initial_trigger_mode", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, initial_trigger_mode), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pccd_170170_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, pccd_170170_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "geometrical_spline_filename", MXFT_STRING, NULL, \
  			1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_PCCD_170170, geometrical_spline_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "geometrical_mask_filename", MXFT_STRING, NULL, \
  			1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_PCCD_170170, geometrical_mask_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  \
  {-1, -1, "buffer_overrun", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, buffer_overrun), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "use_top_half_of_detector", MXFT_BOOL, NULL, 0, {0}, \
      MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, use_top_half_of_detector), \
	{0}, NULL, 0}, \
  \
  \
  {-1, -1, "dh_base", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_base), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_CONTROL, \
		-1, "dh_control", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_control), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OVERSCANNED_PIXELS_PER_LINE, \
	    -1, "dh_overscanned_pixels_per_line", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_overscanned_pixels_per_line), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_PHYSICAL_LINES_IN_QUADRANT, \
		-1, "dh_physical_lines_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_physical_lines_in_quadrant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_PHYSICAL_PIXELS_IN_QUADRANT, \
		-1, "dh_physical_pixels_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_physical_pixels_in_quadrant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_LINES_READ_IN_QUADRANT, \
		-1, "dh_lines_read_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_lines_read_in_quadrant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_PIXELS_READ_IN_QUADRANT, \
		-1, "dh_pixels_read_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_pixels_read_in_quadrant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_INITIAL_DELAY_TIME, \
		-1, "dh_initial_delay_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_initial_delay_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_EXPOSURE_TIME, \
		-1, "dh_exposure_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_exposure_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_READOUT_DELAY_TIME, \
		-1, "dh_readout_delay_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_readout_delay_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE, \
		-1, "dh_frames_per_sequence", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_frames_per_sequence), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_GAP_TIME, \
		-1, "dh_gap_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_gap_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER, \
		-1, "dh_exposure_multiplier", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_exposure_multiplier), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_GAP_MULTIPLIER, \
		-1, "dh_gap_multiplier", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_gap_multiplier), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_CONTROLLER_FPGA_VERSION, \
		-1, "dh_controller_fpga_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_controller_fpga_version), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_LINE_BINNING, \
		-1, "dh_line_binning", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_line_binning), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_PIXEL_BINNING, \
		-1, "dh_pixel_binning", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_pixel_binning), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_SUBFRAME_SIZE, \
		-1, "dh_subframe_size", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_subframe_size), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_SUBIMAGES_PER_READ, \
		-1, "dh_subimages_per_read", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_subimages_per_read), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_STREAK_MODE_LINES, \
		-1, "dh_streak_mode_lines", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_streak_mode_lines), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_A1, \
		-1, "dh_offset_a1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_a1), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_A2, \
		-1, "dh_offset_a2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_a2), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_A3, \
		-1, "dh_offset_a3", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_a3), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_A4, \
		-1, "dh_offset_a4", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_a4), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_B1, \
		-1, "dh_offset_b1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_b1), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_B2, \
		-1, "dh_offset_b2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_b2), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_B3, \
		-1, "dh_offset_b3", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_b3), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_B4, \
		-1, "dh_offset_b4", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_b4), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_C1, \
		-1, "dh_offset_c1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_c1), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_C2, \
		-1, "dh_offset_c2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_c2), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_C3, \
		-1, "dh_offset_c3", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_c3), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_C4, \
		-1, "dh_offset_c4", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_c4), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_D1, \
		-1, "dh_offset_d1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_d1), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_D2, \
		-1, "dh_offset_d2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_d2), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_D3, \
		-1, "dh_offset_d3", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_d3), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_D4, \
		-1, "dh_offset_d4", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_d4), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_COMM_FPGA_VERSION, \
		-1, "dh_comm_fpga_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_comm_fpga_version), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  \
  {MXLV_PCCD_170170_DH_DETECTOR_READOUT_MODE, \
  		-1, "dh_detector_readout_mode", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_detector_readout_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_READOUT_SPEED, \
  		-1, "dh_readout_speed", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_readout_speed), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_TEST_MODE, \
  		-1, "dh_test_mode", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_test_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_CORRECTION, \
  		-1, "dh_offset_correction", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_correction), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_EXPOSURE_MODE, \
  		-1, "dh_exposure_mode", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_exposure_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_LINEARIZATION, \
  		-1, "dh_linearization", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_linearization), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_PCCD_170170_DH_DUMMY_FRAME_VALID, \
  		-1, "dh_dummy_frame_valid", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_dummy_frame_valid), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_SHUTTER_DISABLE, \
  		-1, "dh_shutter_disable", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_shutter_disable), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OVER_EXPOSURE_WARNING, \
  		-1, "dh_over_exposure_warning", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_over_exposure_warning), \
	{0}, NULL, 0}

MX_API mx_status_type mxd_pccd_170170_initialize_type( long record_type );
MX_API mx_status_type mxd_pccd_170170_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_open( MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_close( MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_pccd_170170_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_get_extended_status(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_set_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_geometrical_correction(
						MX_AREA_DETECTOR *ad );

MX_API mx_status_type mxd_pccd_170170_camera_link_command(
					MX_PCCD_170170 *pccd_170170,
					char *command,
					char *response,
					size_t max_response_length,
					int debug_flag );

MX_API mx_status_type mxd_pccd_170170_read_register(
					MX_PCCD_170170 *pccd_170170,
					unsigned long register_address,
					unsigned long *register_value );

MX_API mx_status_type mxd_pccd_170170_write_register(
					MX_PCCD_170170 *pccd_170170,
					unsigned long register_address,
					unsigned long register_value );

extern MX_RECORD_FUNCTION_LIST mxd_pccd_170170_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_pccd_170170_ad_function_list;

extern long mxd_pccd_170170_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pccd_170170_rfield_def_ptr;

extern long mxd_pccd_4824_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pccd_4824_rfield_def_ptr;

extern long mxd_pccd_16080_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pccd_16080_rfield_def_ptr;

#endif /* __D_PCCD_170170_H__ */

