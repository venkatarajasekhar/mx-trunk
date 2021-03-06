/*
 * Name:    mx_security.c
 *
 * Purpose: Access security functions for MX-based servers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2006, 2009, 2011-2012, 2016
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_SECURITY_DEBUG	FALSE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_array.h"
#include "mx_variable.h"

#include "mx_process.h"
#include "mx_security.h"

#define NUMERICAL_ADDRESS_CHARACTERS	"0123456789.*?"

MX_EXPORT mx_status_type
mx_setup_connection_acl( MX_RECORD *record_list,
			const char *connection_acl_filename )
{
	static const char fname[] = "mx_setup_connection_acl()";

	MX_CONNECTION_ACL *connection_acl;
	FILE *connection_acl_file;
	MX_LIST_HEAD *list_head;
	long dimension_array[2];
	size_t data_element_size_array[2];
	size_t result, length;
	long i, num_addresses;
	char *address_string;
	char buffer[MXU_HOSTNAME_LENGTH+1];

	if ( record_list == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	/* Make sure this is the list_head. */

	record_list = record_list->list_head;

	list_head = (MX_LIST_HEAD *)(record_list->record_superclass_struct);

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LIST_HEAD pointer in list head record is NULL." );
	}

	mx_info( "Using access control list file '%s'.",
			connection_acl_filename );

	/* Try to open the connection ACL file. */

	connection_acl_file = fopen( connection_acl_filename, "r" );

	if ( connection_acl_file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The connection ACL file '%s' cannot be opened for reading.",
			connection_acl_filename );
	}

	/* How many lines are there in the file? */

	num_addresses = -1;

	while( !feof( connection_acl_file ) ) {
		if ( ferror(connection_acl_file) ) {
			fclose(connection_acl_file);

			return mx_error( MXE_FILE_IO_ERROR, fname,
		"File error occurred while reading connection ACL file '%s'",
				connection_acl_filename );
		}

		num_addresses++;

		mx_fgets(buffer, sizeof buffer, connection_acl_file);
	}

	/* Allocate memory for the connection ACL structures. */

	connection_acl = (MX_CONNECTION_ACL *)
				malloc( sizeof(MX_CONNECTION_ACL) );

	if ( connection_acl == (MX_CONNECTION_ACL *) NULL ) {
		fclose(connection_acl_file);

		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to allocate an MX_CONNECTION_ACL structure." );
	}

	connection_acl->num_addresses = num_addresses;

	list_head->connection_acl = connection_acl;

	/* Is the ACL list empty? */

	if ( num_addresses == 0 ) {

		/* If so, we can return now. */

		connection_acl->is_numerical_address = NULL;
		connection_acl->address_string_array = NULL;

		mx_warning( "ACL list '%s' has zero entries.",
				connection_acl_filename );

		return MX_SUCCESSFUL_RESULT;
	}

	connection_acl->is_numerical_address
				= (int *) malloc(num_addresses * sizeof(int));

	if ( connection_acl->is_numerical_address == NULL ) {
		fclose(connection_acl_file);

		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory allocating %ld integers for the is_numerical_address array "
"of an MX_CONNECTION_ACL structure.", num_addresses );
	}

	dimension_array[0] = num_addresses;
	dimension_array[1] = MXU_HOSTNAME_LENGTH + 1;

	data_element_size_array[0] = sizeof(char);
	data_element_size_array[1] = sizeof(char *);

	connection_acl->address_string_array = (char **)
		mx_allocate_array( MXFT_STRING,
			2, dimension_array, data_element_size_array );

	if ( connection_acl->address_string_array == NULL ) {
		fclose(connection_acl_file);

		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory allocating %ld strings for the address_string_array "
"of an MX_CONNECTION_ACL structure.", num_addresses );
	}

	/* Now go back to the beginning of the file and read the address
	 * strings into the array.
	 */

#if ! defined(OS_RTEMS)
	rewind( connection_acl_file );
#else
	/* RTEMS does not seem to handle rewind() successfully. */

	fclose( connection_acl_file );

	connection_acl_file = fopen( connection_acl_filename, "r" );

	if ( connection_acl_file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The connection ACL file '%s' cannot be opened for reading.",
			connection_acl_filename );
	}
