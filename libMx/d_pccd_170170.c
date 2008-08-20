/*
 * Name:    d_pccd_170170.c
 *
 * Purpose: MX driver for Aviex PCCD series CCD detector.
 *
 * Author:  William Lavender
 *
 * Note:    Currently supported detectors include
 *              PCCD-170170   at SOLEIL SWING
 *              PCCD-4824     at ESRF (WAXS)
 *              PCCD-16080    at APS BioCAT
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PCCD_170170_DEBUG				FALSE

#define MXD_PCCD_170170_DEBUG_BIAS_LOOKUP		FALSE

#define MXD_PCCD_170170_DEBUG_DESCRAMBLING		FALSE

#define MXD_PCCD_170170_DEBUG_ALLOCATION		FALSE

#define MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS	FALSE

#define MXD_PCCD_170170_DEBUG_SERIAL			FALSE

#define MXD_PCCD_170170_DEBUG_MX_IMAGE_ALLOC		FALSE

#define MXD_PCCD_170170_DEBUG_TIMING			FALSE

#define MXD_PCCD_170170_DEBUG_FRAME_CORRECTION		FALSE

#define MXD_PCCD_170170_DEBUG_SETUP_GEOMETRICAL_MASK	FALSE

#define MXD_PCCD_170170_DEBUG_SEQUENCE_TIMES		FALSE

#define MXD_PCCD_170170_DEBUG_EXTENDED_STATUS		FALSE

#define MXD_PCCD_170170_DEBUG_MEMORY_LEAK		FALSE

#define MXD_PCCD_170170_DEBUG_CONTROL_REGISTER		FALSE

#define MXD_PCCD_170170_GEOMETRICAL_MASK_KLUDGE		TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_unistd.h"
#include "mx_ascii.h"
#include "mx_clock.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_key.h"
#include "mx_cfn.h"
#include "mx_bit.h"
#include "mx_hrt.h"
#include "mx_memory.h"
#include "mx_image.h"
#include "mx_digital_output.h"
#include "mx_video_input.h"
#include "mx_camera_link.h"
#include "mx_area_detector.h"
#include "i_epix_xclib.h"
#include "d_epix_xclib.h"
#include "d_pccd_170170.h"

#if MXD_PCCD_170170_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

/*------------------------------------------------------------------------*/

#if MXD_PCCD_170170_DEBUG_MEMORY_LEAK
#  define DISPLAY_MEMORY_USAGE( label ) \
	do {                                                                  \
	    MX_PROCESS_MEMINFO meminfo;                                       \
	    (void) mx_get_process_meminfo( MXF_PROCESS_ID_SELF, &meminfo );   \
	    MX_DEBUG(-2,("\n%s: total_bytes = %lu\n",                         \
			(label), (unsigned long) meminfo.total_bytes));       \
	} while(0)

#else
#  define DISPLAY_MEMORY_USAGE( label )
#endif

/*------------------------------------------------------------------------*/

#if MXD_PCCD_170170_DEBUG_CONTROL_REGISTER
#  define DISPLAY_CONTROL_REGISTER( label ) \
	do {                                                                  \
		long dse_dh_control;                                          \
		(void) mx_area_detector_get_register( ad->record,             \
			"dh_control", &dse_dh_control );                      \
		MX_DEBUG(-2,("%s: dh_control = %#lx", label, dse_dh_control));\
	} while(0)
#else
#  define DISPLAY_CONTROL_REGISTER( label )
#endif

/*------------------------------------------------------------------------*/

/* Internal prototype for smvspatial. */

MX_API int
smvspatial( void *imarr,
	int imwid,
	int imhit,
	int cflags,
	char *splname,
	uint16_t *maskval );

/*------------------------------------------------------------------------*/


#define MXD_MAXIMUM_TOTAL_SUBIMAGE_LINES	2048

