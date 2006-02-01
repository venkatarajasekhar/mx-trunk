/*
 * Name:    mx_array.h
 *
 * Purpose: Header for MX array handling functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2000, 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_ARRAY_H__
#define __MX_ARRAY_H__

MX_API void *mx_allocate_array( mx_length_type num_dimensions,
				mx_length_type *dimension_array,
				size_t *data_element_size_array );

MX_API mx_status_type mx_free_array( void *array_pointer,
				mx_length_type num_dimensions,
				mx_length_type *dimension_array,
				size_t *data_element_size_array );

MX_API void *mx_read_void_pointer_from_memory_location(
					void *memory_location );

MX_API void mx_write_void_pointer_to_memory_location(
					void *memory_location, void *ptr );

MX_API mx_status_type mx_copy_array_to_buffer( void *array_pointer,
		int array_is_dynamically_allocated,
		long mx_datatype,
		mx_length_type num_dimensions,
		mx_length_type *dimension_array,
		size_t *data_element_size_array,
		void *destination_buffer,
		size_t destination_buffer_length,
		size_t *num_bytes_copied );

MX_API mx_status_type mx_copy_buffer_to_array(
		void *source_buffer, size_t source_buffer_length,
		void *array_pointer,
		int array_is_dynamically_allocated,
		long mx_datatype,
		mx_length_type num_dimensions,
		mx_length_type *dimension_array,
		size_t *data_element_size_array,
		size_t *num_bytes_copied );

#define MX_XDR_ENCODE	0
#define MX_XDR_DECODE	1

MX_API mx_status_type mx_xdr_data_transfer( int direction,
		void *array_pointer,
		int array_is_dynamically_allocated,
		long mx_datatype,
		mx_length_type num_dimensions,
		mx_length_type *dimension_array,
		size_t *data_element_size_array,
		void *destination_buffer,
		size_t destination_buffer_length,
		size_t *num_bytes_copied );

MX_API mx_status_type mx_convert_and_copy_array(
		void *source_array_pointer,
		long source_datatype,
		mx_length_type source_num_dimensions,
		mx_length_type *source_dimension_array,
		size_t *source_data_element_size_array,
		void *destination_array_pointer,
		long destination_datatype,
		mx_length_type destination_num_dimensions,
		mx_length_type *destination_dimension_array,
		size_t *destination_data_element_size_array );

#endif /* __MX_ARRAY_H__ */

