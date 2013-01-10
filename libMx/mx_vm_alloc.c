/*
 * Name:    mx_vm_alloc.c
 *
 * Purpose: MX functions that allocate virtual memory directly from the
 *          underlying memory manager.  Read, write, and execute protection
 *          can be set for this memory.  Specifying a requested address of
 *          NULL lets the operating system select the actual address.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_VM_ALLOC_DEBUG	TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_vm_alloc.h"

/*=========================== Microsoft Windows ===========================*/

#if defined(OS_WIN32)

#include <windows.h>

MX_EXPORT void *
mx_vm_alloc( void *requested_address,
		size_t requested_region_size_in_bytes,
		unsigned long protection_flags )
{
	static const char fname[] = "mx_vm_alloc()";

	DWORD vm_allocation_type;
	DWORD vm_protection_flags;
	void *actual_address;

	DWORD last_error_code;
	TCHAR message_buffer[100];

	vm_allocation_type = MEM_COMMIT | MEM_RESERVE;

	/* Write permission implies read permission on Win32. */

	vm_protection_flags = 0;

	if ( protection_flags & X_OK ) {
		if ( protection_flags & W_OK ) {
			vm_protection_flags = PAGE_EXECUTE_READWRITE;
		} else
		if ( protection_flags & R_OK ) {
			vm_protection_flags = PAGE_EXECUTE_READ;
		} else {
			vm_protection_flags = PAGE_EXECUTE;
		}
	} else
	if ( protection_flags & W_OK ) {
		vm_protection_flags = PAGE_READWRITE;
	} else
	if ( protection_flags & R_OK ) {
		vm_protection_flags = PAGE_READONLY;
	} else {
		vm_protection_flags = PAGE_NOACCESS;
	}
	
	actual_address = VirtualAlloc( requested_address,
					requested_region_size_in_bytes,
					vm_allocation_type,
					vm_protection_flags );

	if ( actual_address == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"VirtualAlloc() failed with "
		"Win32 error code = %ld, error message = '%s'",
			last_error_code, message_buffer );
	}

	return actual_address;
}

MX_EXPORT void
mx_vm_free( void *address )
{
	static const char fname[] = "mx_vm_free()";

	BOOL virtual_free_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	virtual_free_status = VirtualFree( address, 0, MEM_RELEASE );

	if ( virtual_free_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"VirtualFree() failed with "
		"Win32 error code = %ld, error message = '%s'",
			last_error_code, message_buffer );
	}

	return;
}