MX_RECORD_FUNCTION_LIST mxd_pccd_170170_record_function_list = {
	mxd_pccd_170170_initialize_type,
	mxd_pccd_170170_create_record_structures,
	mx_area_detector_finish_record_initialization,
	mxd_pccd_170170_delete_record,
	NULL,
	NULL,
	NULL,
	mxd_pccd_170170_open,
	mxd_pccd_170170_close,
	NULL,
	mxd_pccd_170170_resynchronize,
	mxd_pccd_170170_special_processing_setup
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_pccd_170170_function_list = {
	mxd_pccd_170170_arm,
	mxd_pccd_170170_trigger,
	mxd_pccd_170170_stop,
	mxd_pccd_170170_abort,
	NULL,
	NULL,
	NULL,
	mxd_pccd_170170_get_extended_status,
	mxd_pccd_170170_readout_frame,
	mxd_pccd_170170_correct_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_pccd_170170_get_parameter,
	mxd_pccd_170170_set_parameter,
	mx_area_detector_default_measure_correction,
	mxd_pccd_170170_geometrical_correction
};

/* PCCD-170170 data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_pccd_170170_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_PCCD_170170_STANDARD_FIELDS
};

long mxd_pccd_170170_num_record_fields
		= sizeof( mxd_pccd_170170_record_field_defaults )
			/ sizeof( mxd_pccd_170170_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pccd_170170_rfield_def_ptr
			= &mxd_pccd_170170_record_field_defaults[0];

/* PCCD-4824 data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_pccd_4824_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_PCCD_170170_STANDARD_FIELDS
};

long mxd_pccd_4824_num_record_fields
		= sizeof( mxd_pccd_4824_record_field_defaults )
			/ sizeof( mxd_pccd_4824_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pccd_4824_rfield_def_ptr
			= &mxd_pccd_4824_record_field_defaults[0];

/* PCCD-16080 data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_pccd_16080_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_PCCD_170170_STANDARD_FIELDS
};

long mxd_pccd_16080_num_record_fields
		= sizeof( mxd_pccd_16080_record_field_defaults )
			/ sizeof( mxd_pccd_16080_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pccd_16080_rfield_def_ptr
			= &mxd_pccd_16080_record_field_defaults[0];

/*---*/

#define INIT_REGISTER( i, v, r, t, n, x ) \
	do {                                                          \
		mx_status = mxd_pccd_170170_init_register(            \
			pccd_170170, (i), (v), (r), (t), (n), (x) );  \
	                                                              \
		if ( mx_status.code != MXE_SUCCESS )                  \
			return mx_status;                             \
	} while(0)

/*---*/

static mx_status_type mxd_pccd_170170_process_function( void *record_ptr,
							void *record_field_ptr,
							int operation );

static mx_status_type
mxd_pccd_170170_get_pointers( MX_AREA_DETECTOR *ad,
			MX_PCCD_170170 **pccd_170170,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pccd_170170_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pccd_170170 == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PCCD_170170 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pccd_170170 = (MX_PCCD_170170 *)
				ad->record->record_type_struct;

	if ( *pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_PCCD_170170 pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

#if MXD_PCCD_170170_DEBUG_ALLOCATION
static void
mxd_pccd_170170_display_ul_corners( uint16_t ***sector_array, int num_sectors )
{
	static const char fname[] = "mxd_pccd_170170_display_ul_corners()";

	long k;
	char *ptr;

	MX_DEBUG(-2,("****** %s BEGIN *****", fname));

	for ( k = 0; k < num_sectors; k++ ) {
		if ( sector_array[k] == NULL ) {
			MX_DEBUG(-2,("****** %s END alt *****", fname));
			return;
		}

		ptr = (char *) &(sector_array[k][0][0]);

		MX_DEBUG(-2,("ul_corner[%ld] = %p", k, ptr));
	}

	MX_DEBUG(-2,("****** %s END *****", fname));
}
#endif

MX_EXPORT mx_status_type
mxd_pccd_170170_alloc_sector_array( uint16_t ****sector_array_ptr,
					long sector_width,
					long sector_height,
					long num_sector_rows,
					long num_sector_columns,
					uint16_t *image_data )
{
	static const char fname[] = "mxd_pccd_170170_alloc_sector_array()";

	uint16_t ***sector_array;
	uint16_t **sector_array_row_ptr;
	long n, sector_row, row, sector_column, num_sectors;
	long sizeof_row_of_sectors, sizeof_full_row, sizeof_sector_row;
	long row_byte_offset, row_ptr_offset;
	long byte_offset, ptr_offset;

	if ( sector_array_ptr == (uint16_t ****) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The sector_array_ptr argument passed was NULL." );
	}
	if ( image_data == (uint16_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The image_data pointer passed was NULL." );
	}

#if MXD_PCCD_170170_DEBUG_ALLOCATION
	MX_DEBUG(-2,("%s: sector_width = %ld, sector_height = %ld",
		fname, sector_width, sector_height));
#endif

	num_sectors = num_sector_rows * num_sector_columns;

	*sector_array_ptr = NULL;
	
	sector_array = malloc( num_sectors * sizeof(uint16_t **) );

	if ( sector_array == (uint16_t ***) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element "
		"sector array pointer.", num_sectors );
	}

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
	MX_DEBUG(-2,("%s: allocated sector_array = %p, length = %lu",
		fname, sector_array, num_sectors * sizeof(uint16_t **) ));
#endif

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
	memset( sector_array, 0, num_sectors * sizeof(uint16_t **) );
#endif

	sector_array_row_ptr = 
		malloc( num_sectors * sector_height * sizeof(uint16_t *) );

	if ( sector_array_row_ptr == (uint16_t **) NULL ) {
		free( sector_array );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of row pointers.", num_sectors * sector_height );
	}

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
	MX_DEBUG(-2,("%s: allocated sector_array_row_ptr = %p, length = %lu",
		fname, sector_array_row_ptr,
		num_sectors * sector_height * sizeof(uint16_t *) ));
#endif

	row_byte_offset = sector_height * sizeof(sector_array_row_ptr);

	row_ptr_offset = row_byte_offset / sizeof(uint16_t *);

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
	MX_DEBUG(-2,
	("%s: row_byte_offset = %ld (%#lx), row_ptr_offset = %ld (%#lx)",
		fname, row_byte_offset, row_byte_offset,
		row_ptr_offset, row_ptr_offset));

	MX_DEBUG(-2,("%s: sector_array_row_pointer = %p",
			fname, sector_array_row_ptr ));
#endif

	for ( n = 0; n < num_sectors; n++ ) {

		sector_array[n] = sector_array_row_ptr + n * row_ptr_offset;

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
		MX_DEBUG(-2,("%s: sector_array[%ld] = %p",
			fname, n, sector_array[n] ));
#endif
	}

	/* FIXME - The numerical values in the following comments only apply
	 *         directly to the PCCD-170170.
	 */

	/* The 'sizeof_full_row' is the number of bytes in a single horizontal
	 * line of the image.  The 'sizeof_row_of_sectors' is the number of
	 * bytes in a horizontal row of sectors.  In unbinned mode,
	 * sizeof_row_of_sectors = sector_height * sizeof_full_row.
	 */

	/* sizeof_sector_row     = number of bytes in a single horizontal
	 *                         row of a sector.
	 *
	 * sizeof_full_row       = number of bytes in a single horizontal
	 *                         row of the full image.
	 *
	 * sizeof_row_of_sectors = number of bytes in all the rows of a
	 *                         single row of sectors.
	 *
	 * For an unbinned image:
	 *
	 *   sizeof_row_of_sectors = sector_height * sizeof_full_row
	 *        = sector_height * num_sector_columns * sizeof_sector_row
	 */

	sizeof_sector_row = sector_width * sizeof(uint16_t);

	sizeof_full_row = num_sector_columns * sizeof_sector_row;

	sizeof_row_of_sectors = sizeof_full_row * sector_height;

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
	MX_DEBUG(-2,
("%s: image_data = %p, sizeof_full_row = %#lx, sizeof_row_of_sectors = %#lx",
		fname, image_data, sizeof_full_row, sizeof_row_of_sectors));
#endif

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
	MX_DEBUG(-2,("%s:\n"
		"    byte_offset = ( %#lx ) * sector_row\n"
		"                  + ( %#lx ) * row\n"
		"                  + ( %#lx ) * sector_column\n",
	fname, sizeof_row_of_sectors, sizeof_full_row, sizeof_sector_row));

	MX_DEBUG(-2,("%s:\n"
		"    byte_offset = ( %lu ) * sector_row\n"
		"                  + ( %lu ) * row\n"
		"                  + ( %lu ) * sector_column\n",
	fname, sizeof_row_of_sectors, sizeof_full_row, sizeof_sector_row));

	MX_DEBUG(-2,("%s: image_data = %p", fname, image_data));
#endif

	for ( sector_row = 0; sector_row < num_sector_rows; sector_row++ ) {
	    for ( row = 0; row < sector_height; row++ ) {
		for ( sector_column = 0; sector_column < num_sector_columns;
							sector_column++ )
		{

		    n = sector_column + num_sector_columns * sector_row;

		    byte_offset = sector_row * sizeof_row_of_sectors
				+ row * sizeof_full_row
				+ sector_column * sizeof_sector_row;

		    ptr_offset = byte_offset / sizeof(uint16_t);

		    sector_array[n][row] = image_data + ptr_offset;

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
		    if ( row == 0 ) {
			MX_DEBUG(-2,
	("*** n = %ld, sector_row = %ld, sector_column = %ld, addr = %#lx ***",
				n, sector_row, sector_column,
				(long) byte_offset + (long) image_data));

#if MXD_PCCD_170170_DEBUG_DESCRAMBLING
			mxd_pccd_170170_display_ul_corners( sector_array,
								num_sectors );
#endif
		    }
#endif

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
		    MX_DEBUG(-2,
	    ("offset = %#lx = %#lx * (%#lx) + %#lx * (%#lx) + %#lx * (%#lx)",
			byte_offset, sizeof_row_of_sectors, sector_row,
			sizeof_full_row, row,
			sizeof_sector_row, sector_column));
#endif

#if MXD_PCCD_170170_DEBUG_ALLOCATION_DETAILS
		    MX_DEBUG(-2,
	    ("offset = %ld = %ld * (%ld) + %ld * (%ld) + %ld * (%ld)",
			byte_offset, sizeof_row_of_sectors, sector_row,
			sizeof_full_row, row,
			sizeof_sector_row, sector_column));
#endif
		}
	    }
	}

	*sector_array_ptr = sector_array;

#if MXD_PCCD_170170_DEBUG_ALLOCATION
	mxd_pccd_170170_display_ul_corners( sector_array, num_sectors );

#if 0
	fprintf(stderr, "Type any key to continue..." );
	mx_getch();
	fprintf(stderr, "\n");
#endif
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mxd_pccd_170170_free_sector_array( uint16_t ***sector_array )
{
	uint16_t **sector_array_row_ptr;

	if ( sector_array == NULL ) {
		return;
	}

	sector_array_row_ptr = *sector_array;

	if ( sector_array_row_ptr != NULL ) {
		free( sector_array_row_ptr );
	}

	free( sector_array );

	return;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_pccd_170170_load_bias_lookup_table( MX_PCCD_170170 *pccd_170170 )
{
	static const char fname[] = "mxd_pccd_170170_load_bias_lookup_table()";

	FILE *file;
	int saved_errno;
	unsigned long table_size, bytes_read;

#if MXD_PCCD_170170_DEBUG_BIAS_LOOKUP
	MX_DEBUG(-2,("%s invoked for detector '%s'.",
		fname, pccd_170170->record->name ));
#endif

	/* The bias lookup file is a binary file in 'native' byte order that
	 * contains 65536 16-bit integers.  No byte swapping is required.
	 */

	file = fopen( pccd_170170->bias_lookup_table_filename, "rb" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to open bias lookup table file '%s' failed.  "
		"Errno = %d, error message = '%s'",
			pccd_170170->bias_lookup_table_filename,
			saved_errno, mx_strerror( saved_errno, NULL, 0 ) );
	}

	/* Allocate memory for the bias lookup table. */

	table_size = 65536 * sizeof(uint16_t);

	pccd_170170->bias_lookup_table = malloc( table_size );

	if ( pccd_170170->bias_lookup_table == NULL ) {
		fclose( file );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu byte "
		"bias lookup table.", table_size );
	}

	/* Read the contents of the bias lookup table file into the 
	 * local data structure.
	 */

	bytes_read = fread( pccd_170170->bias_lookup_table,
				1, table_size, file );

	fclose( file );

	if ( bytes_read < table_size ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Only %lu bytes were read from the bias lookup table "
		"file '%s'.  The file was supposed to be %lu bytes long.",
			bytes_read, pccd_170170->bias_lookup_table_filename,
			table_size );
	}
	
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_pccd_170170_apply_bias_lookup_table( MX_PCCD_170170 *pccd_170170,
					MX_IMAGE_FRAME *image_frame )
{
	uint16_t *image_data, *lookup_table;
	uint16_t pixel;
	unsigned long i;

#if MXD_PCCD_170170_DEBUG_BIAS_LOOKUP
	static const char fname[] = "mxd_pccd_170170_apply_bias_lookup_table()";

	MX_DEBUG(-2,("%s invoked for detector '%s'.",
		fname, pccd_170170->record->name ));
#endif

	image_data = image_frame->image_data;

	lookup_table = pccd_170170->bias_lookup_table;

	for ( i = 0; i < image_frame->image_length; i++ ) {
		pixel = image_data[i];

		image_data[i] = lookup_table[pixel];
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_pccd_170170_descramble_raw_data( uint16_t *raw_frame_data,
				uint16_t ***image_sector_array,
				long i_framesize,
				long j_framesize )
{
#if 0 && MXD_PCCD_170170_DEBUG_DESCRAMBLING
	static const char fname[] = "mxd_pccd_170170_descramble_raw_data()";
#endif

	long i, j;

#if 0 && MXD_PCCD_170170_DEBUG_DESCRAMBLING
	mxd_pccd_170170_display_ul_corners( image_sector_array, 16 );
#endif

	for ( i = 0; i < i_framesize; i++ ) {
	    for ( j = 0; j < j_framesize; j++ ) {

		image_sector_array[0][i][j] = raw_frame_data[14];

		image_sector_array[1][i][j_framesize-j-1] = raw_frame_data[15];

		image_sector_array[2][i][j] = raw_frame_data[10];

		image_sector_array[3][i][j_framesize-j-1] = raw_frame_data[11];

		image_sector_array[4][i_framesize-i-1][j] = raw_frame_data[13];

		image_sector_array[5][i_framesize-i-1][j_framesize-j-1]
							= raw_frame_data[12];

		image_sector_array[6][i_framesize-i-1][j] = raw_frame_data[9];

		image_sector_array[7][i_framesize-i-1][j_framesize-j-1]
							= raw_frame_data[8];

		image_sector_array[8][i][j] = raw_frame_data[0];

		image_sector_array[9][i][j_framesize-j-1] = raw_frame_data[1];

		image_sector_array[10][i][j] = raw_frame_data[4];

		image_sector_array[11][i][j_framesize-j-1] = raw_frame_data[5];

		image_sector_array[12][i_framesize-i-1][j] = raw_frame_data[3];

		image_sector_array[13][i_framesize-i-1][j_framesize-j-1]
							= raw_frame_data[2];

		image_sector_array[14][i_framesize-i-1][j] = raw_frame_data[7];

		image_sector_array[15][i_framesize-i-1][j_framesize-j-1]
							= raw_frame_data[6];

		raw_frame_data += 16;
	    }
	}

#if 0 && MXD_PCCD_170170_DEBUG_DESCRAMBLING
	{
		long k;

		for ( k = 0; k < 16; k++ ) {
			MX_DEBUG(-2,("%s: ul_corner[%ld] = %d",
				fname, k, image_sector_array[k][0][0] ));
		}
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_pccd_4824_descramble_raw_data( uint16_t *raw_frame_data,
				uint16_t ***image_sector_array,
				long i_framesize,
				long j_framesize )
{
#if 0 && MXD_PCCD_170170_DEBUG_DESCRAMBLING
	static const char fname[] = "mxd_pccd_4824_descramble_raw_data()";
#endif

	long i, j;

#if 0 && MXD_PCCD_170170_DEBUG_DESCRAMBLING
	mxd_pccd_170170_display_ul_corners( image_sector_array, 4 );
#endif

	for ( i = 0; i < i_framesize; i++ ) {
	    for ( j = 0; j < j_framesize; j++ ) {

		image_sector_array[0][i][j] = raw_frame_data[2];

		image_sector_array[1][i][j_framesize-j-1] = raw_frame_data[3];

		image_sector_array[2][i_framesize-i-1][j] = raw_frame_data[1];

		image_sector_array[3][i_framesize-i-1][j_framesize-j-1]
							= raw_frame_data[0];
		raw_frame_data += 4;
	    }
	}

#if 0 && MXD_PCCD_170170_DEBUG_DESCRAMBLING
	{
		long k;

		for ( k = 0; k < 4; k++ ) {
			MX_DEBUG(-2,("%s: ul_corner[%ld] = %d",
				fname, k, image_sector_array[k][0][0] ));
		}
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_pccd_170170_descramble_streak_camera( MX_AREA_DETECTOR *ad,
				MX_PCCD_170170 *pccd_170170,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *raw_frame )
{
#if 0
	static const char fname[] =
		"mxd_pccd_170170_descramble_streak_camera()";
#endif
	uint16_t *image_data, *raw_data;
	uint16_t *image_ptr, *raw_ptr;
	long i, j, row_framesize, column_framesize, total_raw_pixels;
	long rfs;
	mx_status_type mx_status;

	/* First, we figure out how many pixels are in each line
	 * of the raw data by asking for the framesize.
	 */

	mx_status = mx_area_detector_get_framesize( ad->record,
					&row_framesize, &column_framesize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	total_raw_pixels = row_framesize * column_framesize;

	raw_data   = raw_frame->image_data;
	image_data = image_frame->image_data;

#if 0
	MX_DEBUG(-2,("%s: row_framesize = %ld, column_framesize = %ld",
		fname, row_framesize, column_framesize));
#endif

	/* Loop through the lines of the raw image. */

	for ( i = 0; i < (column_framesize/2L); i++ ) {
		/* The raw data arrives in groups of 16 pixels that need
		 * to be appropriately copied to the final image frame.
		 *
		 * We transform 4 lines worth of raw data into 2 lines
		 * worth of image data, since half of the raw data is
		 * discarded.
		 */

		raw_ptr = raw_data + i * row_framesize * 4L;

		image_ptr = image_data + i * row_framesize * 2L;

		/* Copy the pixels. */

		for ( j = 0; j < (row_framesize/4L); j++ ) {

			rfs = row_framesize;

			if ( pccd_170170->use_top_half_of_detector ) {
				image_ptr[j]               = raw_ptr[16*j + 14];
				image_ptr[rfs/2 - 1 - j]   = raw_ptr[16*j + 15];
				image_ptr[rfs/2 + j]       = raw_ptr[16*j + 10];
				image_ptr[rfs - 1 - j]     = raw_ptr[16*j + 11];
				image_ptr[rfs + j]         = raw_ptr[16*j + 13];
				image_ptr[rfs + rfs/2 - 1 - j]
							   = raw_ptr[16*j + 12];
				image_ptr[rfs + rfs/2 + j] = raw_ptr[16*j + 9];
				image_ptr[2 * rfs - 1 - j] = raw_ptr[16*j + 8];
			} else {
				image_ptr[j]               = raw_ptr[16*j];
				image_ptr[rfs/2 - 1 - j]   = raw_ptr[16*j + 1];
				image_ptr[rfs/2 + j]       = raw_ptr[16*j + 4];
				image_ptr[rfs - 1 - j]     = raw_ptr[16*j + 5];
				image_ptr[rfs + j]         = raw_ptr[16*j + 3];
				image_ptr[rfs + rfs/2 - 1 - j]
							   = raw_ptr[16*j + 2];
				image_ptr[rfs + rfs/2 + j] = raw_ptr[16*j + 7];
				image_ptr[2 * rfs - 1 - j] = raw_ptr[16*j + 6];
			}
		}
	}

	/* Patch the column framesize and the image length so that
	 * it matches the total size of the streak camera image.
	 */

	MXIF_COLUMN_FRAMESIZE(image_frame) = column_framesize / 2L;

	image_frame->image_length = ( total_raw_pixels / 2L )
			* mx_round( MXIF_BYTES_PER_PIXEL(raw_frame) );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_pccd_4824_descramble_streak_camera( MX_AREA_DETECTOR *ad,
				MX_PCCD_170170 *pccd_170170,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *raw_frame )
{
#if 0
	static const char fname[] = "mxd_pccd_4824_descramble_streak_camera()";
#endif
	uint16_t *image_data, *raw_data;
	uint16_t *image_ptr, *raw_ptr;
	long i, j, row_framesize, column_framesize, total_raw_pixels;
	long rfs;
	mx_status_type mx_status;

	/* First, we figure out how many pixels are in each line
	 * of the raw data by asking for the framesize.
	 */

	mx_status = mx_area_detector_get_framesize( ad->record,
					&row_framesize, &column_framesize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	total_raw_pixels = row_framesize * column_framesize;

	raw_data   = raw_frame->image_data;
	image_data = image_frame->image_data;

#if 0
	MX_DEBUG(-2,("%s: row_framesize = %ld, column_framesize = %ld",
		fname, row_framesize, column_framesize));
#endif

	/* Loop through the lines of the raw image. */

	for ( i = 0; i < (column_framesize/2L); i++ ) {
		/* The raw data arrives in groups of 4 pixels that need
		 * to be appropriately copied to the final image frame.
		 */

		raw_ptr = raw_data + i * row_framesize * 2L;

		image_ptr = image_data + i * row_framesize * 2L;

		/* Copy the pixels. */

		for ( j = 0; j < (row_framesize/4L); j++ ) {

			rfs = row_framesize;

			image_ptr[j]                   = raw_ptr[16*j + 2];
			image_ptr[rfs/2 - 1 - j]       = raw_ptr[16*j + 3];
			image_ptr[rfs + j]             = raw_ptr[16*j + 1];
			image_ptr[rfs + rfs/2 - 1 - j] = raw_ptr[16*j + 0];
		}
	}

	/* Patch the column framesize and the image length so that
	 * it matches the total size of the streak camera image.
	 */

	MXIF_COLUMN_FRAMESIZE(image_frame) = column_framesize / 2L;

	image_frame->image_length = ( total_raw_pixels / 2L )
			* mx_round( MXIF_BYTES_PER_PIXEL(raw_frame) );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_pccd_170170_descramble_image( MX_AREA_DETECTOR *ad,
				MX_PCCD_170170 *pccd_170170,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *raw_frame )
{
	static const char fname[] = "mxd_pccd_170170_descramble_image()";

	MX_SEQUENCE_PARAMETERS *sp;
	long i, i_framesize, j_framesize;
	long row_framesize, column_framesize;
	long num_sector_rows, num_sector_columns;
	uint16_t *frame_data;
	mx_status_type mx_status;

#if MXD_PCCD_170170_DEBUG_TIMING
	MX_HRT_TIMING memcpy_measurement;
	MX_HRT_TIMING total_measurement;

	/* Shut up useless GCC 4 initialized variable warning. */

	memset( &memcpy_measurement, 0, sizeof(MX_HRT_TIMING) );

	MX_HRT_START(total_measurement);
#endif
	/* Shut up useless GCC 4 initialized variable warnings. */

	i_framesize = j_framesize = 0;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}
	if ( raw_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The raw_frame pointer passed was NULL." );
	}

	sp = &(ad->sequence_parameters);

	switch( ad->record->mx_type ) {
	case MXT_AD_PCCD_170170:
		num_sector_rows = 4;
		num_sector_columns = 4;
		break;
	case MXT_AD_PCCD_4824:
		num_sector_rows = 2;
		num_sector_columns = 2;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal MX driver type '%s' passed for record '%s'.",
			mx_get_driver_name( ad->record ),
			ad->record->name );
		break;
	}

	/* We handle streak camera descrambling in a special function
	 * just for it.
	 */

	if ( sp->sequence_type == MXT_SQ_STREAK_CAMERA ) {
		switch( ad->record->mx_type ) {
		case MXT_AD_PCCD_170170:
			mx_status = mxd_pccd_170170_descramble_streak_camera(
				ad, pccd_170170, image_frame, raw_frame );
			break;
		case MXT_AD_PCCD_4824:
			mx_status = mxd_pccd_4824_descramble_streak_camera(
				ad, pccd_170170, image_frame, raw_frame );
			break;
		}

#if MXD_PCCD_170170_DEBUG_DESCRAMBLING
		MX_DEBUG(-2,
		("%s: Streak camera descrambling complete.", fname));
#endif
		return mx_status;
	}

	/* If this is a subimage frame, we initially
	 * descramble to a temporary frame.
	 */

	if (sp->sequence_type == MXT_SQ_SUBIMAGE) {

#if MXD_PCCD_170170_DEBUG_MX_IMAGE_ALLOC
		MX_DEBUG(-2,
	("%s: Invoking mx_image_alloc() for pccd_170170->temp_frame = %p",
			fname, pccd_170170->temp_frame ));
#endif

		mx_status = mx_image_alloc( &(pccd_170170->temp_frame),
					MXIF_ROW_FRAMESIZE(image_frame),
					MXIF_COLUMN_FRAMESIZE(image_frame),
					MXIF_IMAGE_FORMAT(image_frame),
					MXIF_BYTE_ORDER(image_frame),
					MXIF_BYTES_PER_PIXEL(image_frame),
					image_frame->header_length,
					image_frame->image_length );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		frame_data = pccd_170170->temp_frame->image_data;
	} else {
		frame_data = image_frame->image_data;
	}

#if MXD_PCCD_170170_DEBUG_DESCRAMBLING
	{
		long num_pixels;

		num_pixels = image_frame->allocated_image_length / 2;

		for ( i = 0; i < num_pixels; i++ ) {
			frame_data[i] = 0x3fff;
		}
	}
#endif

	/* For each frame, we overlay the frame with 16 sets of sector array
	 * pointers so that we can treat each of the 16 sectors as independent
	 * two dimensional arrays.
	 */

	/* The sector_array pointer will be NULL, if any of the following
	 * has happened.
	 *
	 * 1. This is the first frame taken by the program.
	 *
	 * 2. The framesize or detector mode has changed
	 *      (full frame, subimage, streak camera).
	 */

	/* Initially descramble the full image. */

	column_framesize = MXIF_COLUMN_FRAMESIZE(image_frame);
	row_framesize = MXIF_ROW_FRAMESIZE(image_frame);

	switch( ad->record->mx_type ) {
	case MXT_AD_PCCD_170170:
		i_framesize = column_framesize / 4;
		j_framesize = row_framesize / 4;
		break;
	case MXT_AD_PCCD_4824:
		i_framesize = column_framesize / 2;
		j_framesize = row_framesize / 2;
		break;
	}

	if ( pccd_170170->sector_array == NULL ) {

		mx_status = mxd_pccd_170170_alloc_sector_array(
				&(pccd_170170->sector_array),
				j_framesize, i_framesize,
				num_sector_rows, num_sector_columns,
				frame_data );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Copy and descramble the pixels from the raw frame to the image frame.
	 */

	switch( ad->record->mx_type ) {
	case MXT_AD_PCCD_170170:
		mx_status = mxd_pccd_170170_descramble_raw_data(
				raw_frame->image_data,
				pccd_170170->sector_array,
				i_framesize, j_framesize );
		break;
	case MXT_AD_PCCD_4824:
		mx_status = mxd_pccd_4824_descramble_raw_data(
				raw_frame->image_data,
				pccd_170170->sector_array,
				i_framesize, j_framesize );
		break;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if (sp->sequence_type == MXT_SQ_SUBIMAGE) {

		long num_lines_per_subimage, num_subimages, frame_width;
		long bytes_per_half_subimage, bytes_per_half_image;
		long top_offset, bottom_offset, use_top_half;
		char *temp_ptr, *image_ptr;
		char *top_src_ptr, *top_dest_ptr;
		char *bottom_src_ptr, *bottom_dest_ptr;

		num_lines_per_subimage = sp->parameter_array[0];
		num_subimages          = sp->parameter_array[1];
		frame_width            = MXIF_ROW_FRAMESIZE(image_frame);

		temp_ptr  = pccd_170170->temp_frame->image_data;
		image_ptr = image_frame->image_data;

		bytes_per_half_subimage =
			frame_width * num_lines_per_subimage
			* ( sizeof(uint16_t) / 2L );

		bytes_per_half_image = MXIF_ROW_FRAMESIZE(image_frame)
				* MXIF_COLUMN_FRAMESIZE(image_frame)
				* ( sizeof(uint16_t) / 2L );

#if MXD_PCCD_170170_DEBUG_DESCRAMBLING
		MX_DEBUG(-2,("%s: num_lines_per_subimage = %ld",
			fname, num_lines_per_subimage));
		MX_DEBUG(-2,("%s: num_subimages = %ld", fname, num_subimages));
		MX_DEBUG(-2,("%s: frame_width = %ld", fname, frame_width));
		MX_DEBUG(-2,("%s: bytes_per_half_subimage = %ld",
			fname, bytes_per_half_subimage));
		MX_DEBUG(-2,("%s: bytes_per_half_image = %ld",
			fname, bytes_per_half_image));
#endif

		/* Check the flag that tells subimage mode to use the top
		 * half of the detector rather than the bottom half.
		 */

		if ( pccd_170170->use_top_half_of_detector ) {
			use_top_half = 1L;
		} else {
			use_top_half = 0L;
		}

#if MXD_PCCD_170170_DEBUG_TIMING
		MX_HRT_START( memcpy_measurement );
#endif

		for ( i = 0; i < num_subimages; i++ ) {
			top_offset =
				(1L - use_top_half) * bytes_per_half_image
					+ i * bytes_per_half_subimage;

			top_src_ptr = temp_ptr + top_offset;

			top_dest_ptr = image_ptr
				+ (2L * i) * bytes_per_half_subimage;

			memcpy( top_dest_ptr, top_src_ptr,
					bytes_per_half_subimage );

			bottom_offset =
				(2L - use_top_half) * bytes_per_half_image
					- (i+1L) * bytes_per_half_subimage;

			bottom_src_ptr = temp_ptr + bottom_offset;

			bottom_dest_ptr = image_ptr
				+ ((2L * i) + 1L) * bytes_per_half_subimage;

			memcpy( bottom_dest_ptr, bottom_src_ptr,
					bytes_per_half_subimage );
		}

#if MXD_PCCD_170170_DEBUG_TIMING
		MX_HRT_END( memcpy_measurement );
#endif

#if 1
		/* Patch the column framesize and the image length so that
		 * it matches the total size of the subimage frame.
		 */

		MXIF_COLUMN_FRAMESIZE(image_frame) =
					num_subimages * num_lines_per_subimage;

		image_frame->image_length =
			num_subimages * bytes_per_half_subimage * 2L;
#endif
	}

#if MXD_PCCD_170170_DEBUG_TIMING
	MX_HRT_END( total_measurement );

	MX_HRT_RESULTS( memcpy_measurement, fname, "for subimage memcpy." );
	MX_HRT_RESULTS( total_measurement, fname, "for total descramble." );
#endif

#if MXD_PCCD_170170_DEBUG_DESCRAMBLING
	MX_DEBUG(-2,("%s: Image descrambling complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_check_value( MX_PCCD_170170 *pccd_170170,
				unsigned long register_address,
				unsigned long register_value,
				MX_PCCD_170170_REGISTER **register_pointer )
{
	static const char fname[] = "mxd_pccd_170170_check_value()";

	MX_PCCD_170170_REGISTER *reg;

	if ( register_address < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal register address %lu.", register_address );
	} else
	if ( register_address >= pccd_170170->num_registers ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal register address %lu.", register_address );
	}

	reg = &(pccd_170170->register_array[register_address]);

	if ( reg->read_only ) {
		return mx_error( MXE_READ_ONLY, fname,
			"Register %lu is read only.", register_address );
	}

	if ( reg->power_of_two ) {
		if ( mx_is_power_of_two( register_value ) == FALSE ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested value %lu for register '%s' (%lu) "
			"is not valid since the value is not a power of two.",
				register_value, reg->name, register_address );
		}
	}

	if ( register_value < reg->minimum ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested value %lu for register '%s' (%lu) "
		"is less than the minimum allowed value of %lu.",
			register_value, reg->name,
			register_address, reg->minimum );
	}

	if ( register_value > reg->maximum ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested value %lu for register '%s' (%lu) "
		"is greater than the maximum allowed value of %lu.",
			register_value, reg->name,
			register_address, reg->maximum );
	}

	if ( register_pointer != (MX_PCCD_170170_REGISTER **) NULL ) {
		*register_pointer = reg;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_init_register( MX_PCCD_170170 *pccd_170170,
				long register_index,
				unsigned long register_value,
				mx_bool_type read_only,
				mx_bool_type power_of_two,
				unsigned long minimum,
				unsigned long maximum )
{
	static const char fname[] = "mxd_pccd_170170_init_register()";

	MX_PCCD_170170_REGISTER *reg;
	long register_address;
	MX_RECORD_FIELD *register_field;
	mx_status_type mx_status;

	register_address = register_index - MXLV_PCCD_170170_DH_BASE;

	if ( register_address < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal register address %ld.", register_address );
	} else
	if ( register_address >= pccd_170170->num_registers ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal register address %ld.", register_address );
	}

	reg = &(pccd_170170->register_array[register_address]);

	reg->value        = register_value;
	reg->read_only    = read_only;
	reg->power_of_two = power_of_two;
	reg->minimum      = minimum;
	reg->maximum      = maximum;
	reg->name         = NULL;

	mx_status = mx_get_field_by_label_value( pccd_170170->record,
						register_index,
						&register_field );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	reg->name = register_field->name;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_simulated_cl_command( MX_PCCD_170170 *pccd_170170,
					char *command,
					char *response,
					size_t max_response_length,
					int debug_flag )
{
	static const char fname[] = "mxd_pccd_170170_simulated_cl_command()";

	MX_PCCD_170170_REGISTER *reg;
	unsigned long register_address, register_value, combined_value;
	int num_items;
	mx_status_type mx_status;

	switch( command[0] ) {
	case 'R':
		num_items = sscanf( command, "R%lu", &register_address );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Illegal command '%s' sent to detector '%s'.",
				command, pccd_170170->record->name );
		}

		if ( register_address >= MX_PCCD_170170_NUM_REGISTERS ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Illegal register address %lu.",
				register_address );
		}

		reg = &(pccd_170170->register_array[register_address]);

		snprintf( response, max_response_length, "S%05lu", reg->value );
		break;
	case 'W':
		num_items = sscanf( command, "W%lu", &combined_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Illegal command '%s' sent to detector '%s'.",
				command, pccd_170170->record->name );
		}

		register_address = combined_value / 100000;
		register_value   = combined_value % 100000;

		mx_status = mxd_pccd_170170_check_value( pccd_170170,
							register_address,
							register_value,
							&reg );

		if ( mx_status.code != MXE_SUCCESS ) {
			strlcpy( response, "E", max_response_length );
		} else {
			strlcpy( response, "X", max_response_length );

			reg->value = register_value;
		}
		break;
	default:
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxp_pccd_170170_epix_save_start_timespec( MX_PCCD_170170 *pccd_170170 )
{
	static const char fname[] =
			"mxp_pccd_170170_epix_save_start_timespec()";

	MX_EPIX_XCLIB *xclib;
	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	struct timespec absolute_timespec, relative_timespec;

	epix_xclib_vinput = pccd_170170->video_input_record->record_type_struct;

	if ( epix_xclib_vinput == (MX_EPIX_XCLIB_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPIX_XCLIB_VIDEO_INPUT pointer "
		"for record '%s' is NULL.",
			pccd_170170->video_input_record->name );
	}
	if ( epix_xclib_vinput->xclib_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The xclib_record pointer for record '%s' is NULL.",
			epix_xclib_vinput->record->name );
	}

	xclib = epix_xclib_vinput->xclib_record->record_type_struct;

	if ( xclib == (MX_EPIX_XCLIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPIX_XCLIB pointer for record '%s' is NULL.",
			epix_xclib_vinput->xclib_record->name );
	}

	relative_timespec = mx_high_resolution_time();

	absolute_timespec = mx_add_high_resolution_times(
					xclib->system_boot_timespec,
					relative_timespec );

	xclib->sequence_start_timespec = absolute_timespec;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s:\n"
		"sequence_start_timespec = (%lu,%ld),\n"
		"system_boot_timespec    = (%lu,%ld),\n"
		"relative_timespec       = (%lu,%ld)",
			fname, xclib->sequence_start_timespec.tv_sec,
			xclib->sequence_start_timespec.tv_nsec,
			xclib->system_boot_timespec.tv_sec,
			xclib->system_boot_timespec.tv_nsec,
			relative_timespec.tv_sec,
			relative_timespec.tv_nsec));
#endif
	return MX_SUCCESSFUL_RESULT;
}

/* PCCD-170170 sequence time formulas as of October 5, 2007. */

static mx_status_type
mxd_pccd_170170_compute_sequence_times( MX_AREA_DETECTOR *ad,
					MX_PCCD_170170 *pccd_170170 )
{
#if MXD_PCCD_170170_DEBUG_SEQUENCE_TIMES
	static const char fname[] = "mxd_pccd_170170_compute_sequence_times()";
#endif

	MX_SEQUENCE_PARAMETERS *sp;
	mx_bool_type high_speed;
	unsigned long i, numframes;
	unsigned long N, n, control_register;	/* C is case sensitive. */
	unsigned long sumlimit;
	unsigned long linenum, pixnum, linesrd, pixrd, lbin, pixbin;
	unsigned long nlsi, mtbe, mtshut, nsi, nsc;
	unsigned long tbe_raw, tpre_raw, tshut_raw, tpost_raw;
	double tbe, tpre, tshut, tpost;
	double vshift, vshiftbin, vshift_half, hshift_hs, hshiftbin, hshift_ln;
	double tbe_sum, tbe_product, tshut_sum, tshut_product;
	double exposure_time, gap_time, exposure_multiplier, gap_multiplier;
	double vertical_shift_time, frame_time;
	mx_status_type mx_status;

	sp = &(ad->sequence_parameters);

	/* Suppress GCC uninitialized variable warning. */

	tbe = tpre = tshut = tpost = 0.0;

	/* Read out the necessary register values from the detector head. */

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_CONTROL,
				&control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( control_register & MXF_PCCD_170170_HIGH_SPEED ) {
		high_speed = TRUE;
	} else {
		high_speed = FALSE;
	}

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_PHYSICAL_LINES_IN_QUADRANT,
				&linenum );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_PHYSICAL_PIXELS_IN_QUADRANT,
				&pixnum );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_LINES_READ_IN_QUADRANT,
				&linesrd );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_PIXELS_READ_IN_QUADRANT,
				&pixrd );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_LINE_BINNING,
				&lbin );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_PIXEL_BINNING,
				&pixbin );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG_SEQUENCE_TIMES
	MX_DEBUG(-2,
	("%s: sequence_type = %ld, control_register = %#lx, high_speed = %d",
		fname, sp->sequence_type, control_register, (int) high_speed));
	MX_DEBUG(-2,
	("%s: linenum = %ld, pixnum = %ld, linesrd = %ld, pixrd = %ld",
		fname, linenum, pixnum, linesrd, pixrd));
	MX_DEBUG(-2,("%s: lbin = %ld, pixbin = %ld", fname, lbin, pixbin));
#endif

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_INITIAL_DELAY_TIME,
				&tpre_raw );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	tpre = 10.0e-6 * (double) tpre_raw;

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_EXPOSURE_TIME,
				&tshut_raw );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	tshut = 1.0e-3 * (double) tshut_raw;

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_READOUT_DELAY_TIME,
				&tpost_raw );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	tpost = 10.0e-6 * (double) tpost_raw;

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_GAP_TIME,
				&tbe_raw );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	tbe = 1.0e-3 * (double) tbe_raw;

#if MXD_PCCD_170170_DEBUG_SEQUENCE_TIMES
	MX_DEBUG(-2,("%s: tshut = %g, tbe = %g, tpre = %g, tpost = %g",
			fname, tshut, tbe, tpre, tpost));
#endif

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE,
				&numframes );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG_SEQUENCE_TIMES
	MX_DEBUG(-2,("%s: numframes = %ld", fname, numframes));
#endif

	if ( ( sp->sequence_type == MXT_SQ_SUBIMAGE )
	  || ( sp->sequence_type == MXT_SQ_STREAK_CAMERA )
	  || ( sp->sequence_type == MXT_SQ_GEOMETRICAL ) )
	{
		mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER,
				&mtshut );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_GAP_MULTIPLIER,
				&mtbe );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_SUBFRAME_SIZE,
				&nlsi );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_SUBIMAGES_PER_READ,
				&nsi );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_STREAK_MODE_LINES,
				&nsc );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXD_PCCD_170170_DEBUG_SEQUENCE_TIMES
		MX_DEBUG(-2,("%s: mtshut = %ld, mtbe = %ld",
			fname, mtshut, mtbe));
		MX_DEBUG(-2,("%s: nlsi = %ld, nsi = %ld, nsc = %ld",
			fname, nlsi, nsi, nsc));
