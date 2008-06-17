/*
 * Name:    i_spellman_hvps.h
 *
 * Purpose: Header file for the Spellman high voltage power supply 
 *          for X-ray generators.
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

#ifndef __I_SPELLMAN_HVPS_H__
#define __I_SPELLMAN_HVPS_H__

/* Analog control signals */

#define MXF_SPELLMAN_HVPS_VOLTAGE_CONTROL		0
#define MXF_SPELLMAN_HVPS_CURRENT_CONTROL		1
#define MXF_SPELLMAN_HVPS_POWER_LIMIT			2
#define MXF_SPELLMAN_HVPS_FILAMENT_CURRENT_LIMIT	3

/* Digital control signals */

#define MXF_SPELLMAN_HVPS_XRAY_ON		0
#define MXF_SPELLMAN_HVPS_XRAY_OFF		1
#define MXF_SPELLMAN_HVPS_POWER_SUPPLY_RESET	2

/* Analog monitor signals */

#define MXF_SPELLMAN_HVPS_VOLTAGE_MONITOR		0
#define MXF_SPELLMAN_HVPS_CURRENT_MONITOR		1
#define MXF_SPELLMAN_HVPS_FILAMENT_CURRENT_MONITOR	2

/* Digital monitor signals */

#define MXF_SPELLMAN_HVPS_KV_MINIMUM_FAULT		0
#define MXF_SPELLMAN_HVPS_OVERCURRENT_FAULT		1
#define MXF_SPELLMAN_HVPS_OVERPOWER_FAULT		2
#define MXF_SPELLMAN_HVPS_OVERVOLTAGE_FAULT		3
#define MXF_SPELLMAN_HVPS_FILAMENT_CURRENT_LIMIT_FAULT	4
#define MXF_SPELLMAN_HVPS_POWER_SUPPLY_FAULT		5
#define MXF_SPELLMAN_HVPS_XRAY_ON_INDICATOR		6
#define MXF_SPELLMAN_HVPS_INTERLOCK_STATUS		7
#define MXF_SPELLMAN_HVPS_CONTROL_MODE_INDICATOR	8

/*---*/

/* NOTE: The control and monitor values in the following structure are
 * stored in the integer hexadecimal format rather than floating point.
 */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	double query_interval;				/* in seconds */
	unsigned long default_power_limit;		/* in hex */
	unsigned long default_filament_current_limit;	/* in hex */

	int analog_control[4];
	int analog_monitor[3];
	int digital_control[3];
	int digital_monitor[9];

	MX_CLOCK_TICK ticks_per_query;
	MX_CLOCK_TICK next_query_tick;
} MX_SPELLMAN_HVPS;

#define MXI_SPELLMAN_HVPS_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPELLMAN_HVPS, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "query_interval", MXFT_DOUBLE, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_SPELLMAN_HVPS, query_interval), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "default_power_limit", MXFT_HEX, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_SPELLMAN_HVPS, default_power_limit), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "default_filament_current_limit", MXFT_HEX, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SPELLMAN_HVPS, default_filament_current_limit), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_spellman_hvps_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxi_spellman_hvps_open( MX_RECORD *record );

MX_API mx_status_type mxi_spellman_hvps_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_spellman_hvps_command(
					MX_SPELLMAN_HVPS *spellman_hvps,
					char *command,
					char *response,
					size_t response_buffer_length,
					int debug_flag );

MX_API mx_status_type mxi_spellman_hvps_query_command(
					MX_SPELLMAN_HVPS *spellman_hvps );

MX_API mx_status_type mxi_spellman_hvps_set_command(
					MX_SPELLMAN_HVPS *spellman_hvps );

extern MX_RECORD_FUNCTION_LIST mxi_spellman_hvps_record_function_list;

extern long mxi_spellman_hvps_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_spellman_hvps_rfield_def_ptr;

#endif /* __I_SPELLMAN_HVPS_H__ */