MX_EXPORT mx_status_type
mx_vm_get_protection( void *address,
		size_t region_size_in_bytes,
		mx_bool_type *valid_address_range,
		unsigned long *protection_flags )
{
	static const char fname[] = "mx_vm_get_protection()";

	MEMORY_BASIC_INFORMATION memory_info;
	SIZE_T bytes_returned;
	DWORD vm_protection_flags;

	DWORD last_error_code;
	TCHAR message_buffer[100];

	unsigned long protection_flags_value;

	if ( address == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The address pointer passed is NULL." );
	}
	if ( protection_flags == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The protection_flags pointer passed was NULL." );
	}

	bytes_returned = VirtualQuery( address,
					&memory_info,
					sizeof(memory_info) );

	if ( bytes_returned == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"VirtualQuery() failed with "
		"Win32 error code = %ld, error message = '%s'",
			last_error_code, message_buffer );
	}

	vm_protection_flags = memory_info.Protect;

	switch( vm_protection_flags ) {
	case PAGE_EXECUTE_READWRITE:
		protection_flags_value = ( R_OK | W_OK | X_OK );
		break;
	case PAGE_EXECUTE_READ:
		protection_flags_value = ( R_OK | X_OK );
		break;
	case PAGE_EXECUTE:
		protection_flags_value = X_OK;
		break;
	case PAGE_READWRITE:
		protection_flags_value = ( R_OK | W_OK );
		break;
	case PAGE_READONLY:
		protection_flags_value = R_OK;
		break;
	case PAGE_NOACCESS:
	case 0:
		protection_flags_value = 0;
		break;
	default:
		protection_flags_value = 0;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The access protection value %#lx returned by "
		"VirtualQuery for address %p is not recognized.",
			(unsigned long) vm_protection_flags,
			address );
		break;
	}

	if ( valid_address_range != NULL ) {
		*valid_address_range = TRUE;
	}

	if ( protection_flags != NULL ) {
		*protection_flags = protection_flags_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_vm_set_protection( void *address,
		size_t region_size_in_bytes,
		unsigned long protection_flags )
{
	static const char fname[] = "mx_vm_set_protection()";

	BOOL virtual_protect_status;
	DWORD vm_protection_flags, old_vm_protection_flags;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( address == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The address pointer passed is NULL." );
	}

	/* Write permission implies read permission on Win32. */

	vm_protection_flags = 0;

	if ( protection_flags & X_OK ) {
		if ( protection_flags & W_OK ) {
			vm_protection_flags = PAGE_EXECUTE_READWRITE;
		} else
		if ( protection_flags & R_OK ) {
			vm_protection_flags = PAGE_EXECUTE_READ;
		} else {
			vm_protection_flags = PAGE_EXECUTE;
		}
	} else
	if ( protection_flags & W_OK ) {
		vm_protection_flags = PAGE_READWRITE;
	} else
	if ( protection_flags & R_OK ) {
		vm_protection_flags = PAGE_READONLY;
	} else {
		vm_protection_flags = PAGE_NOACCESS;
	}

	virtual_protect_status = VirtualProtect( address,
						region_size_in_bytes,
						vm_protection_flags,
						&old_vm_protection_flags );
	
	if ( virtual_protect_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"VirtualProtect() failed with "
		"Win32 error code = %ld, error message = '%s'",
			last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*================================= Posix =================================*/

/*
 * Posix platforms provide a standard way of implementing these functions:
 *
 *   mx_vm_alloc() -------------> mmap()
 *   mx_vm_free() --------------> munmap()
 *   mx_vm_set_protection() ----> mprotect()
 *
 * However, there is no standard way of implementing mx_vm_get_protection().
 * Thus, mx_vm_get_protection() is implemented in platform-specific code
 * later in this section of the file.
 */

#elif defined(OS_LINUX)

#include <errno.h>
#include <sys/mman.h>

MX_EXPORT void *
mx_vm_alloc( void *requested_address,
		size_t requested_region_size_in_bytes,
		unsigned long protection_flags )
{
	static const char fname[] = "mx_vm_alloc()";

	void *actual_address;
	int vm_protection_flags, vm_visibility_flags;
	int saved_errno;

	vm_protection_flags = 0;

	if ( protection_flags & X_OK ) {
		vm_protection_flags |= PROT_EXEC;
	}
	if ( protection_flags & R_OK ) {
		vm_protection_flags |= PROT_READ;
	}
	if ( protection_flags & W_OK ) {
		vm_protection_flags |= PROT_WRITE;
	}

	if ( vm_protection_flags == 0 ) {
		vm_protection_flags = PROT_NONE;
	}

	vm_visibility_flags = MAP_PRIVATE | MAP_ANONYMOUS;

	actual_address = mmap( requested_address,
				requested_region_size_in_bytes,
				vm_protection_flags,
				vm_visibility_flags,
				-1, 0 );

	if ( actual_address == MAP_FAILED ) {
		saved_errno = errno;

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"mmap() failed with errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );

		return NULL;
	}

	return actual_address;
}

MX_EXPORT void
mx_vm_free( void *address )
{
	static const char fname[] = "mx_vm_free()";

	int munmap_status, saved_errno;

	if ( address == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"The address pointer passed is NULL." );

		return;
	}

	munmap_status = munmap( address, 0 );

	if ( munmap_status == (-1) ) {
		saved_errno = errno;

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"munmap() failed with errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );

		return;
	}

	return;
}

MX_EXPORT mx_status_type
mx_vm_set_protection( void *address,
		size_t region_size_in_bytes,
		unsigned long protection_flags )
{
	static const char fname[] = "mx_vm_set_protection()";

	int vm_protection_flags;
	int mprotect_status, saved_errno;

	if ( address == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The address pointer passed is NULL." );
	}

	vm_protection_flags = 0;

	if ( protection_flags & X_OK ) {
		vm_protection_flags |= PROT_EXEC;
	}
	if ( protection_flags & R_OK ) {
		vm_protection_flags |= PROT_READ;
	}
	if ( protection_flags & W_OK ) {
		vm_protection_flags |= PROT_WRITE;
	}

	if ( vm_protection_flags == 0 ) {
		vm_protection_flags = PROT_NONE;
	}

	mprotect_status = mprotect( address,
				region_size_in_bytes,
				vm_protection_flags );

	if ( mprotect_status == (-1) ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"mprotect() failed with errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----- Platform-specific mx_vm_get_protection() for Posix platforms ------*/

#  if defined(OS_LINUX)

#  include <sys/mman.h>

MX_EXPORT mx_status_type
mx_vm_get_protection( void *address,
		size_t region_size_in_bytes,
		mx_bool_type *valid_address_range,
		unsigned long *protection_flags )
{
	static const char fname[] = "mx_vm_get_protection()";

	FILE *file;
	char buffer[300];
	int i, argc, saved_errno;
	char **argv;
	unsigned long start_address, end_address, pointer_address;
	char permissions[20];

	mx_bool_type valid;
	unsigned long protection_flags_value;

	if ( address == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The address pointer passed is NULL." );
	}

#if MX_VM_ALLOC_DEBUG
	MX_DEBUG(-2,("%s invoked for address %p, size %lu",
		fname, address, (unsigned long) region_size_in_bytes ));
#endif

	/* Under Linux, the preferred technique is to read the
	 * necessary information from /proc/self/maps.
	 */

	file = fopen( "/proc/self/maps", "r" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to open '/proc/self/maps' failed with "
		"errno = %d, error message = '%s'.",
			saved_errno, strerror( saved_errno ) );
	}

	/* If we get here, then opening /proc/self/maps succeeded. */

	protection_flags_value = 0;

	pointer_address = (unsigned long) address;

	for ( i = 0; ; i++ ) {

		valid = FALSE;

		mx_fgets( buffer, sizeof(buffer), file );

		if ( feof(file) ) {
			valid = FALSE;
			break;		/* Exit the for() loop. */
		} else
		if ( ferror(file) ) {
			(void) mx_error( MXE_FILE_IO_ERROR, fname,
			"The attempt to read line %d of the output "
			"from /proc/self/maps failed.", i+1 );

			valid = FALSE;
			break;		/* Exit the for() loop. */
		}

#if MX_VM_ALLOC_DEBUG
		MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
#endif

		/* Split up the string using both space characters
		 * and '-' characters as delimiters.
		 */

		mx_string_split( buffer, " -", &argc, &argv );

		start_address = mx_hex_string_to_unsigned_long( argv[0] );
		end_address   = mx_hex_string_to_unsigned_long( argv[1] );

		strlcpy( permissions, argv[2], sizeof(permissions) );

		mx_free(argv);

#if MX_VM_ALLOC_DEBUG
		MX_DEBUG(-2,("%s: address = %p, pointer_address = %#lx, "
			"i = %d, start_address = %#lx, "
			"end_address = %#lx, permissions = '%s'", fname,
			address, pointer_address, i,
			start_address, end_address, permissions));
#endif
		/* If the pointer is located before the start of the
		 * first memory block, then it is definitely bad.
		 */

		if ( (i == 0) && (pointer_address <= start_address) ) {
			valid = FALSE;

			break;		/* Exit the for() loop. */
		}

		/* See if the pointer is in the current memory block. */

		if ( (start_address <= pointer_address)
			&& (pointer_address <= end_address ) )
		{
#if MX_VM_ALLOC_DEBUG
			MX_DEBUG(-2,("%s: pointer is in this block.",fname));
#endif
			valid = TRUE;

			if ( permissions[0] == 'r' ) {
				protection_flags_value |= R_OK;
			}
			if ( permissions[1] == 'w' ) {
				protection_flags_value |= W_OK;
			}
			if ( permissions[2] == 'x' ) {
				protection_flags_value |= X_OK;
			}

			break;		/* Exit the for() loop. */
		}
	}

#if MX_VM_ALLOC_DEBUG
	MX_DEBUG(-2,("%s: valid = %d", fname, valid));
#endif

	fclose(file);

	if ( valid_address_range != NULL ) {
		*valid_address_range = valid;
	}

	if ( protection_flags != NULL ) {
		*protection_flags = protection_flags_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/
#  else
#  error mx_vm_get_protection() is not yet implemented for this Posix platform.
#  endif

/*=========================================================================*/

#else
#error Virtual memory functions are not yet implemented for this platform.
#endif