#endif
	}

	vshift      = 100.0e-6;
	vshiftbin   = 60.0e-6;
	vshift_half = 30.0e-6;
	hshift_hs   = 360.0e-9;
	hshift_ln   = 1.25e-6;
	hshiftbin   = 300.0e-9;

	/******************************************************************
	 *           Detector sequence time calculation formulas          *
	 ******************************************************************/

	switch( sp->sequence_type ) {
	case MXT_SQ_STREAK_CAMERA:
	    N = nsc + linenum - 1L;

	    if ( lbin == 2 ) {
		sumlimit = N / 2L;
	    } else {
		sumlimit = N;
	    }

	    tbe_sum = 0.0;
	    tbe_product = tbe;

	    for ( n = 1; n <= (sumlimit-1); n++ ) {
		tbe_sum = tbe_sum + tbe_product;

		tbe_product = tbe_product * ( mtbe + 1 );
	    }

	    tshut_sum = 0.0;
	    tshut_product = tshut;

	    for ( n = 1; n <= sumlimit; n++ ) {
		tshut_sum = tshut_sum + tshut_product;

		tshut_product = tshut_product * ( mtshut + 1 );
	    }

#if MXD_PCCD_170170_DEBUG_SEQUENCE_TIMES
	    MX_DEBUG(-2,("%s: STREAK: N = %ld, sumlimit = %ld",
		fname, N, sumlimit));
	    MX_DEBUG(-2,("%s: STREAK: tshut_sum = %g, tbe_sum = %g",
		fname, tshut_sum, tbe_sum));
#endif

	    ad->sequence_start_delay = tpre;

	    ad->total_acquisition_time = tshut_sum + tbe_sum;

	    if ( high_speed ) {		/* High Speed Mode */

		if ( (lbin == 1) && (pixbin == 1) ) {
		    ad->detector_readout_time =
			( tpre + tpost + vshift + hshiftbin
					+ hshift_hs * (pixnum - 1) ) * N;
		} else
		if ( (lbin == 2) && (pixbin == 1) ) {
		    ad->detector_readout_time =
			( tpre + tpost + vshift + vshiftbin + hshiftbin
					+ hshift_hs * (pixnum - 1) ) * (N/2);
		} else
		if ( (lbin == 1) && (pixbin < 16) ) {
		    ad->detector_readout_time =
			( tpre + tpost + vshift + hshift_hs + hshiftbin
				+ (hshift_hs + hshiftbin * (pixbin - 1))
					* ((pixnum - 2)/ (double) pixbin) ) * N;
		} else
		if ( (lbin == 1) && (pixbin >= 16) ) {
		    ad->detector_readout_time =
			( tpre + tpost + vshift + hshift_hs + hshiftbin
				+ (hshift_hs + hshiftbin * 7)
					* (((pixnum - 2) - pixrd)/ 8.0)
				+ (hshift_hs + hshiftbin * (pixbin - 1))
					* (pixrd / (double) pixbin) ) * N;
		} else
		if ( (lbin == 2) && (pixbin < 16) ) {
		    ad->detector_readout_time =
			( tpre + tpost + vshift + hshift_hs + hshiftbin
				+ (hshift_hs + hshiftbin * (pixbin - 1))
					* ((pixnum - 2)/ (double) pixbin) )
			* (N/2);
		} else
		if ( (lbin == 2) && (pixbin >= 16) ) {
		    ad->detector_readout_time =
			( tpre + tpost + vshift + hshift_hs + hshiftbin
				+ (hshift_hs + hshiftbin * 7)
					* (((pixnum - 2) - pixrd)/ 8.0)
				+ (hshift_hs + hshiftbin * (pixbin - 1))
					* (pixrd / (double) pixbin) )
			* (N/2);
		}

	    } else {			/* Low Noise Mode */

		if ( (lbin == 1) && (pixbin == 1) ) {
		    ad->detector_readout_time =
			( tpre + tpost + vshift + hshiftbin
					+ hshift_ln * (pixnum - 1) ) * N;
		} else
		if ( (lbin == 2) && (pixbin == 1) ) {
		    ad->detector_readout_time =
			( tpre + tpost + vshift + vshiftbin + hshiftbin
					+ hshift_ln * (pixnum - 1) ) * (N/2);
		} else
		if ( (lbin == 1) && (pixbin < 16) ) {
		    ad->detector_readout_time =
			( tpre + tpost + vshift + hshift_ln + hshiftbin
				+ (hshift_ln + hshiftbin * (pixbin - 1))
					* ((pixnum - 2)/ (double) pixbin) ) * N;
		} else
		if ( (lbin == 1) && (pixbin >= 16) ) {
		    ad->detector_readout_time =
			( tpre + tpost + vshift + hshift_ln + hshiftbin
				+ (hshift_ln + hshiftbin * 7)
					* (((pixnum - 2) - pixrd)/ 8.0)
				+ (hshift_hs + hshiftbin * (pixbin - 1))
					* (pixrd / (double) pixbin) ) * N;
		} else
		if ( (lbin == 2) && (pixbin < 16) ) {
		    ad->detector_readout_time =
			( tpre + tpost + vshift + hshift_ln + hshiftbin
				+ (hshift_ln + hshiftbin * (pixbin - 1))
					* ((pixnum - 2)/ (double) pixbin) )
			* (N/2);
		} else
		if ( (lbin == 2) && (pixbin >= 16) ) {
		    ad->detector_readout_time =
			( tpre + tpost + vshift + hshift_ln + hshiftbin
				+ (hshift_ln + hshiftbin * 7)
					* (((pixnum - 2) - pixrd)/ 8.0)
				+ (hshift_ln + hshiftbin * (pixbin - 1))
					* (pixrd / (double) pixbin) )
			* (N/2);
		}
	    }

	    ad->total_sequence_time = ad->sequence_start_delay
	    				+ ad->total_acquisition_time
					+ ad->detector_readout_time;
	    break;

	  /*-----------------------------------------------------------------*/

	case MXT_SQ_SUBIMAGE:

	    tbe_sum = 0.0;
	    tbe_product = tbe;

	    for ( n = 1; n <= (nsi-1); n++ ) {
		tbe_sum = tbe_sum + tbe_product;

		tbe_product = tbe_product * ( mtbe + 1 );
	    }

	    tshut_sum = 0.0;
	    tshut_product = tshut;

	    for ( n = 1; n <= nsi; n++ ) {
		tshut_sum = tshut_sum + tshut_product;

		tshut_product = tshut_product * ( mtshut + 1 );
	    }

#if MXD_PCCD_170170_DEBUG_SEQUENCE_TIMES
	    MX_DEBUG(-2,("%s: SUBIMAGE: tshut_sum = %g, tbe_sum = %g",
		fname, tshut_sum, tbe_sum));
#endif
	    vertical_shift_time = vshiftbin * nlsi + tpost + vshift_half;

	    ad->sequence_start_delay = tpre;

	    ad->total_acquisition_time = tbe_sum + tshut_sum
					+ vertical_shift_time * ( nsi - 1 );

	    if ( high_speed ) {		/* High Speed Mode */

		if ( (lbin == 1) && (pixbin == 1) ) {
		    ad->detector_readout_time =
			vshiftbin * (linenum - nsi * nlsi - 1)
			+ (vshift + hshiftbin + hshift_hs * (pixnum - 1))
				* (nsi * nlsi + 1);
		} else
		if ( (lbin > 1) && (pixbin == 1) ) {
		    ad->detector_readout_time =
			vshiftbin * (linenum - nsi * nlsi - 1)
			+ vshift + hshiftbin + hshift_hs * (pixnum - 1)
			+ (vshift + vshiftbin * (lbin - 1) + hshift_hs
				+ hshiftbin + hshift_hs * (pixnum - 2))
					* ( nsi * nlsi / (double) lbin );

		} else
		if ( (lbin == 1) && (pixbin < 16) ) {
		    ad->detector_readout_time =
			vshiftbin * (linenum - nsi * nlsi - 1)
			+ ( vshift + hshift_hs + hshiftbin
				+ (hshift_hs + hshiftbin * (pixbin-1))
					* ((pixnum - 2) / (double) pixbin) )
			    * ( nsi * nlsi + 1 );

		} else
		if ( (lbin == 1) && (pixbin >= 16) ) {
		    ad->detector_readout_time =
			vshiftbin * (linenum - nsi * nlsi - 1)
			+ ( vshift + hshift_hs + hshiftbin
				+ (hshift_hs + hshiftbin * 7)
					* (((pixnum-2) - pixrd)/8.0)
				+ (hshift_hs + hshiftbin * (pixbin - 1))
					* (pixrd / (double) pixbin) )
			    * (nsi * nlsi + 1);

		} else
		if ( (lbin > 1) && (pixbin < 16) ) {
		    ad->detector_readout_time =
			vshiftbin * (linenum - nsi * nlsi - 1)
			+ vshift + hshift_hs + hshiftbin + hshift_hs*(pixnum-2)
			+ ( vshift + vshiftbin * (lbin - 1)
				+ hshift_hs + hshiftbin
				+ (hshift_hs + hshiftbin * (pixbin-1))
					* ((pixnum-2)/ (double) pixbin) )
			    * (nsi * nlsi / (double) lbin);

		} else
		if ( (lbin > 1) && (pixbin >= 16) ) {
		    ad->detector_readout_time =
			vshiftbin * (linenum - nsi * nlsi - 1)
			+ vshift + hshift_hs + hshiftbin + hshift_hs*(pixnum-2)
			+ ( vshift + vshiftbin * (lbin - 1)
				+ hshift_hs + hshiftbin
				+ (hshift_hs + hshiftbin * 7)
					* ((pixnum - 2) - pixrd) / 8.0
				+ (hshift_hs + hshiftbin * (pixbin-1))
					* (pixrd / (double) pixbin) )
			    * (nsi * nlsi / (double) lbin);
		}

	    } else {			/* Low Noise Mode */

		if ( (lbin == 1) && (pixbin == 1) ) {
		    ad->detector_readout_time =
			vshiftbin * (linenum - nsi * nlsi - 1)
			+ ( vshift + hshiftbin + hshift_ln * (pixnum-1) )
				* (nsi * nlsi + 1);

		} else
		if ( (lbin > 1) && (pixbin == 1) ) {
		    ad->detector_readout_time =
			vshiftbin * (linenum - nsi * nlsi - 1)
			+ vshift + hshiftbin + hshift_ln * (pixnum-1)
			+ ( vshift + vshiftbin * (lbin-1)
				+ hshift_ln + hshiftbin + hshift_ln*(pixnum-2) )
			    * (nsi * nlsi / (double) lbin);
		} else
		if ( (lbin == 1) && (pixbin < 16) ) {
		    ad->detector_readout_time =
			vshiftbin * (linenum - nsi * nlsi - 1)
			+ ( vshift + hshift_ln + hshiftbin
				+ (hshift_ln + hshiftbin * (pixbin-1))
					*((pixnum-2)/ (double) pixbin) )
			    * (nsi * nlsi + 1);
		} else
		if ( (lbin == 1) && (pixbin >= 16) ) {
		    ad->detector_readout_time =
			vshiftbin * (linenum - nsi * nlsi - 1)
			+ ( vshift + hshift_ln + hshiftbin
				+ (hshift_ln + hshiftbin * 7)
					* (((pixnum-2) - pixrd) / 8.0)
				+ (hshift_ln + hshiftbin * (pixbin-1))
					* (pixrd / (double) pixbin) )
			    * (nsi * nlsi + 1);
		} else
		if ( (lbin > 1) && (pixbin < 16) ) {
		    ad->detector_readout_time =
			vshiftbin * (linenum - nsi * nlsi - 1)
			+ vshift + hshift_ln + hshiftbin + hshift_ln*(pixnum-2)
			+ ( vshift + vshiftbin * (lbin-1)
				+ hshift_ln + hshiftbin
				+ (hshift_ln + hshiftbin*(pixbin-1))
					* ((pixnum-2) / (double) pixbin) )
			    * ( nsi * nlsi / (double) lbin );
		} else
		if ( (lbin > 1) && (pixbin >= 16) ) {
		    ad->detector_readout_time =
			vshiftbin * (linenum - nsi * nlsi - 1)
			+ vshift + hshift_ln + hshiftbin + hshift_ln*(pixnum-2)
			+ ( vshift + vshiftbin * (lbin-1)
				+ hshift_ln + hshiftbin
				+ (hshift_ln + hshiftbin * 7)
					* (((pixnum-2) - pixrd) / 8.0)
				+ (hshift_ln + hshiftbin*(pixbin-1))
					* (pixrd / (double) pixbin) )
			    * (nsi * nlsi / (double) lbin);
		}
	    }

	    ad->total_sequence_time = ad->sequence_start_delay
	    				+ ad->total_acquisition_time
					+ ad->detector_readout_time;
	    break;

	default:	/* Full Frame Mode */

	    /* First compute the detector readout time, since it is
	     * the same for all full frame sequence types.
	     */

	    if ( high_speed ) {		/* High Speed Mode */
		
		if ( (lbin == 1) && (pixbin == 1) ) {
		    ad->detector_readout_time =
		    	( vshift + (hshift_hs + hshiftbin)
			+ hshift_hs*(pixnum - 2) )*(linenum);
		} else
		if ( (lbin > 1) && (pixbin == 1) ) {

		    ad->detector_readout_time =
		    	( vshift + (hshift_hs + hshiftbin)
			+ hshift_hs * (pixnum - 2) )
				* (linenum - (linenum - 6))
			+ ( (vshift + vshiftbin) + (hshift_hs + hshiftbin)
			+ hshift_hs *(pixnum - 2) )
				* (((linenum - 6) - linesrd) / 2)
			+ ( (vshift + vshiftbin*(lbin - 1))
			+ (hshift_hs + hshiftbin)
			+ hshift_hs*(pixnum - 2) ) * (linesrd / lbin);
		} else
		if ( (lbin == 1) && (pixbin < 16) ) {

		    ad->detector_readout_time =
		        ( vshift + (hshift_hs + hshiftbin)
					+ (hshift_hs + hshiftbin*(pixbin - 1))
						*((pixnum - 2) / pixbin) )
				* linenum;

		} else
		if ( (lbin == 1) && (pixbin >= 16) ) {

		    ad->detector_readout_time =
			( vshift + (hshift_hs + hshiftbin)
				+ (hshift_hs + hshiftbin*(7))
						*(((pixnum - 2) - pixrd) / 8)
				+ (hshift_hs + hshiftbin*(pixbin - 1))
					* (pixrd / pixbin) )*(linenum);

		} else
		if ( (lbin > 1) && (pixbin < 16) ) {

		    ad->detector_readout_time =
		    	(vshift + (hshift_hs + hshiftbin)
			+ (hshift_hs + hshiftbin*(pixbin - 1))
				*((pixnum - 2) / pixbin))
				*(linenum - (linenum - 6))
			+ ((vshift + vshiftbin) + (hshift_hs + hshiftbin)
			+ (hshift_hs + hshiftbin*(pixbin - 1))
			*((pixnum - 2) / pixbin))*(((linenum - 6) - linesrd)/2)
			+ ((vshift + vshiftbin*(lbin - 1))
			+ (hshift_hs + hshiftbin) + (hshift_hs
			+ hshiftbin*(pixbin - 1))*((pixnum - 2) / pixbin))
				*(linesrd / lbin);

		} else
		if ( (lbin > 1) && (pixbin >= 16 ) ) {

		    ad->detector_readout_time =
		        (vshift + (hshift_hs + hshiftbin)
			+ (hshift_hs + hshiftbin*(7))*(((pixnum - 2) - pixrd)/8)
			+ (hshift_hs + hshiftbin*(pixbin - 1))*(pixrd / pixbin))
				* (linenum - (linenum - 6))
			+ ((vshift + vshiftbin) + (hshift_hs + hshiftbin)
			+ (hshift_hs + hshiftbin*(7))*(((pixnum - 2) - pixrd)/8)
			+ (hshift_hs + hshiftbin*(pixbin - 1))*(pixrd / pixbin))
				*(((linenum - 6) - linesrd) / 2)
			+ ((vshift + vshiftbin*(lbin - 1))
			+ (hshift_hs + hshiftbin)
			+ (hshift_hs + hshiftbin*(7))*(((pixnum - 2) - pixrd)/8)
			+ (hshift_hs + hshiftbin*(pixbin - 1))*(pixrd / pixbin))
				*(linesrd / lbin);
		}
	    } else {			/* Low Noise Mode */

		if ( (lbin == 1) && (pixbin == 1) ) {

		    ad->detector_readout_time =
			(vshift + (hshift_ln + hshiftbin)
			+ hshift_ln *(pixnum - 2))*(linenum);

		} else
		if ( (lbin > 1) && (pixbin == 1) ) {

		    ad->detector_readout_time =
			(vshift + (hshift_ln + hshiftbin)
			+ hshift_ln *(pixnum - 2))*(linenum - (linenum - 6))
			+ ((vshift + vshiftbin) + (hshift_ln + hshiftbin)
			+ hshift_ln *(pixnum - 2))*(((linenum - 6) - linesrd)/2)
			+ ((vshift + vshiftbin*(lbin - 1))
			+ (hshift_ln + hshiftbin) + hshift_ln *(pixnum - 2))
				*(linesrd / lbin);
		
		} else
		if ( (lbin == 1) && (pixbin < 16) ) {

		    ad->detector_readout_time =
			(vshift + (hshift_ln + hshiftbin)
			+ (hshift_ln + hshiftbin*(pixbin - 1))
				*((pixnum - 2) / pixbin))*(linenum);

		} else
		if ( (lbin == 1) && (pixbin >= 16) ) {

		    ad->detector_readout_time =
			(vshift + (hshift_ln + hshiftbin)
			+ (hshift_ln + hshiftbin*(7))*(((pixnum - 2) - pixrd)/8)
			+ (hshift_ln + hshiftbin*(pixbin - 1))
				*(pixrd / pixbin))*(linenum);

		} else
		if ( (lbin > 1) && (pixbin < 16) ) {

		    ad->detector_readout_time =
			(vshift + (hshift_ln + hshiftbin)
			+ (hshift_ln + hshiftbin*(pixbin - 1))
				*((pixnum - 2) / pixbin))
				*(linenum - (linenum - 6))
			+ ((vshift + vshiftbin) + (hshift_ln + hshiftbin)
			+ (hshift_ln + hshiftbin*(pixbin - 1))
				*((pixnum - 2) / pixbin))
				*(((linenum - 6) - linesrd) / 2)
			+ ((vshift + vshiftbin*(lbin - 1))
			+ (hshift_ln + hshiftbin)
			+ (hshift_ln + hshiftbin*(pixbin - 1))
				*((pixnum - 2) / pixbin))*(linesrd / lbin);

		} else
		if ( (lbin > 1) && (pixbin >= 16) ) {

		    ad->detector_readout_time =
			(vshift + (hshift_ln + hshiftbin)
			+ (hshift_ln + hshiftbin*(7))*(((pixnum - 2) - pixrd)/8)
			+ (hshift_ln + hshiftbin*(pixbin - 1))
				*(pixrd / pixbin))* (linenum - (linenum - 6))
			+ ((vshift + vshiftbin) + (hshift_ln + hshiftbin)
			+ (hshift_ln + hshiftbin*(7))*(((pixnum - 2) - pixrd)/8)
			+ (hshift_ln + hshiftbin*(pixbin - 1))*(pixrd / pixbin))
				*(((linenum - 6) - linesrd) / 2)
			+ ((vshift + vshiftbin*(lbin - 1))
			+ (hshift_ln + hshiftbin) + (hshift_ln + hshiftbin*(7))
				*(((pixnum - 2) - pixrd) / 8)
			+ (hshift_ln + hshiftbin*(pixbin - 1))*(pixrd / pixbin))
				*(linesrd / lbin);
		}
	    }

	    /* Then compute the acquisition and sequence times. */

	    ad->sequence_start_delay = 0.0;

	    exposure_time = tshut;
	    gap_time      = tbe;

#if MXD_PCCD_170170_DEBUG_SEQUENCE_TIMES
	    MX_DEBUG(-2,("%s: exposure_time = %g, gap_time = %g",
	    	fname, exposure_time, gap_time));
	    MX_DEBUG(-2,("%s: detector_readout_time = %g",
	    	fname, ad->detector_readout_time));
	    MX_DEBUG(-2,("%s: numframes = %ld", fname, numframes));
#endif

	    switch( sp->sequence_type ) {
	    case MXT_SQ_ONE_SHOT:
	    case MXT_SQ_CONTINUOUS:
	        ad->total_acquisition_time = exposure_time;
		ad->total_sequence_time = ad->total_acquisition_time
					+ ad->detector_readout_time;
		break;
	    case MXT_SQ_MULTIFRAME:
	    case MXT_SQ_CIRCULAR_MULTIFRAME:
	        frame_time = exposure_time + gap_time
					+ ad->detector_readout_time;
		ad->total_acquisition_time = frame_time * (double) numframes;
		ad->total_sequence_time = ad->total_acquisition_time;
	    	break;
	    case MXT_SQ_STROBE:
	    	frame_time = exposure_time + ad->detector_readout_time;
	    	ad->total_acquisition_time = frame_time * (double) numframes;
		ad->total_sequence_time = ad->total_acquisition_time;
	    	break;
	    case MXT_SQ_BULB:
	    	frame_time = ad->detector_readout_time;
	    	ad->total_acquisition_time = frame_time * (double) numframes;
		ad->total_sequence_time = ad->total_acquisition_time;
	    	break;
	    case MXT_SQ_GEOMETRICAL:
	    	exposure_multiplier = 1.0 + 0.0039 * (double) mtshut;
		gap_multiplier      = 1.0 + 0.0039 * (double) mtbe;

#if MXD_PCCD_170170_DEBUG_SEQUENCE_TIMES
		MX_DEBUG(-2,("%s: mtshut = %ld, mtbe = %ld",
			fname, mtshut, mtbe));
		MX_DEBUG(-2,
		("%s: exposure_multiplier = %g, gap_multiplier = %g",
			fname, exposure_multiplier, gap_multiplier));
#endif

	    	ad->total_acquisition_time = 0.0;

		for ( i = 0; i < numframes; i++ ) {

#if MXD_PCCD_170170_DEBUG_SEQUENCE_TIMES
			MX_DEBUG(-2,
("%s: i = %ld, exposure_time = %g, gap_time = %g, detector_readout_time = %g",
		fname, i, exposure_time, gap_time, ad->detector_readout_time));
#endif
			ad->total_acquisition_time += ad->detector_readout_time;

			ad->total_acquisition_time += exposure_time;
			ad->total_acquisition_time += gap_time;

			exposure_time *= exposure_multiplier;
			gap_time *= gap_multiplier;

#if MXD_PCCD_170170_DEBUG_SEQUENCE_TIMES
			MX_DEBUG(-2,("%s: i = %ld, total_acquisition_time = %g",
				fname, i, ad->total_acquisition_time));
#endif
		}

		ad->total_sequence_time = ad->total_acquisition_time;
		break;
	    }
	    ad->sequence_start_delay = 0.0;

	    ad->total_sequence_time =
	    	ad->total_acquisition_time + ad->detector_readout_time;

	    break;
	}


