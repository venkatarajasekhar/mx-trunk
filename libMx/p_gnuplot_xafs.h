/*
 * Name:    p_gnuplot_xafs.h
 *
 * Purpose: Header file for "gnuplot" plotting support of XAFS scans.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __P_GNUPLOT_XAFS_H__
#define __P_GNUPLOT_XAFS_H__

typedef struct {
	MX_COPROCESS *coprocess;
	MX_RECORD *energy_motor_record;
} MX_PLOT_GNUXAFS;

MX_API mx_status_type mxp_gnuxafs_open( MX_PLOT *plot );
MX_API mx_status_type mxp_gnuxafs_close( MX_PLOT *plot );
MX_API mx_status_type mxp_gnuxafs_add_measurement_to_plot_buffer(
							MX_PLOT *plot );
MX_API mx_status_type mxp_gnuxafs_add_array_to_plot_buffer( MX_PLOT *plot,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array );
MX_API mx_status_type mxp_gnuxafs_display_plot( MX_PLOT *plot );
MX_API mx_status_type mxp_gnuxafs_set_x_range( MX_PLOT *plot,
					double x_min, double x_max );
MX_API mx_status_type mxp_gnuxafs_set_y_range( MX_PLOT *plot,
					double y_min, double y_max );
MX_API mx_status_type mxp_gnuxafs_start_plot_section( MX_PLOT *plot );

extern MX_PLOT_FUNCTION_LIST mxp_gnuxafs_function_list;

#endif /* __P_GNUPLOT_XAFS_H__ */

