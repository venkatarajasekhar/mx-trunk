/*
 * Name:    i_epix_xclib.c
 *
 * Purpose: MX interface driver for cameras controlled through the
 *          EPIX, Inc. EPIX_XCLIB library.
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

#define MXI_EPIX_XCLIB_DEBUG			TRUE

#define MXI_EPIX_XCLIB_DEBUG_SYSTEM_TICKS	FALSE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EPIX_XCLIB

#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "i_epix_xclib.h"

#if defined(OS_WIN32)
#include <windows.h>
#endif

#include "xcliball.h"

MX_RECORD_FUNCTION_LIST mxi_epix_xclib_record_function_list = {
	NULL,
	mxi_epix_xclib_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_epix_xclib_open
};

MX_RECORD_FIELD_DEFAULTS mxi_epix_xclib_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_EPIX_XCLIB_STANDARD_FIELDS
};

long mxi_epix_xclib_num_record_fields
		= sizeof( mxi_epix_xclib_record_field_defaults )
			/ sizeof( mxi_epix_xclib_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_epix_xclib_rfield_def_ptr
			= &mxi_epix_xclib_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_epix_xclib_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_epix_xclib_create_record_structures()";

	MX_EPIX_XCLIB *epix_xclib;

	/* Allocate memory for the necessary structures. */

	epix_xclib = (MX_EPIX_XCLIB *) malloc( sizeof(MX_EPIX_XCLIB) );

	if ( epix_xclib == (MX_EPIX_XCLIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EPIX_XCLIB structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = epix_xclib;

	record->record_function_list = &mxi_epix_xclib_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	epix_xclib->record = record;

	return MX_SUCCESSFUL_RESULT;
}

#if defined(OS_LINUX)

static mx_status_type
mxi_epix_xclib_get_system_boot_time( MX_EPIX_XCLIB *epix_xclib )
{
	static const char fname[] = "mxi_epix_xclib_get_system_boot_time()";
	FILE *proc_stat;
	char buffer[100];
	int saved_errno, num_items;
	unsigned long boot_time_in_seconds;

	proc_stat = fopen( "/proc/stat", "r" );

	if ( proc_stat == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Unable to open /proc/stat.  Errno = %d, error message = '%s'.",
			saved_errno, strerror(saved_errno) );
	}

	/* Read through the output from /proc/stat until we find a line that
	 * begins with the word 'btime'. */

	fgets( buffer, sizeof(buffer), proc_stat );

	for(;;) {
		if ( feof(proc_stat) || ferror(proc_stat) ) {
			fclose(proc_stat);

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Did not find the line starting with 'btime' "
			"that was supposed to contain the boot time." );
		}

#if 0 && MXI_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
#endif

		if ( strncmp( buffer, "btime", 5 ) == 0 ) {
			break;			/* Exit the for(;;) loop. */
		}

		fgets( buffer, sizeof(buffer), proc_stat );
	}

	/* Parse the line that contains the boot time. */

	num_items = sscanf( buffer, "btime %lu", &boot_time_in_seconds );

	if ( num_items != 1 ) {
		fclose(proc_stat);

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"The system boot time could not be found in "
			"the line '%s' returned by /proc/stat.",
				buffer );
	}

	epix_xclib->system_boot_time.tv_sec = boot_time_in_seconds;
	epix_xclib->system_boot_time.tv_nsec = 0;

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: system_boot_time = (%lu,%lu)", fname,
				epix_xclib->system_boot_time.tv_sec,
				epix_xclib->system_boot_time.tv_nsec));
#endif
	fclose(proc_stat);

	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_LINUX */

MX_EXPORT mx_status_type
mxi_epix_xclib_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_epix_xclib_open()";

	char fault_message[80];
	MX_EPIX_XCLIB *epix_xclib;
	int i, length, epix_status;
	char error_message[80];
	uint original_exsync, original_prin;
	unsigned long timeout_in_milliseconds;
	double timeout_in_seconds;
	struct timespec epix_system_timespec, os_timespec;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	epix_xclib = (MX_EPIX_XCLIB *) record->record_type_struct;

	if ( epix_xclib == (MX_EPIX_XCLIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_EPIX_XCLIB pointer for record '%s' is NULL.", record->name);
	}

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

#if defined(OS_LINUX)
	{
		/* Linux specific initialization */

		int os_major, os_minor, os_update;

		mx_status = mx_get_os_version(&os_major, &os_minor, &os_update);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (os_major < 2) || (os_minor < 6) ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"The epix_xclib driver does not support versions "
			"of the Linux kernel older than Linux 2.6.  "
			"You are running Linux %d.%d.%d",
				os_major, os_minor, os_update );
		}

		mx_status = mxi_epix_xclib_get_system_boot_time( epix_xclib );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