#if MXD_PCCD_170170_DEBUG_SEQUENCE_TIMES
	MX_DEBUG(-2,("%s: ad->sequence_start_delay = %g",
			fname, ad->sequence_start_delay));
	MX_DEBUG(-2,("%s: ad->total_acquisition_time = %g",
			fname, ad->total_acquisition_time));
	MX_DEBUG(-2,("%s: ad->detector_readout_time = %g",
			fname, ad->detector_readout_time));
	MX_DEBUG(-2,("%s: ad->total_sequence_time = %g",
			fname, ad->total_sequence_time));
#endif
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_pccd_170170_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_record_fields;
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_type( record_type,
					&num_record_fields,
					&record_field_defaults,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_pccd_170170_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_PCCD_170170 *pccd_170170;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	pccd_170170 = (MX_PCCD_170170 *)
				malloc( sizeof(MX_PCCD_170170) );

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_PCCD_170170 structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = pccd_170170;
	record->class_specific_function_list = 
			&mxd_pccd_170170_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	pccd_170170->record = record;

	pccd_170170->first_dh_command = TRUE;

	pccd_170170->horiz_descramble_factor = -1;
	pccd_170170->vert_descramble_factor = -1;

	/* Set the default file format. */

	ad->frame_file_format = MXT_IMAGE_FILE_SMV;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_delete_record( MX_RECORD *record )
{
	static const char fname[] = "mxd_pccd_170170_delete_record()";

	MX_AREA_DETECTOR *ad;
	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	pccd_170170 = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pccd_170170 != NULL ) {
		if ( pccd_170170->raw_frame != NULL ) {
			mx_free( pccd_170170->raw_frame );
		}
		mx_free( pccd_170170 );
	}

	if ( ad != NULL ) {
		mx_free( ad );
	}

	if ( record != NULL ) {
		mx_free( record );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pccd_170170_open()";

	MX_AREA_DETECTOR *ad;
	MX_PCCD_170170 *pccd_170170;
	MX_RECORD *video_input_record;
	MX_VIDEO_INPUT *vinput;
	long vinput_framesize[2];
	long ad_binsize[2];
	unsigned long ad_flags, pccd_flags;
	unsigned long controller_fpga_version, comm_fpga_version;
	unsigned long control_register_value;
	long x_framesize, y_framesize;
	size_t array_size;
	long master_clock;
	struct timespec hrt;
	mx_bool_type camera_is_master;
	mx_status_type mx_status;

	pccd_170170 = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	ad_flags = ad->area_detector_flags;

	pccd_flags = pccd_170170->pccd_170170_flags;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: area_detector_flags = %#lx", fname, ad_flags));
	MX_DEBUG(-2,("%s: pccd_170170_flags = %#lx", fname, pccd_flags));
#endif

	if ( pccd_flags & MXF_PCCD_170170_SUPPRESS_DESCRAMBLING ) {
		mx_warning( "Area detector '%s' will not descramble "
			"images from the camera head.",
				record->name );
	}
	if ( pccd_flags & MXF_PCCD_170170_USE_DETECTOR_HEAD_SIMULATOR ) {
		mx_warning( "Area detector '%s' will use "
	"an Aviex detector head simulator instead of a real detector head.",
				record->name );
	}

	video_input_record = pccd_170170->video_input_record;

	if ( video_input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The video_input_record pointer for area detector '%s' is NULL.",
			record->name );
	}

	vinput = video_input_record->record_class_struct;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VIDEO_INPUT pointer for video input record '%s' "
		"used by area detector '%s' is NULL.",
			video_input_record->name, record->name );
	}

	/* Automatic loading of image frames does not make any sense for
	 * this detector.
	 */

	if ( ad_flags & MXF_AD_LOAD_FRAME_AFTER_ACQUISITION ) {
	    ad->area_detector_flags &= (~MXF_AD_LOAD_FRAME_AFTER_ACQUISITION);

	    (void) mx_error( MXE_UNSUPPORTED, fname,
		"Automatic loading of image frames from disk files "
		"after an acquisition sequence "
		"is not supported for area detector '%s', since that "
		"would result in the loss of all data from the detector.  "
		"The load frame flag has been turned off.",
			record->name );
	}

	/* See if the user has requested the saving of image frames by MX. */

	if ( ad_flags & MXF_AD_SAVE_FRAME_AFTER_ACQUISITION ) {

		MX_DEBUG(-2,("%s: Enabling automatic saving of image frames "
		"for area detector '%s'.", fname, record->name ));

		mx_status =
		    mx_area_detector_setup_datafile_management( record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Initialize the geometrical mask frames. */

	pccd_170170->geometrical_mask_frame = NULL;
	pccd_170170->rebinned_geometrical_mask_frame = NULL;

	/* Set a limit on the range of values for
	 * the flood field scaling factor.
	 */

	ad->flood_field_scale_max = 10.0;
	ad->flood_field_scale_min = 0.1;

	/* Set the step size in seconds per step
	 * for the exposure time and the gap time.
	 */

	pccd_170170->exposure_and_gap_step_size = 0.001;

	/* Make sure the internal trigger output is low. */

	mx_status = mx_digital_output_write(
				pccd_170170->internal_trigger_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off the buffer overrun flag. */

	pccd_170170->buffer_overrun = FALSE;

	/* Turn off the flag that tells the subimage and streak camera modes
	 * to use the top half of the detector rather than the bottom half.
	 */

	pccd_170170->use_top_half_of_detector = FALSE;

	/* If requested, load the bias lookup table. */

	if ( strlen(pccd_170170->bias_lookup_table_filename) > 0 ) {

		mx_status = mxd_pccd_170170_load_bias_lookup_table(pccd_170170);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		pccd_170170->bias_lookup_table = NULL;
	}

	/* Initialize data structures used to specify attributes
	 * of each detector head register.
	 */

	array_size = MX_PCCD_170170_NUM_REGISTERS
			* sizeof(MX_PCCD_170170_REGISTER);

	/* Allocate memory for the simulated register array. */

	pccd_170170->num_registers = MX_PCCD_170170_NUM_REGISTERS;

	pccd_170170->register_array = malloc( array_size );

	if ( pccd_170170->register_array == NULL ) {
		pccd_170170->num_registers = 0;

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d element array "
		"of simulated detector head registers for detector '%s.'",
			MX_PCCD_170170_NUM_REGISTERS,
			pccd_170170->record->name );
	}

	memset( pccd_170170->register_array, 0, array_size );

	/* Initialize register attributes. */

	INIT_REGISTER( MXLV_PCCD_170170_DH_CONTROL,
					0x184, FALSE, FALSE, 0,  0xffff );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OVERSCANNED_PIXELS_PER_LINE,
					4,     FALSE, FALSE, 1,  2048 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_PHYSICAL_LINES_IN_QUADRANT,
					1046,  FALSE, FALSE, 1,  8192 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_PHYSICAL_PIXELS_IN_QUADRANT,
					1050,  FALSE, FALSE, 1,  8192 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_LINES_READ_IN_QUADRANT,
					1024,  FALSE, FALSE, 1,  8192 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_PIXELS_READ_IN_QUADRANT,
					1024,  FALSE, FALSE, 1,  8192 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_INITIAL_DELAY_TIME,
					0,     FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_EXPOSURE_TIME,
					0,     FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_READOUT_DELAY_TIME,
					0,     FALSE, FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE,
					1,     FALSE, FALSE, 1,  69999 );

#if 0
	INIT_REGISTER( MXLV_PCCD_170170_DH_GAP_TIME,
					1,     FALSE, FALSE, 0,  65535 );
#else
	INIT_REGISTER( MXLV_PCCD_170170_DH_GAP_TIME,
					1,     FALSE, FALSE, 1,  65535 );
#endif

	INIT_REGISTER( MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER,
					0,     FALSE, FALSE, 0,  255 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_GAP_MULTIPLIER,
					0,     FALSE, FALSE, 0,  255 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_CONTROLLER_FPGA_VERSION,
					17010, TRUE,  FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_LINE_BINNING,
					1,     FALSE, TRUE,  1,  128 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_PIXEL_BINNING,
					1,     FALSE, TRUE,  1,  128 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_SUBFRAME_SIZE,
					1024,  FALSE, TRUE,  16, 1024 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_SUBIMAGES_PER_READ,
					1,     FALSE, FALSE, 1,  128 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_STREAK_MODE_LINES,
					1,     FALSE, FALSE, 1,  65535 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_COMM_FPGA_VERSION,
					17010, TRUE,  FALSE, 0,  65535 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_A1,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_A2,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_A3,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_A4,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_B1,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_B2,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_B3,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_B4,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_C1,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_C2,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_C3,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_C4,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_D1,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_D2,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_D3,
					0,     FALSE, FALSE, 0,  65534 );

	INIT_REGISTER( MXLV_PCCD_170170_DH_OFFSET_D4,
					0,     FALSE, FALSE, 0,  65534 );

	/* Check to find out the firmware versions that are being used by
	 * the detector head.
	 */

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_CONTROLLER_FPGA_VERSION,
				&controller_fpga_version );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		break;

	case MXE_TIMED_OUT:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The detector controller for area detector '%s' is not "
			"responding to commands.  Please verify that the "
			"detector is turned on and configured correctly.",
				record->name );
		break;

	default:
		return mx_status;
		break;
	}

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
				MXLV_PCCD_170170_DH_COMM_FPGA_VERSION,
				&comm_fpga_version );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: controller FPGA version = %lu",
			fname, controller_fpga_version ));
	MX_DEBUG(-2,("%s: communications FPGA version = %lu",
			fname, comm_fpga_version ));
#endif
	/* The PCCD-170170 camera generates 16 bit per pixel images. */

	vinput->bits_per_pixel = 16;
	ad->bits_per_pixel = vinput->bits_per_pixel;

	/* Perform camera model specific initialization */

	switch( record->mx_type ) {
	case MXT_AD_PCCD_170170:

		/* In unbinned mode, the PCCD-170170 camera is configured such
		 * that each line contains 1024 groups of pixels, with 16
		 * pixels per group.  This means that for maximum resolution
		 * in full frame mode, the video input will be configured for
		 * a resolution of 16384 by 1024.  However, we want this to
		 * appear to the user to have a resolution of 4096 by 4096.
		 * Thus we must rescale the resolution as reported by the
		 * video card by multiplying and dividing by appropriate
		 * factors of 4.
		 */

		ad->maximum_framesize[0] = 4096;
		ad->maximum_framesize[1] = 4096;

		pccd_170170->horiz_descramble_factor = 4;
		pccd_170170->vert_descramble_factor = 4;

		pccd_170170->pixel_clock_frequency = 60.0e6;
		break;

	case MXT_AD_PCCD_4824:

		/* The PCCD-4824 camera is similar to the PCCD-170170 above,
		 * but it only has a single CCD chip which has a maximum
		 * image size of 3584 by 2048.  Each line from the detector
		 * contains 1792 groups of pixels with 4 pixels per group.
		 * A full frame image has 1024 lines.  This means that the
		 * maximum resolution of the video card should be 7168 by 1024
		 * and that the descramble factors should be 2.
		 */

		ad->maximum_framesize[0] = 3584;
		ad->maximum_framesize[1] = 2048;

		pccd_170170->horiz_descramble_factor = 2;
		pccd_170170->vert_descramble_factor  = 2;

#if 0
		/* For the real detector. */

		pccd_170170->pixel_clock_frequency = 45.4545e6;
#else
		/* For the simulator. */

		pccd_170170->pixel_clock_frequency = 50.0e6;
#endif
		break;

	case MXT_AD_PCCD_16080:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"PCCD-16080 specific code for the AVIEX PCCD driver "
			"has not yet been implemented." );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Record '%s' has an MX type of %ld which "
			"is not supported by this driver.",
				record->name, record->mx_type );
		break;
	}

	/* Set the default framesize and binning. */

	ad->framesize[0] = ad->maximum_framesize[0];
	ad->framesize[1] = ad->maximum_framesize[1];

	mx_status = mx_area_detector_set_binsize( record, 1, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Generate a warning if the video board was not able to get
	 * enough memory for a even just a single frame.
	 */

	mx_status = mx_area_detector_get_framesize( record,
						&x_framesize, &y_framesize );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: x_framesize = %ld, y_framesize = %ld",
		fname, x_framesize, y_framesize));
#endif

	if ( ( x_framesize < ad->maximum_framesize[0] )
	  || ( y_framesize < ad->maximum_framesize[1] ) )
	{
		mx_warning( "Area detector '%s' was not able to acquire "
		"enough memory for even a single full frame.  "
		"Desired framesize = (%ld,%ld).  "
		"Actual framesize = (%ld,%ld).  "
		"This can happen if you have run a large number of "
		"application programs before starting MX.  "
		"Generally you can fix this by starting MX "
		"as soon after the computer boots as is possible.",
			record->name,
			ad->maximum_framesize[0],
			ad->maximum_framesize[1],
			x_framesize, y_framesize );
	}

	/* Read the control register so that we can change it. */

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
					MXLV_PCCD_170170_DH_CONTROL,
					&control_register_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: OLD control_register_value = %#lx",
		fname, control_register_value));
#endif

	/* Put the detector head in full frame mode. */

	control_register_value &= (~MXF_PCCD_170170_DETECTOR_READOUT_MASK);

	/* Turn on an initial runt Frame Valid pulse.  This is used to
	 * work around a misfeature of the PIXCI E4 board.  The E4 board
	 * always ignores the first frame sent by the camera after 
	 * starting a new sequence.  EPIX says that this is to protect
	 * against a race condition in their system.  Aviex's solution
	 * is to send an extra runt Frame Valid pulse at the start of
	 * a sequence, so that the frame thrown away by EPIX is a frame
	 * that we do not want anyway.
	 */

	control_register_value |= MXF_PCCD_170170_DUMMY_FRAME_VALID;

	/* If requested, turn on the test mode pattern. */

	if ( pccd_flags & MXF_PCCD_170170_USE_TEST_PATTERN ) {
		control_register_value |= MXF_PCCD_170170_TEST_MODE_ON;

		mx_warning( "Area detector '%s' will return a test image "
			"instead of taking a real image.",
				record->name );
	} else {
		control_register_value &= (~MXF_PCCD_170170_TEST_MODE_ON);
	}

	/* Write out the new control register value. */

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: NEW control_register_value = %#lx",
		fname, control_register_value));
#endif

	mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_CONTROL,
					control_register_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the pixel clock frequency in the video card. */

	mx_status = mx_video_input_set_pixel_clock_frequency(
		video_input_record, pccd_170170->pixel_clock_frequency );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure the video input's external trigger to trigger on
	 * the rising edge of the trigger pulse.
	 */

	mx_status = mx_video_input_set_external_trigger_polarity(
				video_input_record, MXF_VIN_TRIGGER_RISING );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure the video input to use negative Camera Link pulses. */

	mx_status = mx_video_input_set_camera_trigger_polarity(
				video_input_record, MXF_VIN_TRIGGER_LOW );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the video input's initial trigger mode (internal/external/etc) */

	mx_status = mx_video_input_set_trigger_mode( video_input_record,
				pccd_170170->initial_trigger_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Specify whether or not the camera head or the video board is
	 * in charge of generating the trigger pulse for sequences.
	 */

	camera_is_master =
	   (pccd_170170->pccd_170170_flags & MXF_PCCD_170170_CAMERA_IS_MASTER);

	if ( camera_is_master ) {
		master_clock = MXF_VIN_MASTER_CAMERA;
	} else {
		master_clock = MXF_VIN_MASTER_VIDEO_BOARD;
	}

	mx_status = mx_video_input_set_master_clock( video_input_record,
							master_clock );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize area detector parameters. */

	ad->byte_order = mx_native_byteorder();
	ad->header_length = 0;

	mx_status = mx_area_detector_get_image_format( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_bytes_per_pixel( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_bytes_per_frame( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_bits_per_pixel( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize some parameters used to switch between streak camera
	 * mode and other modes.
	 */

	mx_status = mx_video_input_get_framesize( video_input_record,
						&vinput_framesize[0],
						&vinput_framesize[1] );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pccd_170170->vinput_normal_framesize[0] = vinput_framesize[0];
	pccd_170170->vinput_normal_framesize[1] = vinput_framesize[1];

	mx_status = mx_area_detector_get_binsize( record,
						&ad_binsize[0], &ad_binsize[1]);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pccd_170170->normal_binsize[0] = ad_binsize[0];
	pccd_170170->normal_binsize[1] = ad_binsize[1];

	/* Allocate space for a raw frame buffer.  This buffer will be used to
	 * read in the raw pixels from the imaging board before descrambling.
	 */

#if 0
	MX_DEBUG(-2,("%s: Before final call to mx_image_alloc()", fname));

	MX_DEBUG(-2,("%s: &(pccd_170170->raw_frame) = %p",
			fname, &(pccd_170170->raw_frame) ));

	MX_DEBUG(-2,("%s: vinput_framesize = (%lu,%lu)",
			fname, vinput_framesize[0], vinput_framesize[1]));

	MX_DEBUG(-2,("%s: image_format = %ld", fname, ad->image_format ));
	MX_DEBUG(-2,("%s: byte_order = %ld", fname, ad->byte_order ));
	MX_DEBUG(-2,("%s: bytes_per_pixel = %g", fname, ad->bytes_per_pixel));
	MX_DEBUG(-2,("%s: header_length = %ld", fname, ad->header_length));
	MX_DEBUG(-2,("%s: bytes_per_frame = %ld", fname, ad->bytes_per_frame));
#endif

#if MXD_PCCD_170170_DEBUG_MX_IMAGE_ALLOC
	MX_DEBUG(-2,
	("%s: Invoking mx_image_alloc() for pccd_170170->raw_frame = %p",
		fname, pccd_170170->raw_frame ));
#endif

	mx_status = mx_image_alloc( &(pccd_170170->raw_frame),
					vinput_framesize[0],
					vinput_framesize[1],
					ad->image_format,
					ad->byte_order,
					ad->bytes_per_pixel,
					ad->header_length,
					ad->bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MXIF_ROW_BINSIZE(pccd_170170->raw_frame) = 1;
	MXIF_COLUMN_BINSIZE(pccd_170170->raw_frame) = 1;
	MXIF_BITS_PER_PIXEL(pccd_170170->raw_frame) = ad->bits_per_pixel;

	pccd_170170->old_framesize[0] = -1;
	pccd_170170->old_framesize[1] = -1;

	pccd_170170->sector_array = NULL;

	/* Load the image correction files. */

	mx_status = mx_area_detector_load_correction_files( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the detector to one-shot mode with an exposure time
	 * of 1 second.
	 */

	mx_status = mx_area_detector_set_one_shot_mode( record, 1.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pccd_flags & MXF_PCCD_170170_TEST_DEZINGER ) {

		/* If we are doing a dezinger test, provide a seed value for
		 * the random number generator.  The numbers that we generate
		 * here do not need to be very random, so we just use the
		 * computer's high resolution clock to generate the seed.
		 */
#if 1
		hrt.tv_sec = 1;
#else
		hrt = mx_high_resolution_time();
#endif
		srand( hrt.tv_sec );

		ad->dezinger_threshold = 2.0;
	}

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_pccd_170170_resynchronize()";

	MX_AREA_DETECTOR *ad;
	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	pccd_170170 = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	/* Send an 'Initialize CCD Controller' command to the detector head
	 * by toggling the CC4 line.
	 */

#if 0
	mx_status = mx_camera_link_pulse_cc_line(
					pccd_170170->camera_link_record, 4,
					-1, 1000 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	/* Restart the PIXCI library. */

	mx_status = mx_resynchronize_record( pccd_170170->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_arm()";

	MX_PCCD_170170 *pccd_170170;
	MX_VIDEO_INPUT *vinput;
	MX_SEQUENCE_PARAMETERS *sp;
	MX_CLOCK_TICK num_ticks_to_wait, finish_tick;
	double timeout_in_seconds;
	int comparison;
	long num_frames_in_sequence, master_clock;
	unsigned long control_register_value, status_flags;
	mx_bool_type camera_is_master, external_trigger, circular;
	mx_status_type mx_status;

	pccd_170170 = NULL;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pccd_170170->video_input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The video_input_record pointer for area detector '%s' is NULL.",
			ad->record->name );
	}

	DISPLAY_CONTROL_REGISTER(fname);

	vinput = pccd_170170->video_input_record->record_class_struct;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VIDEO_INPUT pointer for video input '%s' "
		"used by area detector '%s' is NULL.",
			pccd_170170->video_input_record->name,
			ad->record->name );
	}

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	sp = &(ad->sequence_parameters);

	camera_is_master =
	   (pccd_170170->pccd_170170_flags & MXF_PCCD_170170_CAMERA_IS_MASTER);

	external_trigger = (vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER);

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: camera_is_master = %d, external_trigger = %d",
		fname, (int) camera_is_master, (int) external_trigger));
#endif

	switch ( sp->sequence_type ) {
	case MXT_SQ_CONTINUOUS:
		/* For continuous sequences, we must use video board
		 * CC1 pulses to trigger the taking of frames, so that
		 * the total number of frames is not limited.
		 */

		camera_is_master = FALSE;
		break;

	case MXT_SQ_STROBE:
		/* For strobe sequences, the external trigger goes into
		 * the video card, which triggers the taking of frames
		 * via CC1 pulses.
		 */

		camera_is_master = FALSE;
		break;
	}

	/* Stop any currently running imaging sequence. */

	mx_status = mx_video_input_abort( pccd_170170->video_input_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for up to 5 seconds for the video input to stop. */

	timeout_in_seconds = 5.0;

	num_ticks_to_wait =
		mx_convert_seconds_to_clock_ticks( timeout_in_seconds );

	finish_tick = mx_add_clock_ticks( mx_current_clock_tick(),
						num_ticks_to_wait );

	while(1) {
		mx_status = mx_area_detector_get_status( ad->record,
							&status_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (status_flags & MXSF_AD_ACQUISITION_IN_PROGRESS) == 0 )
		{
			break;		/* Exit the while() loop. */
		}

		comparison = mx_compare_clock_ticks( mx_current_clock_tick(),
							finish_tick );

		if ( comparison >= 0 ) {
			(void) mx_error( MXE_TIMED_OUT, fname,
			"Area detector '%s' timed out after waiting %g seconds "
			"for video input '%s' to stop acquiring frames.",
				ad->record->name,
				timeout_in_seconds, vinput->record->name );

			break;		/* Exit the while(1) loop. */
		}

		mx_usleep(1000);
	}

	/* Select the master clock. */

	if ( camera_is_master ) {
		master_clock = MXF_VIN_MASTER_CAMERA;
	} else {
		master_clock = MXF_VIN_MASTER_VIDEO_BOARD;
	}

	mx_status = mx_video_input_set_master_clock( vinput->record,
							master_clock );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off the buffer overrun flag. */

	pccd_170170->buffer_overrun = FALSE;

	/* Configure the detector head to use either an internal or
	 * an external trigger.
	 */

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
					MXLV_PCCD_170170_DH_CONTROL,
					&control_register_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( external_trigger ) {
		control_register_value |= MXF_PCCD_170170_EXTERNAL_TRIGGER;
	} else {
		control_register_value &= (~MXF_PCCD_170170_EXTERNAL_TRIGGER);
	}

	mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_CONTROL,
					control_register_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Prepare the video input for the next trigger. */

	if ( camera_is_master && external_trigger ) {

		mx_status = mx_sequence_get_num_frames(
						&(ad->sequence_parameters),
						&num_frames_in_sequence );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( sp->sequence_type ) {
		case MXT_SQ_CONTINUOUS:
		case MXT_SQ_CIRCULAR_MULTIFRAME:
			circular = TRUE;
			break;
		default:
			circular = FALSE;
			break;
		}

		mx_status = mx_video_input_asynchronous_capture(
					pccd_170170->video_input_record,
					num_frames_in_sequence, circular );
	} else {
		mx_status = mx_video_input_arm(
					pccd_170170->video_input_record );
	}

	/* If we are using an EPIX PIXCI imaging board, record the time
	 * that the sequence started in the 'epix_xclib' record.
	 */

	if ( pccd_170170->video_input_record->mx_type == MXT_VIN_EPIX_XCLIB ) {
	    mx_status = mxp_pccd_170170_epix_save_start_timespec(pccd_170170);
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_trigger()";

	MX_PCCD_170170 *pccd_170170;
	MX_VIDEO_INPUT *vinput;
	MX_SEQUENCE_PARAMETERS *sp;
	long num_frames_in_sequence;
	int i, num_triggers;
	mx_bool_type camera_is_master, internal_trigger, circular;
	mx_status_type mx_status;

	pccd_170170 = NULL;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pccd_170170->video_input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The video_input_record pointer for area detector '%s' is NULL.",
			ad->record->name );
	}

	vinput = pccd_170170->video_input_record->record_class_struct;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VIDEO_INPUT pointer for video input '%s' "
		"used by area detector '%s' is NULL.",
			pccd_170170->video_input_record->name,
			ad->record->name );
	}

	DISPLAY_CONTROL_REGISTER(fname);

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	/* Make sure we are configured for a supported sequence type. */

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
	case MXT_SQ_STROBE:
	case MXT_SQ_BULB:
	case MXT_SQ_GEOMETRICAL:
	case MXT_SQ_SUBIMAGE:
	case MXT_SQ_STREAK_CAMERA:
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Image sequence type %ld is not supported by "
			"area detector '%s'.",
			sp->sequence_type, ad->record->name );
	}

	camera_is_master =
	   (pccd_170170->pccd_170170_flags & MXF_PCCD_170170_CAMERA_IS_MASTER);

	internal_trigger = (vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER);

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: camera_is_master = %d, internal_trigger = %d",
		fname, (int) camera_is_master, (int) internal_trigger));
