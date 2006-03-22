/*
 * Name:    mx_cpu_architecture.c
 *
 * Purpose: Report the CPU architecture.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <errno.h>

#include "mx_osdef.h"

#include "mx_util.h"

#if defined( OS_UNIX )

#include <sys/utsname.h>

MX_EXPORT mx_status_type
mx_get_cpu_architecture( char *architecture_type,
			size_t max_architecture_type_length,
			char *architecture_subtype,
			size_t max_architecture_subtype_length )
{
	static const char fname[] = "mx_get_cpu_architecture()";

	struct utsname uname_struct;
	int status, saved_errno;

	status = uname( &uname_struct );

	if ( status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"uname() failed.  Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	if ( architecture_type != NULL ) {
		strlcpy( architecture_type, uname_struct.machine, 
				max_architecture_type_length );
	}

	if ( architecture_subtype != NULL ) {
		strlcpy( architecture_subtype, "",
				max_architecture_subtype_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

#else

#error Reporting the operating system version has not been implemented for this platform.

#endif

