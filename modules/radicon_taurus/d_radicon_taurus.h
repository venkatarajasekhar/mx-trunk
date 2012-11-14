/*
 * Name:    d_radicon_taurus.h
 *
 * Purpose: MX driver header for the Radicon Taurus series of CMOS detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_RADICON_TAURUS_H__
#define __D_RADICON_TAURUS_H__

/* Values for the 'radicon_taurus_flags' field. */

#define MXF_RADICON_TAURUS_DO_NOT_FLIP_IMAGE	0x1
#define MXF_RADICON_TAURUS_SAVE_RAW_IMAGES	0x2

/* Values for the 'detector_model' field. */

#define MXT_RADICON_TAURUS	1
#define MXT_RADICON_XINEOS	2

#define MXU_RADICON_TAURUS_SERIAL_NUMBER_LENGTH		40

#define MXF_RADICON_TAURUS_MINIMUM_EXPOSURE_TIME	4

#define MXF_RADICON_TAURUS_CLOCK_FREQUENCY_IN_HZ	(30.0e6)

typedef struct {
	mx_bool_type raw_frame_is_saved;
} MX_RADICON_TAURUS_BUFFER_INFO;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *video_input_record;
	MX_RECORD *serial_port_record;
	unsigned long radicon_taurus_flags;
	char pulser_record_name[MXU_RECORD_NAME_LENGTH+1];
	char non_uniformity_filename[MXU_FILENAME_LENGTH+1];

	MX_RECORD *pulser_record;

	unsigned long detector_model;
	char serial_number[MXU_RADICON_TAURUS_SERIAL_NUMBER_LENGTH+1];
	unsigned long firmware_version;

	double clock_frequency;			/* in Hz */
	unsigned long linetime;			/* in ticks */
	double readout_time;			/* in seconds */
	unsigned long minimum_exposure_ticks;	/* in ticks */

	unsigned long sro_mode;
	uint64_t si1_register;
	uint64_t si2_register;
	double si1_si2_ratio;

	MX_IMAGE_FRAME *video_frame;

	mx_bool_type use_different_si2_value;

	mx_bool_type bypass_arm;
	mx_bool_type use_video_frames;
	mx_bool_type have_get_commands;
	mx_bool_type poll_pulser_status;
	mx_bool_type flip_image;

	long old_total_num_frames;
	unsigned long old_status;

	MX_CLOCK_TICK serial_delay_ticks;
	MX_CLOCK_TICK next_serial_command_tick;

	MX_IMAGE_FRAME *non_uniformity_frame;

	char raw_file_directory[MXU_FILENAME_LENGTH+1];
	char raw_file_name[MXU_FILENAME_LENGTH+1];

	unsigned long num_capture_buffers;
	MX_RADICON_TAURUS_BUFFER_INFO *buffer_info_array;

} MX_RADICON_TAURUS;

#define MXLV_RADICON_TAURUS_SRO		80000
#define MXLV_RADICON_TAURUS_SI1		80001
#define MXLV_RADICON_TAURUS_SI2		80002

#define MXD_RADICON_TAURUS_STANDARD_FIELDS \
  {-1, -1, "video_input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_TAURUS, video_input_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "serial_port_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_TAURUS, serial_port_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "radicon_taurus_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_TAURUS, radicon_taurus_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pulser_record_name", MXFT_STRING, \
				NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_TAURUS, pulser_record_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "non_uniformity_filename", MXFT_STRING, \
				NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_TAURUS, non_uniformity_filename), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "detector_model", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, detector_model), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "serial_number", MXFT_STRING, \
			NULL, 1, {MXU_RADICON_TAURUS_SERIAL_NUMBER_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, serial_number), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "firmware_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, firmware_version), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "clock_frequency", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, clock_frequency), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "linetime", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, linetime), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "readout_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, readout_time), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "minimum_exposure_ticks", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_RADICON_TAURUS, minimum_exposure_ticks), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_RADICON_TAURUS_SRO, -1, "sro", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, sro_mode), \
	{0}, NULL, 0 }, \
  \
  {MXLV_RADICON_TAURUS_SI1, -1, "si1", MXFT_UINT64, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, si1_register), \
	{0}, NULL, 0 }, \
  \
  {MXLV_RADICON_TAURUS_SI2, -1, "si2", MXFT_UINT64, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, si2_register), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "si1_si2_ratio", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, si1_si2_ratio), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "use_different_si2_value", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_TAURUS, use_different_si2_value), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "bypass_arm", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, bypass_arm), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "use_video_frames", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, use_video_frames),\
	{0}, NULL, 0 }, \
  \
  {-1, -1, "have_get_commands", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, have_get_commands), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "poll_pulser_status", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, poll_pulser_status), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "flip_image", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, flip_image), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "raw_file_directory", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, raw_file_directory), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {-1, -1, "raw_file_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_TAURUS, raw_file_name), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}

MX_API mx_status_type mxd_radicon_taurus_initialize_driver(
							MX_DRIVER *driver );
MX_API mx_status_type mxd_radicon_taurus_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_radicon_taurus_open( MX_RECORD *record );
MX_API mx_status_type mxd_radicon_taurus_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxd_radicon_taurus_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_radicon_taurus_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_get_extended_status(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_readout_frame(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_correct_frame(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_get_parameter(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_set_parameter(
						MX_AREA_DETECTOR *ad );

MX_API mx_status_type mxd_radicon_taurus_get_sro( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_set_sro( MX_AREA_DETECTOR *ad );

MX_API mx_status_type mxd_radicon_taurus_get_si1( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_set_si1( MX_AREA_DETECTOR *ad );

MX_API mx_status_type mxd_radicon_taurus_get_si2( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_taurus_set_si2( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_radicon_taurus_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_radicon_taurus_ad_function_list;

extern long mxd_radicon_taurus_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_radicon_taurus_rfield_def_ptr;

MX_API_PRIVATE
mx_status_type mxd_radicon_taurus_command( MX_RADICON_TAURUS *radicon_taurus,
						char *command,
						char *response,
						size_t max_response_length,
						mx_bool_type debug_flag );

#endif /* __D_RADICON_TAURUS_H__ */