#endif

	switch ( sp->sequence_type ) {
	case MXT_SQ_CONTINUOUS:
		/* For continuous sequences, we must use video board
		 * CC1 pulses to trigger the taking of frames, so that
		 * the total number of frames is not limited.
		 */

		camera_is_master = FALSE;
		break;

	case MXT_SQ_STROBE:
		/* For strobe sequences, the external trigger goes into
		 * the video card, which triggers the taking of frames
		 * via CC1 pulses.
		 */

		camera_is_master = FALSE;
		break;
	}

	if ( camera_is_master && internal_trigger ) {

		mx_status = mx_sequence_get_num_frames(
						&(ad->sequence_parameters),
						&num_frames_in_sequence );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( sp->sequence_type ) {
		case MXT_SQ_CONTINUOUS:
		case MXT_SQ_CIRCULAR_MULTIFRAME:
			circular = TRUE;
			break;
		default:
			circular = FALSE;
			break;
		}

		mx_status = mx_video_input_asynchronous_capture(
					pccd_170170->video_input_record,
					num_frames_in_sequence, circular );
	} else {
		/* Send the trigger request to the video input board. */

		mx_status = mx_video_input_trigger(
					pccd_170170->video_input_record );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we are configured for internal triggering and the camera is
	 * expected to be in charge of frame timing, then we must send
	 * a start pulse to the camera.
	 */

	if ( camera_is_master && internal_trigger ) {

#if 0
		if ( sp->sequence_type == MXT_SQ_SUBIMAGE ) {
			num_triggers = 2;
		} else {
			num_triggers = 1;
		}
#else
		num_triggers = 1;
#endif

		for ( i = 0; i < num_triggers; i++ ) {

			/* Set the output high. */

#if MXD_PCCD_170170_DEBUG
			MX_DEBUG(-2,
				("%s: Sending trigger to camera head.", fname));
#endif

			mx_status = mx_digital_output_write(
				pccd_170170->internal_trigger_record, 1 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_msleep(100);		/* Wait 0.1 seconds. */

			/* Set the output low. */

			mx_status = mx_digital_output_write(
				pccd_170170->internal_trigger_record, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( num_triggers > 1 ) {
				mx_msleep(100);		/* Wait 0.1 seconds. */
			}
		}
	}

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_stop()";

	MX_PCCD_170170 *pccd_170170;
	MX_CLOCK_TICK timeout_in_ticks, current_tick, finish_tick;
	double timeout;
	int comparison;
	mx_bool_type busy, timed_out;
	mx_status_type mx_status;

	pccd_170170 = NULL;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* Send an 'Abort Exposure' command to the detector head by
	 * toggling the CC2 line.
	 */

	mx_status = mx_camera_link_pulse_cc_line(
					pccd_170170->camera_link_record, 2,
					-1, 1000 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the imaging board to stop acquiring frames after the end
	 * of the current frame.  If we are using an EPIX video board via
	 * the XCLIB library, then we force the driver to use the abort
	 * function instead, since the EPIX driver waits until the end
	 * of the current sequence before acknowledging the stop command.
	 */

	if ( pccd_170170->video_input_record->mx_type == MXT_VIN_EPIX_XCLIB ) {

		mx_status = mx_video_input_abort(
				pccd_170170->video_input_record );
	} else {
		mx_status = mx_video_input_stop(
				pccd_170170->video_input_record );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the imaging board to stop.  If this takes more than
	 * 'timeout' seconds, then break out with a timeout error.
	 */

	timeout = 1.0;		/* in seconds */

	timeout_in_ticks = mx_convert_seconds_to_clock_ticks( timeout );

	current_tick = mx_current_clock_tick();

	finish_tick = mx_add_clock_ticks( current_tick, timeout_in_ticks );

	timed_out = FALSE;

	for (;;) {
		mx_status = mx_video_input_is_busy(
				pccd_170170->video_input_record, &busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( busy == FALSE ) {
			break;			/* Exit the for(;;) loop. */
		}

		current_tick = mx_current_clock_tick();

		comparison = mx_compare_clock_ticks(current_tick, finish_tick);

		if ( comparison >= 0 ) {
			timed_out = TRUE;
			break;			/* Exit the for(;;) loop. */
		}
	}

	if ( timed_out ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"Timed out after waiting %g seconds for video input '%s' "
		"to stop acquiring the current frame.",
			timeout, pccd_170170->video_input_record->name );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

MX_EXPORT mx_status_type
mxd_pccd_170170_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_abort()";

	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	pccd_170170 = NULL;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	/* Send a 'Reset CCD Controller' command to the detector head by
	 * toggling the CC3 line.  Not all versions of the firmware 
	 * actually pay attention to CC3.
	 */

	mx_status = mx_camera_link_pulse_cc_line(
					pccd_170170->camera_link_record, 3,
					-1, 1000 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the imaging board to immediately stop acquiring frames. */

	mx_status = mx_video_input_abort( pccd_170170->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_get_extended_status()";

	MX_PCCD_170170 *pccd_170170;
	long last_frame_number;
	long total_num_frames;
	unsigned long status_flags;
	mx_status_type mx_status;

	pccd_170170 = NULL;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	DISPLAY_CONTROL_REGISTER(fname);

	/* Ask the video board for its current status. */

	mx_status = mx_video_input_get_extended_status(
						pccd_170170->video_input_record,
						&last_frame_number,
						&total_num_frames,
						&status_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG_EXTENDED_STATUS
	MX_DEBUG(-2,
("%s: last_frame_number = %ld, total_num_frames = %ld, status_flags = %#lx",
		fname, last_frame_number, total_num_frames, status_flags));
#endif

	ad->last_frame_number = last_frame_number;

	ad->total_num_frames = total_num_frames;

	ad->status = 0;

	if ( status_flags & MXSF_VIN_IS_BUSY ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

	if ( pccd_170170->buffer_overrun ) {
		ad->status |= MXSF_AD_BUFFER_OVERRUN;
	}

	return MX_SUCCESSFUL_RESULT;
}

static void
mxd_pccd_170170_save_raw_frame( uint16_t *image_data,
				unsigned long image_length )
{
	static const char fname[] = "mxd_pccd_170170_save_raw_frame()";

	FILE *file;
	int saved_errno, filename_type;
	unsigned long bytes_written;
	char raw_frame_filename[MXU_FILENAME_LENGTH+1];

	if ( getenv("MXDIR") == NULL ) {
		filename_type = MX_CFN_CWD;
	} else {
		filename_type = MX_CFN_STATE;
	}

	mx_construct_control_system_filename( filename_type,
						"pccd_170170_raw_frame.bin",
						raw_frame_filename,
						sizeof(raw_frame_filename) );

	MX_DEBUG(-2,("%s: Writing raw frame to '%s'",
		fname, raw_frame_filename ));

	file = fopen( raw_frame_filename, "w" );

	if ( file == NULL ) {
		saved_errno = errno;

		mx_error( MXE_NOT_FOUND, fname,
		"Cannot open raw frame file '%s'.  "
		"Errno = %d, error message = '%s'",
			raw_frame_filename,
			saved_errno, strerror(saved_errno) );

		return;
	}

	bytes_written = fwrite( image_data, 1, image_length, file );

	fclose( file );

	if ( bytes_written < image_length ) {
		mx_error( MXE_FILE_IO_ERROR, fname,
		"Raw frame file '%s' was supposed to be %lu bytes long, "
		"but only %lu bytes were written.",
			raw_frame_filename, image_length, bytes_written );
	}

	return;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_readout_frame()";

	MX_PCCD_170170 *pccd_170170;
	unsigned long flags;
	long frame_number, maximum_num_frames, total_num_frames;
	long frame_difference, num_times_looped;
	long number_of_frame_that_overwrote_the_frame_we_want;
	long row_framesize, column_framesize;
	size_t bytes_to_copy, raw_frame_length, image_length;
	struct timespec exposure_timespec;
	double exposure_time;
	mx_status_type mx_status;

	pccd_170170 = NULL;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	/* Compute the frame number modulo the maximum_number of frames.
	 * The modulo part is for the sake of MXT_SQ_CIRCULAR_MULTIFRAME
	 * and MXT_SQ_CONTINUOUS sequences.
	 */

	ad->parameter_type = MXLV_AD_MAXIMUM_FRAME_NUMBER;

	mx_status = mxd_pccd_170170_get_parameter( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	maximum_num_frames = ad->maximum_frame_number + 1;

	frame_number = ad->readout_frame % maximum_num_frames;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: Reading raw frame %ld", fname, frame_number));
	MX_DEBUG(-2,("%s: ad->readout_frame = %ld, maximum_num_frames = %ld",
		fname, ad->readout_frame, maximum_num_frames));
#endif
	/* Has this frame already been overwritten? */

	mx_status = mx_video_input_get_total_num_frames(
						pccd_170170->video_input_record,
						&total_num_frames );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame_difference = total_num_frames - ad->readout_frame;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,
("%s: frame_difference = %ld, total_num_frames = %ld, ad->readout_frame = %ld",
		fname, frame_difference, total_num_frames, ad->readout_frame));
#endif
	/* Only check for frame overwriting if ad->readout_frame is larger
	 * than the total number of frame buffers.  Only in this case can
	 * we be sure that the possibility of buffer overrun exists.
	 */

	if ( ( ad->readout_frame >= maximum_num_frames )
	  && ( frame_difference > 0 ) )
	{
		num_times_looped = frame_difference / maximum_num_frames;

#if MXD_PCCD_170170_DEBUG
		MX_DEBUG(-2,("%s: num_times_looped = %ld",
			fname, num_times_looped));
#endif
		if ( num_times_looped > 0 ) {
			pccd_170170->buffer_overrun = TRUE;

			number_of_frame_that_overwrote_the_frame_we_want
				= ad->readout_frame
					+ num_times_looped * maximum_num_frames;

#if 0
			mx_warning(
		    "%s: Frame %ld has already been overwritten by frame %ld.",
			fname, ad->readout_frame, 
			number_of_frame_that_overwrote_the_frame_we_want );
#endif
		}
	}

#if MXD_PCCD_170170_DEBUG
	{
		long video_x_width, video_y_width;

		mx_status = mx_video_input_get_framesize(
				pccd_170170->video_input_record,
				&video_x_width, &video_y_width );

		MX_DEBUG(-2,("%s: video_x_width = %ld, video_y_width = %ld",
			fname, video_x_width, video_y_width));

		MX_DEBUG(-2,
		("%s: ad->framesize[0] = %ld, ad->framesize[1] = %ld",
			fname, ad->framesize[0], ad->framesize[1]));

		MX_DEBUG(-2,
		("%s: raw_frame x width = %ld, raw_frame y width = %ld",
			fname, (long)MXIF_ROW_FRAMESIZE(pccd_170170->raw_frame),
			(long)MXIF_COLUMN_FRAMESIZE(pccd_170170->raw_frame) ));
	}
#endif

	/* Read in the raw image frame. */

	mx_status = mx_video_input_get_frame(
		pccd_170170->video_input_record,
		frame_number, &(pccd_170170->raw_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the binsize in the header. */

	MXIF_ROW_BINSIZE(pccd_170170->raw_frame)    = ad->binsize[0];
	MXIF_COLUMN_BINSIZE(pccd_170170->raw_frame) = ad->binsize[1];

	/* Compute and add the exposure time to the image frame header. */

	mx_status = mx_sequence_get_exposure_time( &(ad->sequence_parameters),
					ad->readout_frame, &exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	exposure_timespec =
		mx_convert_seconds_to_high_resolution_time( exposure_time );

	MXIF_EXPOSURE_TIME_SEC(pccd_170170->raw_frame)
						= exposure_timespec.tv_sec;

	MXIF_EXPOSURE_TIME_NSEC(pccd_170170->raw_frame)
						= exposure_timespec.tv_nsec;

	/* Make sure that the image frame is the correct size. */

	row_framesize = MXIF_ROW_FRAMESIZE(pccd_170170->raw_frame)
				/ pccd_170170->horiz_descramble_factor;

	column_framesize = MXIF_COLUMN_FRAMESIZE(pccd_170170->raw_frame)
				* pccd_170170->vert_descramble_factor;

#if MXD_PCCD_170170_DEBUG_MX_IMAGE_ALLOC
	MX_DEBUG(-2,
	("%s: Invoking mx_image_alloc() for ad->image_frame = %p",
		fname, ad->image_frame));
#endif

	mx_status = mx_image_alloc( &(ad->image_frame),
				row_framesize,
				column_framesize,
				MXIF_IMAGE_FORMAT(pccd_170170->raw_frame),
				MXIF_BYTE_ORDER(pccd_170170->raw_frame),
				MXIF_BYTES_PER_PIXEL(pccd_170170->raw_frame),
				pccd_170170->raw_frame->header_length,
				pccd_170170->raw_frame->image_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Copy the image header. */

	mx_status = mx_image_copy_header( pccd_170170->raw_frame,
					ad->image_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Copying the header overwrites all of the values,
	 * so we patch in the correct descrambled dimensions.
	 */

	MXIF_ROW_FRAMESIZE(ad->image_frame) = row_framesize;

	MXIF_COLUMN_FRAMESIZE(ad->image_frame) = column_framesize;

	flags = pccd_170170->pccd_170170_flags;

	/* If requested, save a copy of the undescrambled data. */

	if ( flags & MXF_PCCD_170170_SAVE_RAW_FRAME ) {
		mxd_pccd_170170_save_raw_frame(
			pccd_170170->raw_frame->image_data,
			pccd_170170->raw_frame->image_length );
	}

	/* If required, we now descramble the image. */

	if ( flags & MXF_PCCD_170170_SUPPRESS_DESCRAMBLING ) {

		/* Just copy directly from the raw frame to the image frame. */

		raw_frame_length = pccd_170170->raw_frame->image_length;

		image_length = ad->image_frame->image_length;

		if ( raw_frame_length < image_length ) {
			bytes_to_copy = raw_frame_length;
		} else {
			bytes_to_copy = image_length;
		}

#if MXD_PCCD_170170_DEBUG
		MX_DEBUG(-2,
	    ("%s: Copying camera simulator image.  Image length = %lu bytes.",
			fname, (unsigned long) bytes_to_copy ));
#endif
		memcpy( ad->image_frame->image_data,
			pccd_170170->raw_frame->image_data,
			bytes_to_copy );
	} else {
		/* Descramble the image. */

		switch( ad->record->mx_type ) {
		case MXT_AD_PCCD_170170:
		case MXT_AD_PCCD_4824:
			mx_status = mxd_pccd_170170_descramble_image( ad,
							pccd_170170,
							ad->image_frame,
							pccd_170170->raw_frame);
			break;
		case MXT_AD_PCCD_16080:
			mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Image descrambling for area detector '%s' is not "
			"yet implemented for AVIEX cameras of type '%s'.",
				ad->record->name,
				mx_get_driver_name( ad->record ) );
			break;
		default:
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The AVIEX PCCD driver requested for record '%s' "
			"cannot be used by records of type '%s'.",
				ad->record->name,
				mx_get_driver_name( ad->record ) );
			break;
		}
	}

	/* If required, apply the bias lookup table. */

	if ( pccd_170170->bias_lookup_table != NULL ) {
		mx_status = mxd_pccd_170170_apply_bias_lookup_table(
						pccd_170170, ad->image_frame );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Dezingering test code. */

	if ( flags & MXF_PCCD_170170_TEST_DEZINGER ) {

		/* If we are testing the dezingering logic, we potentially
		 * introduce fake zingers here.
		 */

		uint16_t *image_data;
		int i, num_random_values, random_location, random_value;

		image_data = ad->image_frame->image_data;

		num_random_values = rand() % 4;

		for ( i = 0; i < num_random_values; i++ ) {
			random_location = rand()
			   % (ad->image_frame->image_length / sizeof(uint16_t));

			random_value = rand() % 65536;

			image_data[random_location] = random_value;

			MX_DEBUG(-2,
			("%s: Replaced pixel %d with the zinger value %d.",
				fname, random_location, random_value));
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_correct_frame()";

	MX_PCCD_170170 *pccd_170170;
	MX_SEQUENCE_PARAMETERS *sp;
	MX_IMAGE_FRAME *mask_frame;
	MX_IMAGE_FRAME *bias_frame;
	MX_IMAGE_FRAME *flood_field_frame;
	unsigned long flags;
	uint16_t *mask_data_ptr, *bias_data_ptr, *flood_field_data_ptr;
	uint16_t *image_data_array;
	uint16_t *image_stripe_ptr, *image_row_ptr, *image_pixel_ptr;
	uint16_t *mask_pixel_ptr, *bias_pixel_ptr, *flood_field_pixel_ptr;
	double flood_field_scale_factor;
	long big_image_pixel;
	long image_row_framesize, image_column_framesize;
	long image_num_stripes, image_pixels_per_stripe;
	long corr_row_framesize, corr_column_framesize;
	long corr_centerline, corr_num_rows_per_stripe;
	long corr_first_row, corr_start_offset;
	long i, j, k;
	mx_status_type mx_status;

	pccd_170170 = NULL;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sp = &(ad->sequence_parameters);

	flags = ad->correction_flags;

#if MXD_PCCD_170170_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
	MX_DEBUG(-2,("%s: ad->correction_flags = %#lx", fname, flags));
#endif

	if ( flags == 0 ) {
		/* No corrections have been requested. */

		return MX_SUCCESSFUL_RESULT;
	}

	if ( ad->image_frame == NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"No image frames have been taken or loaded by detector '%s' "
		"since this program was started.", ad->record->name );
	}

	image_data_array = ad->image_frame->image_data;

	image_row_framesize    = MXIF_ROW_FRAMESIZE(ad->image_frame);
	image_column_framesize = MXIF_COLUMN_FRAMESIZE(ad->image_frame);

#if MXD_PCCD_170170_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,
	("%s: image_row_framesize = %ld, image_column_framesize = %ld",
		fname, image_row_framesize, image_column_framesize));
#endif

	/*-----*/

	if ( (sp->sequence_type != MXT_SQ_SUBIMAGE)
	  && (sp->sequence_type != MXT_SQ_STREAK_CAMERA) )
	{
		/**********************
		 * NORMAL FULL FRAMES *
		 **********************/

		/* For normal full frame images, first apply the
		 * default correction function.
		 */

		mx_status = mx_area_detector_default_correct_frame( ad );

		return mx_status;
	}

	/*-----*/

	/************************************
	 * STREAK CAMERA OR SUBIMAGE FRAMES *
	 ************************************/

	/*
	 * Find the pixel offset of the first row in the correction frames
	 * that will be used to correct the data.
	 */

	/* We get the dimensions of the correction frames from the mask frame.*/

	mx_status = mx_area_detector_get_correction_frame( ad, ad->image_frame,
							MXFT_AD_MASK_FRAME,
							"mask",
							&mask_frame );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	corr_row_framesize    = MXIF_ROW_FRAMESIZE(mask_frame);
	corr_column_framesize = MXIF_COLUMN_FRAMESIZE(mask_frame);

	if ( image_column_framesize != corr_column_framesize ) {
		return mx_error( MXE_CONFIGURATION_CONFLICT, fname,
		"The number of columns (%ld) in the image frame is not "
		"same as the number of columns (%ld) in the mask frame "
		"for area detector '%s'.",
			image_column_framesize, corr_column_framesize,
			ad->record->name );
	}

#if MXD_PCCD_170170_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,
	("%s: corr_row_framesize = %ld, corr_column_framesize = %ld",
		fname, corr_row_framesize, corr_column_framesize));
#endif

	/* Find the centerline of the correction frame data. */

	if ( pccd_170170->use_top_half_of_detector ) {
		corr_centerline = mx_round(0.25 * (double) corr_row_framesize);
	} else {
		corr_centerline = mx_round(0.75 * (double) corr_row_framesize);
	}

	if ( sp->sequence_type == MXT_SQ_STREAK_CAMERA ) {
		corr_num_rows_per_stripe = 2L;
	} else {
		corr_num_rows_per_stripe = mx_round( sp->parameter_array[0] );
	}

	image_num_stripes = image_column_framesize / corr_num_rows_per_stripe;

	image_pixels_per_stripe =
			image_row_framesize * corr_num_rows_per_stripe;

#if MXD_PCCD_170170_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,
	("%s: corr_centerline = %ld, corr_num_rows_per_stripe = %ld",
		fname, corr_centerline, corr_num_rows_per_stripe));
	MX_DEBUG(-2,
	("%s: image_num_stripes = %ld, image_pixels_per_stripe = %ld",
		fname, image_num_stripes, image_pixels_per_stripe));
#endif
	/* Now find the first row pixel offset. */

	corr_first_row = corr_centerline - ( corr_num_rows_per_stripe / 2L );

	corr_start_offset = corr_first_row * corr_column_framesize;

#if MXD_PCCD_170170_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s: corr_first_row = %ld, corr_start_offset = %ld",
		fname, corr_first_row, corr_start_offset));
#endif

	/*---*/

	/* Get pointers to the parts of the correction frames that we
	 * will use for the subimage or streak camera frame.
	 */

	if ( (flags & MXFT_AD_MASK_FRAME) == 0 ) {
		mask_data_ptr = NULL;
	} else {
#if 0
		/* We already found the mask frame earlier in the routine. */

		mx_status = mx_area_detector_get_correction_frame(
							ad, ad->image_frame,
							MXFT_AD_MASK_FRAME,
							"mask",
							&mask_frame );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
#endif

		mask_data_ptr = mask_frame->image_data;
		mask_data_ptr += corr_start_offset;
	}

	if ( (flags & MXFT_AD_BIAS_FRAME) == 0 ) {
		bias_data_ptr = NULL;
	} else {
		mx_status = mx_area_detector_get_correction_frame(
							ad, ad->image_frame,
							MXFT_AD_BIAS_FRAME,
							"bias",
							&bias_frame );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		bias_data_ptr = bias_frame->image_data;
		bias_data_ptr += corr_start_offset;
	}

	/* We do not do dark current correction for subimage
	 * or streak camera frames.
	 */

	if ( (flags & MXFT_AD_FLOOD_FIELD_FRAME) == 0 ) {
		flood_field_data_ptr = NULL;
	} else {
		mx_status = mx_area_detector_get_correction_frame(
						ad, ad->image_frame,
						MXFT_AD_FLOOD_FIELD_FRAME,
						"flood_field",
						&flood_field_frame );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		flood_field_data_ptr = flood_field_frame->image_data;
		flood_field_data_ptr += corr_start_offset;
	}

	/* Now walk through the stripes in the image data
	 * and do the corrections.
	 */

	for ( i = 0; i < image_num_stripes; i++ ) {
	    image_stripe_ptr = image_data_array + i * image_pixels_per_stripe;

	    for ( j = 0; j < corr_num_rows_per_stripe; j++ ) {
		image_row_ptr = image_stripe_ptr + j * image_column_framesize;

		for ( k = 0; k < image_column_framesize; k++ ) {
		    image_pixel_ptr = image_row_ptr + k;

		    /* See if the pixel is masked off. */

		    if ( mask_data_ptr != NULL ) {
			mask_pixel_ptr = mask_data_ptr
						+ j * corr_column_framesize + k;

			if ( *mask_pixel_ptr == 0 ) {

			    /* If the pixel is masked off, skip this pixel
			     * and return to the top of the k loop for
			     * the next pixel.
			     */

			    *image_pixel_ptr = 0;
			    continue;
			}					
		    }

		    /* Add the detector bias. */

		    if ( bias_data_ptr != NULL ) {
			bias_pixel_ptr = bias_data_ptr
						+ j * corr_column_framesize + k;

			*image_pixel_ptr += *bias_pixel_ptr;
		    }

		    /* Do the flood field correction. */

		    if ( flood_field_data_ptr != NULL ) {
			flood_field_pixel_ptr = flood_field_data_ptr
						+ j * corr_column_framesize + k;

			flood_field_scale_factor =
			    mx_divide_safely( ad->flood_field_average_intensity,
					(double) *flood_field_pixel_ptr );

			big_image_pixel = mx_round(
			  flood_field_scale_factor * (double) *image_pixel_ptr);

			if ( big_image_pixel > 65535 ) {
			    *image_pixel_ptr = 65535;
			} else {
			    *image_pixel_ptr = big_image_pixel;
			}
		    }
		}
	    }
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_find_register( MX_AREA_DETECTOR *ad,
					MX_PCCD_170170 *pccd_170170,
					long *parameter_type,
					void **value_ptr,
					MX_RECORD_FIELD **field )
{
	static const char fname[] = "mxd_pccd_170170_find_register()";

	MX_RECORD_FIELD *register_value_field;
	mx_status_type mx_status;

	if ( *parameter_type < 0 ) {
		/* In this case, we got here by way of the 'register_value'
		 * field.  We must find the label value of the actual field
		 * so that we can tell whether or not it is a pseudo register
		 * that depends on the control register.
		 */

		mx_status = mx_find_record_field( ad->record,
					ad->register_name, field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*parameter_type = (*field)->label_value;

		if ( *parameter_type < MXLV_PCCD_170170_DH_BASE ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"'%s' is not the name of a register for area detector '%s'.",
				ad->register_name, ad->record->name );
		}

		/* In this case, we read from and write to the 'register_value'
		 * field, instead of the field we just found above.
		 */
		
		mx_status = mx_find_record_field( ad->record,
				"register_value", &register_value_field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*value_ptr = mx_get_field_value_pointer( register_value_field );
	} else
	if ( *parameter_type >= MXLV_PCCD_170170_DH_BASE ) {
		/* If we were passed the field label value directly, then the
		 * 'register_value' field was not involved in getting here.
		 *
		 * In this case, we read from and write to the corresponding
		 * field directly.
		 */

		mx_status = mx_get_field_by_label_value( ad->record,
						*parameter_type, field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*value_ptr = mx_get_field_value_pointer( *field );
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "Unrecognized parameter type %ld for PCCD-170170 detector '%s'.",
					*parameter_type, ad->record->name );
	}

	if ( (*field)->datatype != MXFT_ULONG ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	    "Record field '%s' for record '%s' is not an unsigned long field.",
			(*field)->name, ad->record->name );
	}

	return mx_status;
}

static mx_status_type
mxd_pccd_170170_get_register_value( MX_AREA_DETECTOR *ad,
					MX_PCCD_170170 *pccd_170170,
					long parameter_type )
{
	static const char fname[] = "mxd_pccd_170170_get_register_value()";

	MX_RECORD_FIELD *field;
	unsigned long control_register, pseudo_reg_value;
	void *value_ptr;
	unsigned long *ulong_value_ptr;
	mx_status_type mx_status;

	/* Note: mxd_pccd_170170_find_register() will modify the value of
	 * 'parameter_type' if the original value of 'parameter_type' is
	 * less than zero.
	 */

	mx_status = mxd_pccd_170170_find_register( ad, pccd_170170,
							&parameter_type,
							&value_ptr,
							&field );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ulong_value_ptr = value_ptr;

	/* For real registers, getting the value is simple. */

	if ( parameter_type < MXLV_PCCD_170170_DH_PSEUDO_BASE ) {
		mx_status = mxd_pccd_170170_read_register( pccd_170170,
						parameter_type, value_ptr );
		return mx_status;
	}

	/* For pseudo registers, we must extract the value from
	 * the control register.
	 */

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
					MXLV_PCCD_170170_DH_CONTROL,
					&control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( parameter_type ) {
	case MXLV_PCCD_170170_DH_DETECTOR_READOUT_MODE:
		pseudo_reg_value = (control_register >> 5) & 0x3;
		break;
	case MXLV_PCCD_170170_DH_READOUT_SPEED:
		pseudo_reg_value = (control_register >> 1) & 0x1;
		break;
	case MXLV_PCCD_170170_DH_TEST_MODE:
		pseudo_reg_value = control_register & 0x1;
		break;
	case MXLV_PCCD_170170_DH_OFFSET_CORRECTION:
		pseudo_reg_value = (control_register >> 2) & 0x1;
		break;
	case MXLV_PCCD_170170_DH_EXPOSURE_MODE:
		pseudo_reg_value = (control_register >> 3) & 0x3;
		break;
	case MXLV_PCCD_170170_DH_LINEARIZATION:
		pseudo_reg_value = (control_register >> 7) & 0x1;
		break;
	case MXLV_PCCD_170170_DH_DUMMY_FRAME_VALID:
		pseudo_reg_value = (control_register >> 9) & 0x1;
		break;
	case MXLV_PCCD_170170_DH_SHUTTER_DISABLE:
		pseudo_reg_value = (control_register >> 10) & 0x1;
		break;
	case MXLV_PCCD_170170_DH_OVER_EXPOSURE_WARNING:
		pseudo_reg_value = (control_register >> 11) & 0x1;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal pseudoregister %lu requested for area detector '%s'.",
			parameter_type, ad->record->name );
		break;
	}

	*ulong_value_ptr = pseudo_reg_value;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_set_register_value( MX_AREA_DETECTOR *ad,
					MX_PCCD_170170 *pccd_170170,
					long parameter_type )
{
	static const char fname[] = "mxd_pccd_170170_set_register_value()";

	MX_RECORD_FIELD *field;
	unsigned long control_register, register_value, pseudo_reg_value;
	void *value_ptr;
	unsigned long *ulong_value_ptr;
	mx_status_type mx_status;

	/* Note: mxd_pccd_170170_find_register() will modify the value of
	 * 'parameter_type' if the original value of 'parameter_type' is
	 * less than zero.
	 */

	mx_status = mxd_pccd_170170_find_register( ad, pccd_170170,
							&parameter_type,
							&value_ptr,
							&field );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ulong_value_ptr = value_ptr;

	/* We refuse to write to registers that are marked as read only. */

	if ( field->flags & MXFF_READ_ONLY ) {
		return mx_error( MXE_READ_ONLY, fname,
	    "PCCD-170170 register '%s' for area detector '%s' is read only.",
		field->name, ad->record->name );
	}

	/* For real registers, setting the value is simple. */

	if ( parameter_type < MXLV_PCCD_170170_DH_PSEUDO_BASE ) {
		mx_status = mxd_pccd_170170_write_register( pccd_170170,
					parameter_type, *ulong_value_ptr );
		return mx_status;
	}

	/* For pseudo registers, we must change specific bits in
	 * the control register.
	 */

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
					MXLV_PCCD_170170_DH_CONTROL,
					&control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	register_value = *ulong_value_ptr;

	switch( parameter_type ) {
	case MXLV_PCCD_170170_DH_DETECTOR_READOUT_MODE:
		pseudo_reg_value = ( register_value & 0x3 ) << 5;

		control_register &= ~0x60;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_PCCD_170170_DH_READOUT_SPEED:
		pseudo_reg_value = ( register_value & 0x1 ) << 0x1;

		control_register &= ~0x2;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_PCCD_170170_DH_TEST_MODE:
		pseudo_reg_value = register_value & 0x1;

		control_register &= ~0x1;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_PCCD_170170_DH_OFFSET_CORRECTION:
		pseudo_reg_value = ( register_value & 0x1 ) << 2;

		control_register &= ~0x4;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_PCCD_170170_DH_EXPOSURE_MODE:
		pseudo_reg_value = ( register_value & 0x3 ) << 3;

		control_register &= ~0x18;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_PCCD_170170_DH_LINEARIZATION:
		pseudo_reg_value = ( register_value & 0x1 ) << 7;

		control_register &= ~0x80;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_PCCD_170170_DH_DUMMY_FRAME_VALID:
		pseudo_reg_value = ( register_value & 0x1 ) << 9;

		control_register &= ~0x200;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_PCCD_170170_DH_SHUTTER_DISABLE:
		pseudo_reg_value = ( register_value & 0x1 ) << 10;

		control_register &= ~0x400;

		control_register |= pseudo_reg_value;
		break;
	case MXLV_PCCD_170170_DH_OVER_EXPOSURE_WARNING:
		pseudo_reg_value = ( register_value & 0x1 ) << 11;

		control_register &= ~0x800;

		control_register |= pseudo_reg_value;
		break;
	}

	mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_CONTROL,
					control_register );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_get_parameter()";

	MX_PCCD_170170 *pccd_170170;
	MX_RECORD *video_input_record;
	long vinput_horiz_framesize, vinput_vert_framesize;
	long shutter_disabled;
	mx_status_type mx_status;

	pccd_170170 = NULL;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	{
		char name_buffer[MXU_FIELD_NAME_LENGTH+1];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type(
				ad->record, ad->parameter_type,
				name_buffer, sizeof(name_buffer)) ));
	}
#endif
	video_input_record = pccd_170170->video_input_record;

	switch( ad->parameter_type ) {
	case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		mx_status = mx_video_input_get_maximum_frame_number(
			video_input_record, &(ad->maximum_frame_number) );
		break;

	case MXLV_AD_FRAMESIZE:
		mx_status = mx_video_input_get_framesize( video_input_record,
			     &vinput_horiz_framesize, &vinput_vert_framesize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* See the comments about the Aviex camera in the function
		 * mxd_pccd_170170_open() for the explanation of where the
		 * horizontal and vertical descramble factors come from.
		 */

		ad->framesize[0] = 
		  vinput_horiz_framesize / pccd_170170->horiz_descramble_factor;

		ad->framesize[1] =
		  vinput_vert_framesize * pccd_170170->vert_descramble_factor;

		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		mx_status = mx_video_input_get_image_format( video_input_record,
						&(ad->image_format) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		mx_status = mx_video_input_get_bytes_per_frame(
				video_input_record, &(ad->bytes_per_frame) );
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
		mx_status = mx_video_input_get_bytes_per_pixel(
				video_input_record, &(ad->bytes_per_pixel) );
		break;

	case MXLV_AD_SEQUENCE_START_DELAY:
	case MXLV_AD_TOTAL_ACQUISITION_TIME:
	case MXLV_AD_DETECTOR_READOUT_TIME:
	case MXLV_AD_TOTAL_SEQUENCE_TIME:
		switch( ad->record->mx_type ) {
		case MXT_AD_PCCD_170170:
			mx_status = mxd_pccd_170170_compute_sequence_times(
							ad, pccd_170170 );
			break;
		case MXT_AD_PCCD_4824:
			ad->sequence_start_delay = 0.0;
			ad->total_acquisition_time = 0.0;
			ad->detector_readout_time = 0.0;
			ad->total_sequence_time = 0.0;
			break;
		case MXT_AD_PCCD_16080:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Computed sequence times are not yet available "
			"for record '%s' which uses driver '%s'.",
				ad->record->name,
				mx_get_driver_name( ad->record ) );
			break;
		default:
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The AVIEX PCCD driver requested for record '%s' "
			"cannot be used by records of type '%s'.",
				ad->record->name,
				mx_get_driver_name( ad->record ) );
			break;
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXD_PCCD_170170_DEBUG
		MX_DEBUG(-2,("%s: ad->total_sequence_time = %g",
			fname, ad->total_sequence_time));
#endif
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		break;

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_video_input_get_trigger_mode(
				video_input_record, &(ad->trigger_mode) );
		break;

	case MXLV_AD_REGISTER_VALUE:
		mx_status = mxd_pccd_170170_get_register_value( ad,
							pccd_170170, -1 );
		break;

	case MXLV_AD_SHUTTER_ENABLE:
		mx_status = mx_area_detector_get_register( ad->record,
							"dh_shutter_disable",
							&shutter_disabled );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( shutter_disabled ) {
			ad->shutter_enable = FALSE;
		} else {
			ad->shutter_enable = TRUE;
		}
		break;

	default:
		if ( ad->parameter_type >= MXLV_PCCD_170170_DH_BASE ) {
			mx_status = mxd_pccd_170170_get_register_value( ad,
					    pccd_170170, ad->parameter_type );
		} else {
			mx_status =
			    mx_area_detector_default_get_parameter_handler(ad);
		}
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_set_parameter()";

	MX_PCCD_170170 *pccd_170170;
	MX_SEQUENCE_PARAMETERS *sp;
	unsigned long old_control_register_value, new_control_register_value;
	unsigned long old_detector_readout_mode;
	unsigned long flags;
	long vinput_horiz_framesize, vinput_vert_framesize;
	long horiz_binsize, vert_binsize;
	long saved_vert_binsize, saved_vert_framesize;
	long num_streak_mode_lines;
	long num_frames, exposure_steps, gap_steps;
	long exposure_multiplier_steps, gap_multiplier_steps;
	long total_num_subimage_lines;
	long shutter_disable;
	unsigned long new_delay_time;
	double exposure_time, frame_time, gap_time, subimage_time, line_time;
	double exposure_multiplier, gap_multiplier;
	double bytes_per_frame;
	mx_status_type mx_status;

	static long allowed_binsize[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

	static int num_allowed_binsizes = sizeof( allowed_binsize )
						/ sizeof( allowed_binsize[0] );

	pccd_170170 = NULL;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	{
		char name_buffer[MXU_FIELD_NAME_LENGTH+1];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type(
				ad->record, ad->parameter_type,
				name_buffer, sizeof(name_buffer)) ));
	}
#endif
	flags = pccd_170170->pccd_170170_flags;

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:

		sp = &(ad->sequence_parameters);

		/* Invalidate any existing image sector array. */

		mxd_pccd_170170_free_sector_array(
				pccd_170170->sector_array );

		pccd_170170->sector_array = NULL;

		/* Find a compatible framesize and binsize. */

		if ( sp->sequence_type == MXT_SQ_STREAK_CAMERA ) {
			/* Handle streak camera mode. */

			saved_vert_binsize   = ad->binsize[1];
			saved_vert_framesize = ad->framesize[1];

			/* Restrict the allowed binsize for Y
			 * to either 1 or 2.
			 */

			if ( saved_vert_binsize <= 1 ) {
				saved_vert_binsize = 1;
			} else
			if ( saved_vert_binsize >= 2 ) {
				saved_vert_binsize = 2;
			}

#if MXD_PCCD_170170_DEBUG
			MX_DEBUG(-2,
		("%s: saved_vert_binsize = %ld, saved_vert_framesize = %ld",
			    fname, saved_vert_binsize, saved_vert_framesize));
#endif

			/* Allow the utility function to change the
			 * X binning and framesize.
			 */

			mx_status = mx_area_detector_compute_new_binning( ad,
							ad->parameter_type,
							num_allowed_binsizes,
							allowed_binsize );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Restore the saved Y binsize and framesize. */

			ad->binsize[1] = saved_vert_binsize;
			ad->framesize[1] = saved_vert_framesize;
		} else {
			/* Handle non-streak camera modes. */

			mx_status = mx_area_detector_compute_new_binning( ad,
							ad->parameter_type,
							num_allowed_binsizes,
							allowed_binsize );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		/* Tell the detector head to change its binsize. */

		mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_PIXEL_BINNING,
					ad->binsize[0] );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_LINE_BINNING,
					ad->binsize[1] );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Tell the video input to change its framesize. */

		/* See the comments about the Aviex camera in the function
		 * mxd_pccd_170170_open() for the explanation of where the
		 * horizontal and vertical descramble factors come from.
		 */

		vinput_horiz_framesize =
			ad->framesize[0] * pccd_170170->horiz_descramble_factor;

		vinput_vert_framesize =
			ad->framesize[1] / pccd_170170->vert_descramble_factor;

		horiz_binsize = ad->binsize[0];
		vert_binsize  = ad->binsize[1];

		mx_status = mx_video_input_set_framesize(
					pccd_170170->video_input_record,
					vinput_horiz_framesize,
					vinput_vert_framesize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If the resolution change succeeded, save the framesize for
		 * later use in switching to and from streak camera mode.
		 */

		pccd_170170->vinput_normal_framesize[0] =
						vinput_horiz_framesize;

		pccd_170170->vinput_normal_framesize[1] =
						vinput_vert_framesize;

		pccd_170170->normal_binsize[0] = horiz_binsize;

		pccd_170170->normal_binsize[1] = vert_binsize;

		/* Update the number of bytes per frame for this resolution. */

		bytes_per_frame =
		    ad->bytes_per_pixel * ad->framesize[0] * ad->framesize[1];

		ad->bytes_per_frame = mx_round( bytes_per_frame );
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:

		/* Do not send any changes to the hardware when these two
		 * values are changed since the contents of the sequence
		 * parameter array may not match the change.  For example,
		 * if we are changing from one-shot mode to multiframe mode,
		 * parameter_array[0] is the exposure time in one-shot mode,
		 * but parameter_array[0] is the number of frames in
		 * multiframe mode.  Thus, the only quasi-safe time for
		 * sending changes to the hardware is when the contents
		 * of the sequence parameter array has been changed.
		 */
		break;

	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 

		/* Invalidate any existing image sector array. */

		mxd_pccd_170170_free_sector_array(
				pccd_170170->sector_array );

		pccd_170170->sector_array = NULL;

		/* Update the maximum frame number. */

		mx_status = mx_video_input_get_maximum_frame_number(
					pccd_170170->video_input_record,
					&(ad->maximum_frame_number) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Get the number of frames requested by the sequence. */

		sp = &(ad->sequence_parameters);

		mx_status = mx_sequence_get_num_frames( sp, &num_frames );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( sp->sequence_type ) {
		case MXT_SQ_CONTINUOUS:
			/* Continuous scans are not limited by the maximum
			 * number of frames that the video card can handle,
			 * since they wrap back to the first frame when they
			 * reach the last frame.
			 */
			break;
		case MXT_SQ_CIRCULAR_MULTIFRAME:
			if ( num_frames >
				MXF_PCCD_170170_MAXIMUM_DETECTOR_HEAD_FRAMES )
			{
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"The circular multiframe sequence requested "
				"for area detector '%s' would have more "
				"frames (%ld) than the maximum number of "
				"frames available (%d).",
				ad->record->name, num_frames,
				MXF_PCCD_170170_MAXIMUM_DETECTOR_HEAD_FRAMES );
			}
			break;
		default:
			if ( num_frames > (ad->maximum_frame_number + 1) ) {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"The sequence requested for area detector '%s' "
				"would have more frames (%ld) than the maximum "
				"number of frames available (%ld) from video "
				"input '%s'.",
					ad->record->name, num_frames,
					ad->maximum_frame_number + 1,
					pccd_170170->video_input_record->name );
			}
			break;
		}

		/* Get the old detector readout mode, since that will be
		 * used by the following code.
		 */

		mx_status = mxd_pccd_170170_read_register( pccd_170170,
					MXLV_PCCD_170170_DH_CONTROL,
					&old_control_register_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		old_detector_readout_mode = old_control_register_value
				& MXF_PCCD_170170_DETECTOR_READOUT_MASK;

#if MXD_PCCD_170170_DEBUG
		MX_DEBUG(-2,("%s: old_control_register_value = %#lx",
			fname, old_control_register_value));
		MX_DEBUG(-2,("%s: old_detector_readout_mode = %#lx",
			fname, old_detector_readout_mode));
#endif
		/* Reprogram the detector head for the requested sequence. */

		new_control_register_value = old_control_register_value;

		/* Mask off the duration bit, since it is only used
		 * by MXT_SQ_BULB sequences.
		 */

		new_control_register_value
			&= (~MXF_PCCD_170170_EXTERNAL_DURATION_TRIGGER);

		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_CONTINUOUS:
		case MXT_SQ_MULTIFRAME:
		case MXT_SQ_CIRCULAR_MULTIFRAME:
		case MXT_SQ_STROBE:
		case MXT_SQ_BULB:
		case MXT_SQ_GEOMETRICAL:

			/* If this is bulb mode, turn on the duration bit. */

			if ( sp->sequence_type == MXT_SQ_BULB ) {
				new_control_register_value
				  |= MXF_PCCD_170170_EXTERNAL_DURATION_TRIGGER;
			}

			/* Get the detector readout time. */

			mx_status = mx_area_detector_get_detector_readout_time(
							ad->record, NULL );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( sp->sequence_type == MXT_SQ_GEOMETRICAL ) {
				exposure_multiplier = sp->parameter_array[3];

				exposure_multiplier_steps = mx_round_down(
					(exposure_multiplier - 1.0) * 256.0);

				gap_multiplier = sp->parameter_array[4];

				gap_multiplier_steps = mx_round_down(
					(gap_multiplier - 1.0) * 256.0);
			} else {
				exposure_multiplier_steps = 0L;
				gap_multiplier_steps = 0L;
			}

#if MXD_PCCD_170170_DEBUG_SEQUENCE_TIMES
			MX_DEBUG(-2,
			("%s: exposure_multiplier = %g, gap_multiplier = %g",
				fname, exposure_multiplier, gap_multiplier));
			MX_DEBUG(-2,
	    ("%s: exposure_multiplier_steps = %ld, gap_multiplier_steps = %ld",
				fname, exposure_multiplier_steps,
				gap_multiplier_steps));
#endif

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER,
					exposure_multiplier_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_GAP_MULTIPLIER,
					gap_multiplier_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( old_detector_readout_mode != 0 ) {
				/* We must switch to full frame mode. */

				new_control_register_value
				    &= (~MXF_PCCD_170170_DETECTOR_READOUT_MASK);

				/* If we were previously in streak camera mode,
				 * we must restore the normal imaging board
				 * framesize.
				 */

				if ( old_detector_readout_mode
					== MXF_PCCD_170170_STREAK_CAMERA_MODE )
				{
				    mx_status = mx_video_input_set_framesize(
					pccd_170170->video_input_record,
					pccd_170170->vinput_normal_framesize[0],
				       pccd_170170->vinput_normal_framesize[1]);

				    if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				    mx_status = mx_area_detector_set_binsize(
					pccd_170170->record,
					pccd_170170->normal_binsize[0],
					pccd_170170->normal_binsize[1]);

				    if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
				}
			}

			/* Configure the detector head for the requested 
			 * number of frames, exposure time, and gap time.
			 */

			if ( sp->sequence_type == MXT_SQ_ONE_SHOT ) {
				num_frames = 1;
				exposure_time = sp->parameter_array[0];
				frame_time = exposure_time;
			} else
			if ( sp->sequence_type == MXT_SQ_CONTINUOUS ) {
				num_frames = 1;
				exposure_time = sp->parameter_array[0];
				frame_time = exposure_time;
			} else
			if ( sp->sequence_type == MXT_SQ_MULTIFRAME ) {
				num_frames = mx_round( sp->parameter_array[0] );
				exposure_time = sp->parameter_array[1];
				frame_time = sp->parameter_array[2];
			} else
			if ( sp->sequence_type == MXT_SQ_CIRCULAR_MULTIFRAME ) {
				num_frames = mx_round( sp->parameter_array[0] );
				exposure_time = sp->parameter_array[1];
				frame_time = sp->parameter_array[2];
			} else
			if ( sp->sequence_type == MXT_SQ_STROBE ) {
				num_frames = mx_round( sp->parameter_array[0] );
				exposure_time = sp->parameter_array[1];

				frame_time = exposure_time
						+ ad->detector_readout_time;
			} else
			if ( sp->sequence_type == MXT_SQ_BULB ) {
				num_frames = mx_round( sp->parameter_array[0] );

				if ( num_frames > 1 ) {
					return mx_error(
					MXE_WOULD_EXCEED_LIMIT, fname,
				"For a 'bulb' sequence, the maximum number of "
				"images for detector '%s' is 1.  You requested "
				"%ld images, which is not supported.",
						ad->record->name, num_frames );
				}

				exposure_time =
					pccd_170170->exposure_and_gap_step_size;

				frame_time = exposure_time;
			} else
			if ( sp->sequence_type == MXT_SQ_GEOMETRICAL ) {
				num_frames = mx_round( sp->parameter_array[0] );
				exposure_time = sp->parameter_array[1];
				frame_time    = sp->parameter_array[2];

				gap_time = frame_time - exposure_time
						- ad->detector_readout_time;

#if MXD_PCCD_170170_DEBUG
				MX_DEBUG(-2,("%s: num_frames = %ld",
					fname, num_frames));
				MX_DEBUG(-2,("%s: exposure_time = %g",
					fname, exposure_time));
				MX_DEBUG(-2,("%s: frame_time = %g",
					fname, frame_time)); 
				MX_DEBUG(-2,("%s: detector_readout_time = %g",
					fname, ad->detector_readout_time));
				MX_DEBUG(-2,("%s: gap_time = %g",
					fname, gap_time));
#endif

			} else {
				return mx_error( MXE_FUNCTION_FAILED, fname,
				"Inconsistent control structures for "
				"sequence type.  Sequence type = %lu",
					sp->sequence_type );
			}

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE,
					num_frames );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			exposure_steps = mx_round_down( exposure_time
				/ pccd_170170->exposure_and_gap_step_size );

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_EXPOSURE_TIME,
					exposure_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( sp->sequence_type == MXT_SQ_CONTINUOUS ) {
				/* For continuous sequences, we want the
				 * gap time to be one, so that the camera
				 * will take frames as fast as it can.
				 */

				mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_GAP_TIME,
					1 );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			} else
			if ( num_frames > 1 ) {
				/* For sequence types other than
				 * MXT_SQ_CONTINUOUS.
				 */

				gap_time = frame_time - exposure_time
						- ad->detector_readout_time;

#if MXD_PCCD_170170_DEBUG
				MX_DEBUG(-2,
				("%s: num_frames = %ld, gap_time = %g",
					fname, num_frames, gap_time));
#endif

				if ( gap_time < 0.0 ) {
					return mx_error(
						MXE_ILLEGAL_ARGUMENT, fname,
				"The requested time per frame (%g seconds) "
				"is less than the sum of the requested "
				"exposure time (%g seconds) and the detector "
				"readout time (%g seconds) for detector '%s'.",
						frame_time, exposure_time,
						ad->detector_readout_time,
						ad->record->name );
				}

				gap_steps = mx_round_down( gap_time
				    / pccd_170170->exposure_and_gap_step_size );

				if ( gap_steps > 65535 ) {
					return mx_error(
						MXE_ILLEGAL_ARGUMENT, fname,
				"The computed gap time (%g seconds) is "
				"greater than the maximum allowed gap time "
				"(%g seconds) for detector '%s'.",
						gap_time,
			pccd_170170->exposure_and_gap_step_size * (double)65535,
						ad->record->name );
				}

				if ( gap_steps < 1 ) {
					return mx_error(
						MXE_ILLEGAL_ARGUMENT, fname,
				"The computed gap time (%g seconds) is "
				"less than the minimum allowed gap time "
				"(%g seconds) for detector '%s'.",
					gap_time,
					pccd_170170->exposure_and_gap_step_size,
					ad->record->name );
				}

				mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_GAP_TIME,
					gap_steps );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
			break;
		case MXT_SQ_STREAK_CAMERA:
			/* See if the user has requested too few streak
			 * camera lines.
			 */

			num_streak_mode_lines
				= mx_round_down( sp->parameter_array[0] );

#if 0
			if ( num_streak_mode_lines < 1050 ) {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The number of streak camera lines (%ld) requested "
			"for area detector '%s' is less than the minimum "
			"of 1050 lines.  If you need less than 1050 lines, "
			"then use subimage mode instead.",
					num_streak_mode_lines,
					ad->record->name );
			}
#endif

			/* Turn off the multipliers. */

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER,
					0L );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_GAP_MULTIPLIER,
					0L );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Get the current framesize. */

			mx_status = mx_video_input_get_framesize(
					pccd_170170->video_input_record,
					&vinput_horiz_framesize,
					&vinput_vert_framesize );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( old_detector_readout_mode
				!= MXF_PCCD_170170_STREAK_CAMERA_MODE )
			{
				/* Before switching to streak camera mode,
				 * save the video board's current framesize
				 * for later restoration.
				 */

				pccd_170170->vinput_normal_framesize[0]
					= vinput_horiz_framesize;

				pccd_170170->vinput_normal_framesize[1]
					= vinput_vert_framesize;

				pccd_170170->normal_binsize[0] = ad->binsize[0];

				pccd_170170->normal_binsize[1] = ad->binsize[1];

				/* Now switch to streak camera mode. */

				new_control_register_value
					|= MXF_PCCD_170170_STREAK_CAMERA_MODE;
			}

			/* Set the number of frames in sequence to 1. */

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE,
					1 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Set the number of streak camera mode lines.*/

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_STREAK_MODE_LINES,
					num_streak_mode_lines );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Set the exposure time per line. */

			exposure_time = sp->parameter_array[1];

			exposure_steps = mx_round_down( exposure_time
				/ pccd_170170->exposure_and_gap_step_size );

			mx_status = mxd_pccd_170170_write_register(
				pccd_170170,
				MXLV_PCCD_170170_DH_EXPOSURE_TIME,
				exposure_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Compute the gap time per line from the 
			 * total time per line.
			 */

			line_time = sp->parameter_array[2];

			gap_time = line_time - exposure_time;

			gap_steps = mx_round_down( gap_time
				/ pccd_170170->exposure_and_gap_step_size );

			if ( gap_steps < 1 ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The computed gap time (%g seconds) is "
				"less than the minimum allowed gap time "
				"(%g seconds) for detector '%s'.",
					gap_time,
					pccd_170170->exposure_and_gap_step_size,
					ad->record->name );
			}

			/* Set the gap time. */

			mx_status = mxd_pccd_170170_write_register(
				pccd_170170,
				MXLV_PCCD_170170_DH_GAP_TIME,
				gap_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Set the streak-mode framesize. */

			mx_status = mx_video_input_set_framesize(
				pccd_170170->video_input_record,
				vinput_horiz_framesize,
				num_streak_mode_lines / 2L );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Get the detector readout time. */

			mx_status = mx_area_detector_get_detector_readout_time(
							ad->record, NULL );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			break;

		case MXT_SQ_SUBIMAGE:
			total_num_subimage_lines =
					mx_round( sp->parameter_array[0]
						* sp->parameter_array[1] );

			if ( total_num_subimage_lines >
					MXD_MAXIMUM_TOTAL_SUBIMAGE_LINES )
			{
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested number of lines per subimage (%g) "
			"and the requested number of subimages (%g) results "
			"in a total number of image lines (%ld) that is "
			"larger than the maximum number of subimage lines (%d) "
			"allowed by area detector '%s'.",
					sp->parameter_array[0],
					sp->parameter_array[1],
					total_num_subimage_lines,
					MXD_MAXIMUM_TOTAL_SUBIMAGE_LINES,
					ad->record->name );
			}

			if ( old_detector_readout_mode
				!= MXF_PCCD_170170_SUBIMAGE_MODE )
			{
				/* Switch to sub-image mode. */

				new_control_register_value
				    &= (~MXF_PCCD_170170_DETECTOR_READOUT_MASK);

				new_control_register_value
					|= MXF_PCCD_170170_SUBIMAGE_MODE;

				/* If we were previously in streak camera mode,
				 * we must also restore the normal imaging
				 * board framesize.
				 */

				if ( old_detector_readout_mode
					== MXF_PCCD_170170_STREAK_CAMERA_MODE )
				{
				    mx_status = mx_video_input_set_framesize(
					pccd_170170->video_input_record,
					pccd_170170->vinput_normal_framesize[0],
				       pccd_170170->vinput_normal_framesize[1]);

				    if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				    mx_status = mx_area_detector_set_binsize(
					pccd_170170->record,
					pccd_170170->normal_binsize[0],
				       pccd_170170->normal_binsize[1]);

				    if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
				}
			}

			/* Set the number of frames in sequence to 1. */

			mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE,
					1 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Set the number of lines per subimage.
			 * The number of subimage lines is twice
			 * the value of the "subframe size".
			 */

			mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_SUBFRAME_SIZE,
				    mx_round_down(sp->parameter_array[0]/2.0) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Set the number of subimages per frame. */

			mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_SUBIMAGES_PER_READ,
					mx_round_down(sp->parameter_array[1]) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Set the exposure time. */

			exposure_time = sp->parameter_array[2];

			exposure_steps = mx_round_down( exposure_time
				/ pccd_170170->exposure_and_gap_step_size );

			mx_status = mxd_pccd_170170_write_register(
				pccd_170170,
				MXLV_PCCD_170170_DH_EXPOSURE_TIME,
				exposure_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Compute the gap time from the subimage time. */

			subimage_time = sp->parameter_array[3];

			gap_time = subimage_time - exposure_time;

			gap_steps = mx_round_down( gap_time
				/ pccd_170170->exposure_and_gap_step_size );

			if ( gap_steps < 1 ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The computed gap time (%g seconds) is "
				"less than the minimum allowed gap time "
				"(%g seconds) for detector '%s'.",
					gap_time,
					pccd_170170->exposure_and_gap_step_size,
					ad->record->name );
			}

			/* Set the gap time. */

			mx_status = mxd_pccd_170170_write_register(
				pccd_170170,
				MXLV_PCCD_170170_DH_GAP_TIME,
				gap_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
					
			exposure_multiplier = sp->parameter_array[4];

			exposure_multiplier_steps = 
			   mx_round_down( (exposure_multiplier - 1.0) * 256.0 );

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER,
					exposure_multiplier_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			gap_multiplier = sp->parameter_array[5];

			gap_multiplier_steps = 
				mx_round_down( (gap_multiplier - 1.0) * 256.0 );

			mx_status = mxd_pccd_170170_write_register(
					pccd_170170,
					MXLV_PCCD_170170_DH_GAP_MULTIPLIER,
					gap_multiplier_steps );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* For subimage mode, getting the detector readout
			 * time is the last thing we do, since the value
			 * of the detector readout time depends on some of
			 * the values set above.
			 */

			/* Get the detector readout time. */

			mx_status = mx_area_detector_get_detector_readout_time(
							ad->record, NULL );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			break;

		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal sequence type %ld requested "
			"for detector '%s'.",
				sp->sequence_type, ad->record->name );
		}

		/* Reprogram the control register for the new mode. */

#if MXD_PCCD_170170_DEBUG
		MX_DEBUG(-2, ("%s: old_control_register_value = %#lx",
				fname, old_control_register_value));
		MX_DEBUG(-2, ("%s: new_control_register_value = %#lx",
				fname, new_control_register_value));
#endif

		if (new_control_register_value != old_control_register_value) {

			mx_status = mxd_pccd_170170_write_register(
						pccd_170170,
						MXLV_PCCD_170170_DH_CONTROL,
						new_control_register_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		/* Reprogram the imaging board. */

		mx_status = mx_video_input_set_sequence_parameters(
					pccd_170170->video_input_record, sp );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the trigger mode for the imaging board. */

		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_CONTINUOUS:
			mx_status = mx_video_input_set_trigger_mode( 
						pccd_170170->video_input_record,
						MXT_IMAGE_INTERNAL_TRIGGER );
			break;

		case MXT_SQ_STROBE:
		case MXT_SQ_BULB:
			mx_status = mx_video_input_set_trigger_mode( 
						pccd_170170->video_input_record,
						MXT_IMAGE_EXTERNAL_TRIGGER );
			break;

		case MXT_SQ_MULTIFRAME:
		case MXT_SQ_CIRCULAR_MULTIFRAME:
		case MXT_SQ_GEOMETRICAL:
		case MXT_SQ_STREAK_CAMERA:
		case MXT_SQ_SUBIMAGE:
			/* For these cases, we leave the trigger mode alone,
			 * since we may want to use either internal triggering
			 * or external triggering.
			 */
			break;

		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal sequence type %ld requested "
			"for detector '%s'.",
				sp->sequence_type, ad->record->name );
		}

		/* Insert a delay to give the detector head FPGAs time to
		 * finish their processing.
		 */

		mx_msleep(1);

		break; 

	case MXLV_AD_SEQUENCE_START_DELAY:
		new_delay_time = mx_round( 1.0e5 * ad->sequence_start_delay );

		mx_status = mxd_pccd_170170_write_register( pccd_170170,
					MXLV_PCCD_170170_DH_INITIAL_DELAY_TIME,
					new_delay_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_video_input_set_trigger_mode(
				pccd_170170->video_input_record,
				ad->trigger_mode );
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Changing parameter '%s' for area detector '%s' is not supported.",
			mx_get_field_label_string( ad->record,
				ad->parameter_type ), ad->record->name );
		break;

	case MXLV_AD_REGISTER_VALUE:
		mx_status = mxd_pccd_170170_set_register_value( ad,
							pccd_170170, -1 );
		break;

	case MXLV_AD_SHUTTER_ENABLE:
		if ( ad->shutter_enable ) {
			shutter_disable = 0;
		} else {
			shutter_disable = 1;
		}

		mx_status = mx_area_detector_set_register( ad->record,
							"dh_shutter_disable",
							shutter_disable );
		break;

	default:
		if ( ad->parameter_type >= MXLV_PCCD_170170_DH_BASE ) {
			mx_status = mxd_pccd_170170_set_register_value( ad,
					    pccd_170170, ad->parameter_type );
		} else {
			mx_status =
			    mx_area_detector_default_set_parameter_handler(ad);
		}
		break;
	}

	return mx_status;
}

#if MXD_PCCD_170170_GEOMETRICAL_MASK_KLUDGE

static mx_status_type
mxp_pccd_170170_geometrical_mask_kludge( MX_AREA_DETECTOR *ad,
					MX_PCCD_170170 *pccd_170170 )
{
	static const char fname[] = "mxp_pccd_170170_geometrical_mask_kludge()";

	MX_IMAGE_FRAME *mask_frame;
	long mask_row_binsize, mask_column_binsize;
	long mask_row_framesize, mask_column_framesize;
	long unbinned_row_framesize, unbinned_column_framesize;

	mask_frame = pccd_170170->geometrical_mask_frame;

	if ( mask_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The geometrical_mask_frame pointer for area detector '%s' "
		"is NULL at a time when it should not be NULL.",
			ad->record->name );
	}

	mask_row_binsize    = MXIF_ROW_BINSIZE(mask_frame);
	mask_column_binsize = MXIF_COLUMN_BINSIZE(mask_frame);

	/* If either the mask row binsize or the mask column binsize
	 * is not 1, then we assume that the bin sizes for the mask
	 * frame have been set correctly and return.
	 */

	if ( ( mask_row_binsize != 1 )
	  || ( mask_column_binsize != 1 ) )
	{
		return MX_SUCCESSFUL_RESULT;
	}

#if MXD_PCCD_170170_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,("%s: computing mask binsize for area detector '%s'.",
		fname, ad->record->name ));