#endif

	for ( i = 0; i < num_addresses; i++ ) {
		address_string = connection_acl->address_string_array[i];

		mx_fgets( address_string, MXU_HOSTNAME_LENGTH,
					connection_acl_file );

		if ( ferror( connection_acl_file )
		  || feof( connection_acl_file ) )
		{
			fclose( connection_acl_file );

			return mx_error( MXE_FILE_IO_ERROR, fname,
"File error occurred while reading connection ACL file '%s' on second pass.",
				connection_acl_filename );
		}

		length = strlen(address_string);

		/* Is this a numerical address or not? */

		result = strspn(address_string, NUMERICAL_ADDRESS_CHARACTERS);

		if ( result < length ) {
			connection_acl->is_numerical_address[i] = FALSE;
		} else {
			connection_acl->is_numerical_address[i] = TRUE;
		}
	}

	fclose( connection_acl_file );

	for ( i = 0; i < num_addresses; i++ ) {
		mx_info( "ACL entry: %s",
			connection_acl->address_string_array[i] );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_check_socket_connection_acl_permissions( MX_RECORD *record_list,
				const char *client_address_string,
				mx_bool_type perform_dns_lookup,
				mx_bool_type *connection_allowed )
{
	static const char fname[]
		= "mx_check_socket_connection_acl_permissions()";

	MX_LIST_HEAD *list_head;
	MX_CONNECTION_ACL *connection_acl;
	char client_hostname[MXU_HOSTNAME_LENGTH+1];
	char *address_string;
	long i;
	mx_bool_type reverse_dns_lookup_done;
	mx_status_type status;

#if MX_SECURITY_DEBUG
	MX_DEBUG(-2,("%s invoked for client '%s'",
				fname, client_address_string));
#endif

	/* Make sure this is the list_head. */

	record_list = record_list->list_head;

	list_head = (MX_LIST_HEAD *)(record_list->record_superclass_struct);

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LIST_HEAD pointer in list head record is NULL." );
	}

	connection_acl = (MX_CONNECTION_ACL *) list_head->connection_acl;

	if ( connection_acl == (MX_CONNECTION_ACL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_CONNECTION_ACL pointer in list head is NULL." );
	}

	*connection_allowed = FALSE;

	if ( perform_dns_lookup ) {
		reverse_dns_lookup_done = FALSE;
	} else {
		reverse_dns_lookup_done = TRUE;

		strlcpy( client_hostname, client_address_string,
				sizeof(client_hostname) );
	}

	for ( i = 0; i < connection_acl->num_addresses; i++ ) {

		address_string = connection_acl->address_string_array[i];

		if ( connection_acl->is_numerical_address[i] ) {

			if (mx_match(address_string, client_address_string)) {

				*connection_allowed = TRUE;
				break;		/* Exit the for() loop. */
			}
		} else {
			if ( reverse_dns_lookup_done == FALSE ) {
				status = mx_get_client_hostname(
						client_address_string,
						client_hostname,
						sizeof(client_hostname) );

				if ( status.code != MXE_SUCCESS )
					return status;

				reverse_dns_lookup_done = TRUE;
			}
			if (mx_match(address_string, client_hostname)) {

				*connection_allowed = TRUE;
				break;		/* Exit the for() loop. */
			}
		}
	}

#if MX_SECURITY_DEBUG
	MX_DEBUG(-2,("%s: *connection_allowed = %d",
				fname, (int) *connection_allowed));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_client_hostname( const char *client_address_string,
			char *client_hostname,
			size_t max_hostname_length )
{
#if MX_SECURITY_DEBUG
	static const char fname[] = "mx_get_client_hostname()";
#endif

	struct hostent *host_entry;
	unsigned long inet_address;
	mx_status_type mx_status;

	/* The following declaration is probably being overly picky, but
	 * is for the benefit of inet_addr().
	 */

	union {
		struct in_addr s;
		unsigned long ul;
	} client_in_addr;

#if MX_SECURITY_DEBUG
	MX_DEBUG(-2,("%s invoked for client_address_string = '%s'",
		fname, client_address_string ));
#endif

#if 0
	client_in_addr.ul = inet_addr( client_address_string );

#if defined(OS_VMS)
	if ( client_in_addr.ul == 0xffffffff ) {
#else
	if ( client_in_addr.ul == (unsigned long) (-1) ) {
#endif

		snprintf( client_hostname, max_hostname_length,
			"Address '%s' is not a valid address",
			client_address_string );

		return MX_SUCCESSFUL_RESULT;
	}
#else
	mx_status = mx_socket_get_inet_address( client_address_string,
						&inet_address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	client_in_addr.ul = inet_address;
#endif

#if defined(OS_VXWORKS)
	host_entry = NULL;
#else
	host_entry = gethostbyaddr( (char *) &(client_in_addr.s),
					sizeof( client_in_addr ),
					AF_INET );
#endif

	if ( host_entry == NULL ) {
		snprintf( client_hostname, max_hostname_length,
			"Address '%s' not found.", client_address_string );
	} else {
		strlcpy( client_hostname, host_entry->h_name,
					max_hostname_length );
	}

#if MX_SECURITY_DEBUG
	MX_DEBUG(-2,("%s: client_hostname = '%s'", fname, client_hostname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