#endif

	/* Initialize XCLIB. */

	epix_status = pxd_PIXCIopen( NULL, NULL, epix_xclib->format_file );

	if ( epix_status < 0 ) {

		pxd_mesgFaultText(-1, fault_message, sizeof(fault_message) );

		length = strlen(fault_message);

		for ( i = 0; i < length; i++ ) {
			if ( fault_message[i] == '\n' )
				fault_message[i] = ' ';
		}

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Loading PIXCI configuration '%s' failed for record '%s' "
		"with error code %d (%s).  Fault description = '%s'.", 
			epix_xclib->format_file, record->name,
			epix_status, pxd_mesgErrorCode( epix_status ),
			fault_message );
	}

#if MXI_EPIX_XCLIB_DEBUG
	/* Display some statistics. */

	MX_DEBUG(-2,("%s: Library Id = '%s'", fname, pxd_infoLibraryId() ));

	MX_DEBUG(-2,("%s: Include Id = '%s'", fname, pxd_infoIncludeId() ));

	MX_DEBUG(-2,("%s: Driver Id  = '%s'", fname, pxd_infoDriverId() ));

	MX_DEBUG(-2,("%s: Image frame buffer memory size = %lu bytes",
					fname, pxd_infoMemsize(-1) ));

	MX_DEBUG(-2,("%s: Number of boards = %d", fname, pxd_infoUnits() ));

	MX_DEBUG(-2,("%s: Number of frame buffers per board= %d",
					fname, pxd_imageZdim() ));

	MX_DEBUG(-2,("%s: X dimension = %d, Y dimension = %d",
				fname, pxd_imageXdim(), pxd_imageYdim() ));

	MX_DEBUG(-2,("%s: %d bits per pixel component",
					fname, pxd_imageBdim() ));

	MX_DEBUG(-2,("%s: %d components per pixel", fname, pxd_imageCdim() ));

	MX_DEBUG(-2,("%s: %d fields per frame buffer", fname, pxd_imageIdim()));
#endif

#if MXI_EPIX_XCLIB_DEBUG_SYSTEM_TICKS

	MX_DEBUG(-2,("%s: Getting the zero for EPIX system time.", fname));

	/* Verify that the computed EPIX system time matches the operating
	 * system time by taking a frame and then immediately comparing
	 * the operating system time to the EPIX system time.
	 */

	/* Take a frame.  Time out if the frame takes longer than 1 second. */

	timeout_in_milliseconds = 1000;

	epix_status = pxd_doSnap(1,1,timeout_in_milliseconds);

	/* Get the current wall clock time. */

	os_timespec = mx_current_os_time();

	/* Get the EPIX system time for the buffer we just acquired. */

	epix_system_timespec =
		mxi_epix_xclib_get_buffer_timespec( epix_xclib, 1, 1 );

	MX_DEBUG(-2,("****** Start of statistics dump ******"));

	MX_DEBUG(-2,("%s: epix_system_timespec = (%lu,%ld)", fname,
				epix_system_timespec.tv_sec,
				epix_system_timespec.tv_nsec));

	MX_DEBUG(-2,("%s: os_timespec = (%lu,%ld)", fname,
					os_timespec.tv_sec,
					os_timespec.tv_nsec));

	MX_DEBUG(-2,("*** /proc/uptime ***"));
	system("cat /proc/uptime");

	MX_DEBUG(-2,("*** /proc/interrupts ***"));
	system("cat /proc/interrupts");

	MX_DEBUG(-2,("*** /proc/stat ***"));
	system("cat /proc/stat");

	MX_DEBUG(-2,("****** End of statistics dump ******"));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------*/