#endif

	/* If we get here, then we need to attempt to figure out what the
	 * real mask binsize is.
	 */

	/* FIXME: We _assume_ that an unbinned image is square and has a
	 * width that is identical to the maximum row width of the image
	 * frame, namely, ad->maximum_framesize[0].
	 */

	unbinned_row_framesize = ad->maximum_framesize[0];
	unbinned_column_framesize = unbinned_row_framesize;

	mask_row_framesize    = MXIF_ROW_FRAMESIZE(mask_frame);
	mask_column_framesize = MXIF_COLUMN_FRAMESIZE(mask_frame);

#if MXD_PCCD_170170_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,
	("%s: unbinned_row_framesize = %lu, unbinned_column_framesize = %lu",
		fname, unbinned_row_framesize, unbinned_column_framesize));
	MX_DEBUG(-2,
	("%s: mask_row_framesize = %lu, mask_column_framesize = %lu",
		fname, mask_row_framesize, mask_column_framesize));
#endif

	if ( ( mask_row_framesize > unbinned_row_framesize )
	  || ( mask_column_framesize > unbinned_column_framesize ) )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"One or more of the dimensions (%ld,%ld) of the "
		"mask frame '%s' is larger than the maximum image "
		"size (%ld,%ld) for area detector '%s'.",
			mask_row_framesize, mask_column_framesize,
			pccd_170170->geometrical_mask_filename,
			unbinned_row_framesize, unbinned_column_framesize,
			ad->record->name );
	}

	if ( ( (unbinned_row_framesize % mask_row_framesize) != 0 )
	  || ( (unbinned_column_framesize % mask_column_framesize) != 0 ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"One or more of the dimensions (%ld,%ld) of the "
		"mask frame '%s' is not an integer fraction of "
		"the maximum image size (%ld,%ld) for area detector '%s'.",
			mask_row_framesize, mask_column_framesize,
			pccd_170170->geometrical_mask_filename,
			unbinned_row_framesize, unbinned_column_framesize,
			ad->record->name );
	}

	mask_row_binsize = unbinned_row_framesize / mask_row_framesize;
	mask_column_binsize = unbinned_column_framesize / mask_column_framesize;

#if MXD_PCCD_170170_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,
	("%s: mask_row_binsize = %lu, mask_column_binsize = %lu",
		fname, mask_row_binsize, mask_column_binsize));
#endif

	MXIF_ROW_BINSIZE(mask_frame)    = mask_row_binsize;
	MXIF_COLUMN_BINSIZE(mask_frame) = mask_column_binsize;

	return MX_SUCCESSFUL_RESULT;
}

#endif

static mx_status_type
mxd_pccd_170170_setup_geometrical_mask_frame( MX_AREA_DETECTOR *ad,
				MX_PCCD_170170 *pccd_170170,
				MX_IMAGE_FRAME *image_frame,
				uint16_t **geometrical_mask_frame_buffer )
{
#if MXD_PCCD_170170_DEBUG_SETUP_GEOMETRICAL_MASK
	static const char fname[] =
			"mxd_pccd_170170_setup_geometrical_mask_frame()";
#endif

	MX_IMAGE_FRAME *mask_frame;
	void *image_data_pointer;
	long image_row_binsize, image_column_binsize;
	long mask_row_binsize, mask_column_binsize;
	long rebinned_row_binsize, rebinned_column_binsize;
	mx_bool_type need_rebinned_mask_frame, new_mask_frame_loaded;
	mx_bool_type create_rebinned_mask_frame;
	mx_status_type mx_status;

	image_row_binsize    = MXIF_ROW_BINSIZE(image_frame);
	image_column_binsize = MXIF_COLUMN_BINSIZE(image_frame);

#if MXD_PCCD_170170_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,("%s: image_row_binsize = %ld, image_column_binsize = %ld",
		fname, image_row_binsize, image_column_binsize));

	MX_DEBUG(-2,("%s: ad->binsize[0] = %ld, ad->binsize[1] = %ld",
		fname, ad->binsize[0], ad->binsize[1]));