MX_EXPORT char *
mxi_epix_xclib_error_message( int unitmap,
				int epix_error_code,
				char *buffer,
				size_t buffer_length )
{
	char fault_message[500];
	int i, string_length, fault_string_length;

	MX_DEBUG(-2,("debug: unitmap = %d, epix_error_code = %d",
			unitmap, epix_error_code ));

	if ( buffer == NULL )
		return NULL;

	snprintf( buffer, buffer_length, "PIXCI error code = %d (%s).  ",
			epix_error_code, pxd_mesgErrorCode( epix_error_code ) );

	string_length = strlen( buffer );

	MX_DEBUG(-2,("debug: string_length = %d, buffer = %p, buffer = '%s'",
			string_length, buffer, buffer));

	return buffer;

	pxd_mesgFaultText( unitmap, fault_message, sizeof(fault_message) );

	MX_DEBUG(-2,("debug: fault_message #1 = '%s'", fault_message));

	fault_string_length = strlen( fault_message );

	MX_DEBUG(-2,("debug: fault_string_length #1 = %d",
			fault_string_length));

	if ( fault_message[fault_string_length - 1] == '\n' ) {
		fault_message[fault_string_length - 1] = '\0';

		fault_string_length--;
	}

	MX_DEBUG(-2,("debug: fault_message #2 = '%s'", fault_message));

	MX_DEBUG(-2,("debug: fault_string_length #2 = %d",
			fault_string_length));

	for ( i = 0; i < fault_string_length; i++ ) {
		if ( fault_message[i] == '\n' )
			fault_message[i] = ' ';
	}

	MX_DEBUG(-2,("debug: fault_message #3 = '%s'", fault_message));

	MX_DEBUG(-2,("debug: buffer = '%s'", buffer));

	return buffer;
}

/*------------------------------------------------------------*/

#if defined(OS_LINUX)

/* FIXME: INITIAL_JIFFIES is copied from the Linux kernel include file
 *        "linux/jiffies.h".  It is not advisable to include kernel 
 *        header files directly, so we copy the definition.
 */

#define INITIAL_JIFFIES ((unsigned long)(unsigned int) (-300000))

#endif /* OS_LINUX */

MX_EXPORT struct timespec
mxi_epix_xclib_get_buffer_timespec( MX_EPIX_XCLIB *epix_xclib,
					long unitmap,
					long buffer_number )
{
	static const char fname[] = "mxi_epix_xclib_get_buffer_timespec()";

	struct timespec result, timespec_since_boot;
	uint32 epix_buffer_sys_ticks;

	result.tv_sec = 0;
	result.tv_nsec = 0;

	if ( epix_xclib == (MX_EPIX_XCLIB *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPIX_XCLIB pointer passed was NULL." );

		return result;
	}

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: unitmap = %ld, buffer_number = %ld",
		fname, unitmap, buffer_number ));
#endif

	epix_buffer_sys_ticks = pxd_buffersSysTicks( unitmap, buffer_number );

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: epix_buffer_sys_ticks = %lu",
		fname, (unsigned long) epix_buffer_sys_ticks));
#endif

	/********************** Win32 *************************/

#if defined( OS_WIN32 )
#error Win32 support not yet written.

	/********************** Linux *************************/

#elif defined( OS_LINUX )

	if ( epix_xclib->use_high_resolution_timing ) {

		mx_warning(
		"High resolution timing not yet implemented for Linux.");
	} else {
		/* Here we use Linux kernel 'jiffies'. */

		uint32 jiffies_since_boot;

		/* Subtract the value of INITIAL_JIFFIES, so that system boot
		 * time becomes the zero time.
		 */

		jiffies_since_boot = epix_buffer_sys_ticks - INITIAL_JIFFIES;

		timespec_since_boot.tv_sec = jiffies_since_boot / 1000L;
		timespec_since_boot.tv_nsec =
			(jiffies_since_boot % 1000L) * 1000000L;

#if MXI_EPIX_XCLIB_DEBUG
		MX_DEBUG(-2,
	    ("%s: jiffies_since_boot = %lu, timespec_since_boot = (%lu,%ld)",
			fname, (unsigned long) jiffies_since_boot,
			timespec_since_boot.tv_sec,
			timespec_since_boot.tv_nsec));

		MX_DEBUG(-2,("%s: system_boot_time = (%lu,%ld)", fname,
			(unsigned long) epix_xclib->system_boot_time.tv_sec,
			epix_xclib->system_boot_time.tv_nsec));
#endif
		result = mx_add_high_resolution_times( timespec_since_boot,
						epix_xclib->system_boot_time );
	}
#else
#error This platform is not supported for EPIX XCLIB.
#endif

#if MXI_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: result = (%lu,%ld)", fname,
		result.tv_sec, result.tv_nsec));
#endif

	return result;
}

#endif /* HAVE_EPIX_XCLIB */