#endif

	/* Make sure that the unbinned geometrical mask frame
	 * has been loaded.
	 */

	if ( pccd_170170->geometrical_mask_frame == (MX_IMAGE_FRAME *) NULL ) {
		mx_image_free( pccd_170170->rebinned_geometrical_mask_frame );

		pccd_170170->rebinned_geometrical_mask_frame = NULL;

		mx_status = mx_image_read_smv_file(
				&(pccd_170170->geometrical_mask_frame),
				pccd_170170->geometrical_mask_filename );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXD_PCCD_170170_GEOMETRICAL_MASK_KLUDGE
		mx_status = mxp_pccd_170170_geometrical_mask_kludge( ad,
								pccd_170170 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
#endif

		new_mask_frame_loaded = TRUE;
	} else {
		new_mask_frame_loaded = FALSE;
	}

	mask_row_binsize =
		MXIF_ROW_BINSIZE(pccd_170170->geometrical_mask_frame);

	mask_column_binsize =
		MXIF_COLUMN_BINSIZE(pccd_170170->geometrical_mask_frame);

#if MXD_PCCD_170170_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,("%s: mask_row_binsize = %ld, mask_column_binsize = %ld",
		fname, mask_row_binsize, mask_column_binsize));
#endif

	/* Do we need to use a rebinned geometrical mask frame? */

	if ( mask_row_binsize != image_row_binsize ) {
		need_rebinned_mask_frame = TRUE;
	} else
	if ( mask_column_binsize != image_column_binsize ) {
		need_rebinned_mask_frame = TRUE;
	} else {
		need_rebinned_mask_frame = FALSE;
	}

#if MXD_PCCD_170170_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,
	("%s: new_mask_frame_loaded = %d, need_rebinned_mask_frame = %d",
		fname, (int) new_mask_frame_loaded,
		(int) need_rebinned_mask_frame ));
#endif

	/* If it is needed, see if the correct rebinned mask image
	 * is already present.
	 */

	if ( need_rebinned_mask_frame == FALSE ) {

		/* If we do not need a rebinned mask frame, then discard any 
		 * such frame that currently exists.
		 */

		mx_image_free( pccd_170170->rebinned_geometrical_mask_frame );

		pccd_170170->rebinned_geometrical_mask_frame = NULL;
	} else {
		/* We _do_ need a rebinned mask frame.
		 * Do we need to create a new one now?
		 */

		create_rebinned_mask_frame = FALSE;

		if ( new_mask_frame_loaded ) {
			create_rebinned_mask_frame = TRUE;
		} else
		if ( pccd_170170->rebinned_geometrical_mask_frame == NULL ) {
			create_rebinned_mask_frame = TRUE;
		} else {
			/* See if the existing rebinned mask frame already has
			 * the dimensions that we need.  If so, then we just
			 * use the already existing mask frame.
			 */

			rebinned_row_binsize =
	    MXIF_ROW_BINSIZE(pccd_170170->rebinned_geometrical_mask_frame);
		
			rebinned_column_binsize =
	    MXIF_COLUMN_BINSIZE(pccd_170170->rebinned_geometrical_mask_frame);

#if MXD_PCCD_170170_DEBUG_SETUP_GEOMETRICAL_MASK
			MX_DEBUG(-2,
	    ("%s: rebinned_row_binsize = %ld, rebinned_column_binsize = %ld",
			fname, rebinned_row_binsize, rebinned_column_binsize));
#endif
	    		if ( (rebinned_row_binsize != image_row_binsize)
			  || (rebinned_column_binsize != image_column_binsize) )
			{
				create_rebinned_mask_frame = TRUE;
			}
		}

		/* If necessary, create a new rebinned mask frame. */

		if ( create_rebinned_mask_frame ) {
			unsigned long row_framesize, column_framesize;

			row_framesize = MXIF_ROW_FRAMESIZE(image_frame);
			column_framesize = MXIF_COLUMN_FRAMESIZE(image_frame);

			mx_status = mx_image_rebin(
			    &(pccd_170170->rebinned_geometrical_mask_frame),
			    	pccd_170170->geometrical_mask_frame,
				row_framesize, column_framesize );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/* Return the frame buffer pointer for the geometrical mask frame
	 * that we are using.
	 */

	if ( need_rebinned_mask_frame == FALSE ) {
		mask_frame = pccd_170170->geometrical_mask_frame;
	} else {
		mask_frame = pccd_170170->rebinned_geometrical_mask_frame;
	}

#if MXD_PCCD_170170_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,("%s: pccd_170170->geometrical_mask_frame = %p",
		fname, pccd_170170->geometrical_mask_frame));

	MX_DEBUG(-2,("%s: pccd_170170->rebinned_geometrical_mask_frame = %p",
		fname, pccd_170170->rebinned_geometrical_mask_frame));

	MX_DEBUG(-2,("%s: mask_frame = %p", fname, mask_frame));
#endif

	mx_status = mx_image_get_image_data_pointer( mask_frame, NULL,
							&image_data_pointer );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*geometrical_mask_frame_buffer = image_data_pointer;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_geometrical_correction( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame )
{
	static const char fname[] = "mxd_pccd_170170_geometrical_correction()";

	MX_PCCD_170170 *pccd_170170;
	int spatial_status, os_status, saved_errno;
	int row_framesize, column_framesize;
	uint16_t *geometrical_mask_frame_buffer;
	mx_status_type mx_status;

	/* Suppress stupid GCC 4 initialization warnings. */

	pccd_170170 = NULL;
	geometrical_mask_frame_buffer = NULL;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {

		if ( ad->image_frame != (MX_IMAGE_FRAME *) NULL ) {
			image_frame = ad->image_frame;
		} else {
			return mx_error( MXE_NOT_READY, fname,
			"Area detector '%s' has not yet taken an image frame.",
				ad->record->name );
		}
	}

	if ( image_frame->image_data == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	  "The image_frame->image_data pointer for area detector '%s' is NULL.",
			ad->record->name );
	}
	if ( pccd_170170->geometrical_spline_filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The geometrical_spline_filename pointer for "
		"area detector '%s' is NULL.", ad->record->name );
	}
	if ( strlen( pccd_170170->geometrical_spline_filename ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"No geometrical correction spline filename was provided for "
		"area detector '%s'.", ad->record->name );
	}
	if ( pccd_170170->geometrical_mask_filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The geometrical_mask_filename pointer for "
		"area detector '%s' is NULL.", ad->record->name );
	}
	if ( strlen( pccd_170170->geometrical_mask_filename ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"No geometrical correction mask filename was provided for "
		"area detector '%s'.", ad->record->name );
	}

	os_status = access( pccd_170170->geometrical_spline_filename, R_OK );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot read geometrical correction spline file '%s'.  "
		"errno = %d, error message = '%s'.",
			pccd_170170->geometrical_spline_filename,
			saved_errno, strerror(saved_errno) );
	}

	os_status = access( pccd_170170->geometrical_mask_filename, R_OK );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot read geometrical correction mask file '%s'.  "
		"errno = %d, error message = '%s'.",
			pccd_170170->geometrical_mask_filename,
			saved_errno, strerror(saved_errno) );
	}

	/* Make sure that the geometrical mask frame is binned to the
	 * correct size.
	 */

	DISPLAY_MEMORY_USAGE( "BEFORE setup of geometrical mask" );

	mx_status = mxd_pccd_170170_setup_geometrical_mask_frame(
			ad, pccd_170170, image_frame,
			&geometrical_mask_frame_buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	row_framesize = MXIF_ROW_FRAMESIZE(image_frame);
	column_framesize = MXIF_COLUMN_FRAMESIZE(image_frame);

#if MXD_PCCD_170170_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("BEFORE smvspatial(), image_frame histogram = "));
	mx_image_statistics( image_frame );
#endif

#if 0
	DISPLAY_MEMORY_USAGE( "BEFORE kludge" );

	/* Kludge: free internal smvspatial() memory. */

	smvspatial( NULL, -1, -1, 0, NULL, NULL );
#endif

	DISPLAY_MEMORY_USAGE( "BEFORE smvspatial()" );

#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(_MSC_VER)
	/* Perform the geometrical correction. */

	spatial_status = smvspatial( image_frame->image_data, 
				row_framesize, column_framesize, 0,
				pccd_170170->geometrical_spline_filename,
				geometrical_mask_frame_buffer );
#else
	mx_warning("XGEN geometrical correction is currently only available "
		"on Linux, Windows, and MacOS X.");

	spatial_status = 0;
#endif

#if 0
	fflush(stdout);		/* Thank you MSI! */
#endif

	DISPLAY_MEMORY_USAGE( "AFTER smvspatial()" );

	switch( spatial_status ) {
	case 0:		/* Spatial correction succeeded. */
		mx_status = MX_SUCCESSFUL_RESULT;
		break;
	case ENOENT:
		mx_status = mx_error( MXE_FILE_IO_ERROR, fname,
			"smvspatial() was unable to open one of the correction "
			"files '%s' or '%s' for area detector '%s'.",
				pccd_170170->geometrical_spline_filename,
				pccd_170170->geometrical_mask_filename,
				ad->record->name );
		break;
	case EIO:
		mx_status = mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred in smvspatial() while reading one "
			"of the correction files '%s' or '%s' for "
			"area detector '%s'.",
				pccd_170170->geometrical_spline_filename,
				pccd_170170->geometrical_mask_filename,
				ad->record->name );
		break;
	case ENOMEM:
		mx_status = mx_error( MXE_OUT_OF_MEMORY, fname,
			"smvspatial() was unable to allocate memory for "
			"spatial correction of the current image for "
			"area detector '%s'.",
				ad->record->name );
		break;
	case EINVAL:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"An illegal value was seen by smvspatial() during "
			"spatial correction of the current image for "
			"area detector '%s'.",
				ad->record->name );
		break;
	default:
		mx_status = mx_error( MXE_UNKNOWN_ERROR, fname,
			"Unexpected error code %d was returned by "
			"smvspatial() for area detector '%s'.",
				spatial_status, ad->record->name );
		break;
	}

#if MXD_PCCD_170170_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("AFTER smvspatial(), image_frame histogram = "));
	mx_image_statistics( image_frame );
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_camera_link_command( MX_PCCD_170170 *pccd_170170,
					char *command,
					char *response,
					size_t max_response_length,
					int debug_flag )
{
	static const char fname[] = "mxd_pccd_170170_camera_link_command()";

	MX_RECORD *camera_link_record;
	size_t command_length, response_length;
	size_t i, bytes_available, bytes_to_read, total_bytes_read, buffer_left;
	char *ptr;
	unsigned long use_dh_simulator;
	double timeout_in_seconds;
	MX_CLOCK_TICK timeout_in_clock_ticks;
	MX_CLOCK_TICK start_tick, timeout_tick, current_tick;
	int comparison;
	mx_status_type mx_status;

#if MXD_PCCD_170170_DEBUG_SERIAL
	debug_flag |= 1;
#endif

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed for record '%s' was NULL.",
			pccd_170170->record->name );
	}
	if ( response == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The response pointer passed for record '%s' was NULL.",
			pccd_170170->record->name );
	}
	if ( max_response_length < 1 ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The requested response buffer length %lu for record '%s' "
		"is too short to hold a minimum length response.",
			(unsigned long) max_response_length,
			pccd_170170->record->name );
	}

	camera_link_record = pccd_170170->camera_link_record;

	if ( camera_link_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The camera_link_record pointer for record '%s' is NULL.",
			pccd_170170->record->name );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sent '%s' to '%s'",
			fname, command, camera_link_record->name ));
	}

	use_dh_simulator = pccd_170170->pccd_170170_flags
				& MXF_PCCD_170170_USE_DETECTOR_HEAD_SIMULATOR;

	if ( use_dh_simulator ) {

		/* Talk to a simulated detector head. */

		mx_status = mxd_pccd_170170_simulated_cl_command( 
						pccd_170170, command,
						response, max_response_length,
						debug_flag );
	} else {
		/* Talk to the real detector head. */

		/* Throw away any unread input.  It is likely to be garbage
		 * from electromagnetic interference.
		 */

		mx_status = mx_camera_link_flush_port( camera_link_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Send the command. */

		command_length = strlen(command);

		mx_status = mx_camera_link_serial_write( camera_link_record,
						command, &command_length, -1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Arrange for a timeout if it takes too long to get a
		 * response from the detector head.
		 */

		timeout_in_seconds = 1.0;

		timeout_in_clock_ticks = 
			mx_convert_seconds_to_clock_ticks( timeout_in_seconds );

		start_tick = mx_current_clock_tick();

		timeout_tick = mx_add_clock_ticks( start_tick,
						timeout_in_clock_ticks );

		/* Leave room in the response buffer to null terminate
		 * the response.
		 */

		response_length = max_response_length - 1;

		total_bytes_read = 0;

		for (;;) {
			mx_msleep(1);	/* Wait 1 millisecond. */

			mx_status = mx_camera_link_get_num_bytes_avail(
							camera_link_record,
							&bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			ptr = response + total_bytes_read;

			buffer_left = response_length - total_bytes_read;

			if ( bytes_available < 1 ) {
#if 0 && MXD_PCCD_170170_DEBUG_SERIAL
				MX_DEBUG(-2,
				("%s: No bytes available from '%s'.",
					fname, camera_link_record->name));
#endif
			}

#if 0
			bytes_to_read = 1;
#else
			if ( bytes_available > 0 ) {
				bytes_to_read = 1;
			} else {
				bytes_to_read = 0;
			}
#endif

			if ( bytes_to_read > buffer_left ) {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"The number of bytes that we want to "
				"read (%ld) is larger than the space "
				"left (%ld) in the "
				"response buffer for Camera Link record '%s'.",
					(long) bytes_to_read,
					(long) buffer_left,
					camera_link_record->name );
			}

			/* The Camera Link specification implies that you
			 * should never attempt to read more characters than
			 * were said to be available by a previous call to
			 * mx_camera_link_get_num_bytes_available().
			 *
			 * If you do attempt to read more bytes than are
			 * available, the spec says that no bytes should
			 * be read and you should get back an error code.
			 */

			mx_status = mx_camera_link_serial_read(
				camera_link_record, ptr, &bytes_to_read, -1);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			total_bytes_read += bytes_to_read;

			/* Were there any carriage returns in the bytes
			 * that we just received?  If so, we have reached
			 * the end of the response and can now break out of
			 * this loop.
			 */

			for ( i = 0; i < bytes_to_read; i++ ) {
				if ( ptr[i] == MX_CR ) {
					ptr[i] = '\0';
					break;	/* Exit the for(i) loop. */
				}
			}

			if ( i < bytes_to_read ) {
				/* We saw a carriage return, so break out
				 * of the for(;;) loop as well.
				 */
				break;
			}

			current_tick = mx_current_clock_tick();

			comparison = mx_compare_clock_ticks( current_tick,
								timeout_tick );

			if ( comparison >= 0 ) {
				return mx_error( MXE_TIMED_OUT, fname,
				"Timed out after waiting %g seconds for "
				"a response to the '%s' command sent to '%s'.",
					timeout_in_seconds, command,
					camera_link_record->name );
			}
		}

		response[total_bytes_read] = '\0';
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response, camera_link_record->name ));
	}

	if ( response[0] == 'E' ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error response received for command '%s' sent to "
		"Camera Link interface '%s'.",
			command, camera_link_record->name );
	}
	
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_read_register( MX_PCCD_170170 *pccd_170170,
				unsigned long register_address,
				unsigned long *register_value )
{
	static const char fname[] = "mxd_pccd_170170_read_register()";

	char command[20], response[20];
	int num_items;
	mx_status_type mx_status;

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}
	if ( register_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The register_value pointer passed for record '%s' was NULL.",
			pccd_170170->record->name );
	}

	if ( register_address >= MXLV_PCCD_170170_DH_BASE ) {
		register_address -= MXLV_PCCD_170170_DH_BASE;
	}

	snprintf( command, sizeof(command), "R%03lu", register_address );

	mx_status = mxd_pccd_170170_camera_link_command( pccd_170170,
					command, response, sizeof(response),
					MXD_PCCD_170170_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "S%lu", register_value );

	if ( num_items != 1 ) {
		/* The first read from the detector head may fail if the
		 * detector head has just been powered on.  If so, then
		 * we just retry the command without generating an
		 * error message.
		 */

		if ( pccd_170170->first_dh_command ) {
			pccd_170170->first_dh_command = FALSE;

			mx_status = mxd_pccd_170170_read_register(
						pccd_170170,
						register_address,
						register_value );

			return mx_status;
		} else {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Could not find the register value in the response "
			"'%s' by detector '%s' to the command '%s'.",
				response, pccd_170170->record->name, command );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_write_register( MX_PCCD_170170 *pccd_170170,
				unsigned long register_address,
				unsigned long register_value )
{
	static const char fname[] = "mxd_pccd_170170_write_register()";

	char command[20], response[20];
	unsigned long value_read, flags;
	mx_status_type mx_status;

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}

	flags = pccd_170170->pccd_170170_flags;

	if ( register_address >= MXLV_PCCD_170170_DH_BASE ) {
		register_address -= MXLV_PCCD_170170_DH_BASE;
	}

	mx_status = mxd_pccd_170170_check_value( pccd_170170,
					register_address, register_value, NULL);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"W%03lu%05lu", register_address, register_value );

	/* Write the new value. */

	mx_status = mxd_pccd_170170_camera_link_command( pccd_170170,
					command, response, sizeof(response),
					MXD_PCCD_170170_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the value back to verify that the value was set correctly. */

	mx_status = mxd_pccd_170170_read_register( pccd_170170,
						register_address, &value_read );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the value read back matches the value sent. */

	if ( value_read != register_value ) {
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"The attempt to set '%s' register %lu to %lu "
		"appears to have failed since the value read back "
		"from that register is now %lu.",
			pccd_170170->record->name, register_address,
			register_value, value_read );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

/* mxp_pccd_170170_geometrical_mask_changed() is invoked if someone has
 * written to the 'geometrical_mask_filename' field.  All the function
 * does is discard any existing geometrical mask data structures.
 */

static mx_status_type
mxp_pccd_170170_geometrical_mask_changed( MX_RECORD *record )
{
	static const char fname[] =
			"mxp_pccd_170170_geometrical_mask_changed()";

	MX_PCCD_170170 *pccd_170170;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pccd_170170 = record->record_type_struct;

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PCCD_170170 pointer for record '%s' is NULL.",
			record->name );
	}

#if MXD_PCCD_170170_DEBUG_SETUP_GEOMETRICAL_MASK
	MX_DEBUG(-2,("%s invoked for area detector '%s', filename = '%s'",
		fname, record->name, pccd_170170->geometrical_mask_filename));
#endif

	mx_image_free( pccd_170170->geometrical_mask_frame );

	pccd_170170->geometrical_mask_frame = NULL;

	mx_image_free( pccd_170170->rebinned_geometrical_mask_frame );

	pccd_170170->rebinned_geometrical_mask_frame = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_pccd_170170_special_processing_setup( MX_RECORD *record )
{
#if MXD_PCCD_170170_DEBUG
	static const char fname[] =
		"mxd_pccd_170170_special_processing_setup()";
#endif

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		if ( record_field->label_value >= MXLV_PCCD_170170_DH_BASE ) {
			record_field->process_function
					= mxd_pccd_170170_process_function;
		} else {
			switch( record_field->label_value ) {
			case MXLV_PCCD_170170_GEOMETRICAL_MASK_FILENAME:
				record_field->process_function
					= mxd_pccd_170170_process_function;
				break;
			default:
				break;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_process_function( void *record_ptr,
				void *record_field_ptr,
				int operation )
{
	static const char fname[] = "mxd_pccd_170170_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_AREA_DETECTOR *ad;
	unsigned long *register_value_ptr;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	record_field = (MX_RECORD_FIELD *) record_field_ptr;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AREA_DETECTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		if ( record_field->label_value >= MXLV_PCCD_170170_DH_BASE ) {
			mx_status = mx_area_detector_get_register(
					record, record_field->name, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			register_value_ptr =
				mx_get_field_value_pointer( record_field );

			*register_value_ptr = ad->register_value;
		}
		break;

	case MX_PROCESS_PUT:
		if ( record_field->label_value >= MXLV_PCCD_170170_DH_BASE ) {

			register_value_ptr =
				mx_get_field_value_pointer( record_field );

			mx_status = mx_area_detector_set_register(
					record, record_field->name,
					*register_value_ptr );
		} else {
			switch( record_field->label_value ) {
			case MXLV_PCCD_170170_GEOMETRICAL_MASK_FILENAME:
				mx_status =
			    mxp_pccd_170170_geometrical_mask_changed( record );

				break;
			default:
				break;
			}
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unknown operation code = %d for record '%s'.",
			operation, record->name );
	}

	return mx_status;
}

