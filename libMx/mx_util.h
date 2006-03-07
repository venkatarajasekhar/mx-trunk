/*
 * Name:    mx_util.h
 *
 * Purpose: Define utility functions and generally used symbols.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 1999-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_UTIL_H__
#define __MX_UTIL_H__

#include <string.h>	/* We get 'size_t' from here. */

#include <time.h>	/* We get 'struct timespec' from here. */

#include <stdarg.h>	/* We get 'va_list' from here. */

/*
 * Macros for declaring shared library or DLL functions.
 *
 * The MX_API and MX_EXPORT macros are used for the declaration of functions
 * in shared libraries or DLLs.  They must be used by publically visible
 * functions in a shared library or DLL and must _not_ be used by functions
 * that are not in a shared library or DLL.
 *
 * MX_API and MX_EXPORT are similar but not the same.  MX_API is used only
 * in header files (.h files), while MX_EXPORT is used only in .c files
 * where the body of the function appears.  On Win32, these are identical,
 * but this will not be true, in general, on any other platform.
 */

#if defined(OS_WIN32) && defined(__GNUC__)
    /* For MinGW to get a definition of _declspec(). */
#   include <windef.h>
#endif

#if defined(OS_WIN32)
#   ifdef __MX_LIBRARY__
#	define MX_API		_declspec(dllexport)
#	define MX_EXPORT	_declspec(dllexport)
#   else
#	define MX_API		_declspec(dllimport)
#	define MX_EXPORT	__ERROR_ONLY_USE_THIS_IN_LIBRARIES__
#   endif
#elif defined(OS_VMS)
#       define MX_API		extern
#       define MX_EXPORT
#else
#   ifdef __MX_LIBRARY__
#	define MX_API		extern
#	define MX_EXPORT
#   else
#	define MX_API		extern
#	define MX_EXPORT	__ERROR_ONLY_USE_THIS_IN_LIBRARIES__
#   endif
#endif

/*
 * The MX_API_PRIVATE macro is used to indicate functions that may be exported
 * by the MX library, but which are not intended to be used by general
 * application programs.  At present, it is just an alias for MX_API, but
 * this may change later.
 */

#define MX_API_PRIVATE		MX_API

/*------------------------------------------------------------------------*/

/*
 * Solaris 2 defines SIG_DFL, SIG_ERR, SIG_IGN, and SIG_HOLD in a way
 * such that GCC on Solaris 2 generates a warning about a function
 * definition being an invalid prototype.
 */

#if defined( OS_SOLARIS ) && defined( __GNUC__ )

# ifdef SIG_DFL
#   undef SIG_DFL
# endif

# ifdef SIG_ERR
#   undef SIG_ERR
# endif

# ifdef SIG_IGN
#   undef SIG_IGN
# endif

# ifdef SIG_HOLD
#   undef SIG_HOLD
# endif

#define SIG_DFL  (void(*)(int))0
#define SIG_ERR  (void(*)(int))-1
#define SIG_IGN  (void(*)(int))1
#define SIG_HOLD (void(*)(int))2

#endif /* OS_SOLARIS && __GNUC__ */

/*------------------------------------------------------------------------*/

#if defined( OS_DJGPP )
   /* This has to appear before we include <sys/param.h> below, which
    * includes <sys/swap.h>, which requires these prototypes to exist.
    * They are also used by the mx_socket.h header file.
    */
   extern __inline__ unsigned long  __ntohl( unsigned long );
   extern __inline__ unsigned short __ntohs( unsigned short );
#endif

#if defined( OS_WIN32 )
#  include <stdlib.h>
#  define MXU_FILENAME_LENGTH		_MAX_PATH

#elif defined( OS_VXWORKS )
#  include <limits.h>
#  define MXU_FILENAME_LENGTH		PATH_MAX

#elif defined( OS_VMS )
#  define MXU_FILENAME_LENGTH		255	/* According to comp.os.vms */

#else
#  include <sys/param.h>
#  if defined( MAXPATHLEN )
#     define MXU_FILENAME_LENGTH	MAXPATHLEN
#  else
#     error Maximum path length not yet defined for this platform.
#  endif
#endif

/*------------------------------------------------------------------------*/

#if defined( OS_WIN32 ) && defined(__BORLANDC__)
   /* I have problems with lockups when using the malloc(), etc. functions
    * provided by Borland C++ 5.5, so I define replacements for them.
    */

   MX_API void *bc_calloc( size_t, size_t );
   MX_API void  bc_free( void * );
   MX_API void *bc_malloc( size_t );
   MX_API void *bc_realloc( void *, size_t );

#  define calloc(x,y)  bc_calloc((x),(y))
#  define free(x)      bc_free(x)
#  define malloc(x)    bc_malloc(x)
#  define realloc(x,y) bc_realloc((x),(y))

#endif

/*------------------------------------------------------------------------*/

/* The following definitions allow for typechecking of printf and scanf
 * style function arguments with GCC.
 */

#if defined( __GNUC__ )

#define MX_PRINTFLIKE( a, b ) __attribute__ ((format (printf, a, b)))
#define MX_SCANFLIKE( a, b )  __attribute__ ((format (scanf, a, b)))

#else

#define MX_PRINTFLIKE( a, b )
#define MX_SCANFLIKE( a, b )

#endif

/*------------------------------------------------------------------------*/

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#define MX_WHITESPACE		" \t"

#define MXU_STRING_LENGTH       20
#define MXU_BUFFER_LENGTH	400
#define MXU_HOSTNAME_LENGTH	100

/*------------------------------------------------------------------------*/

/* Sleep functions with higher resolution than sleep(). */

MX_API void mx_sleep( unsigned long seconds );
MX_API void mx_msleep( unsigned long milliseconds );
MX_API void mx_usleep( unsigned long microseconds );

/* mx_standard_signal_error_handler() is used by several MX programs to provide
 * a standard response to a program crash.
 */

MX_API void mx_standard_signal_error_handler( int signal_number );

/*--- Stack debugging tools ---*/

/* mx_stack_traceback() does its best to provide a traceback of the
 * function call stack at the time of invocation.
 */

MX_API void mx_stack_traceback( void );

/* mx_stack_check() tries to see if the stack is in a valid state.
 * It returns TRUE if the stack is valid and attempts to return FALSE
 * if the stack is not valid.  However, in cases of bad stack frame
 * corruption, it may not be possible to successfully invoke and
 * return from this function, so keep one's expectations low.
 */

MX_API int mx_stack_check( void );

/*--- Heap debugging tools ---*/

/* mx_is_valid_heap_pointer() checks to see if the supplied pointer
 * was allocated from the heap.
 */

MX_API int mx_is_valid_heap_pointer( void *pointer );

/* mx_heap_check() looks to see if the heap is in a valid state.
 * It returns TRUE if the heap is valid and FALSE if the heap is
 * corrupted.  Your program is likely to crash soon if the heap 
 * is corrupted though.
 */

MX_API int mx_heap_check( void );

/*--- Other debugging tools ---*/

/* mx_is_valid_pointer() __attempts__ to see if the supplied pointer
 * is a valid pointer.  This is not guaranteed to work on all platforms
 * by any means.
 */

MX_API int mx_is_valid_pointer( void *pointer );

/*
 * mx_force_core_dump() attempts to force the creation of a snapshot
 * of the state of the application that can be examined with a debugger
 * and then exit.  On Unix, this is done with a core dump.  On other
 * platforms, an appropriate platform-specific action should occur.
 */

MX_API void mx_force_core_dump( void );

/*
 * mx_start_debugger() attempts to start an external debugger such as the
 * Visual C++ debugger, DDD, gdb, etc. with the program stopped at the
 * line where mx_start_debugger() was invoked.
 *
 * The 'command' argument to mx_start_debugger() is a platform-dependent
 * way of changing the debugger to be started, modifying the way the debugger
 * is run, or anything else appropriate for a given platform.  If the 'command'
 * argument is set to NULL, then a default debugging environment will be
 * started.
 *
 * Warning: This feature is not implemented on all platforms.
 */

MX_API void mx_start_debugger( char *command );

/*
 * mx_hex_char_to_unsigned_long() converts a hexadecimal character to an
 * unsigned long integer.  mx_hex_string_to_unsigned_long() does the same
 * thing for a string.
 */

MX_API unsigned long mx_hex_char_to_unsigned_long( char c );

MX_API unsigned long mx_hex_string_to_unsigned_long( char *string );

/* mx_string_to_long() and mx_string_to_unsigned_long() can handle decimal,
 * octal, and hexadecimal representations of numbers.  If the string starts
 * with '0x' the number is taken to be hexadecimal.  If not, then if it
 * starts with '0', it is taken to be octal.  Otherwise, it is taken to be
 * decimal.  The functions are just wrappers for strtol() and strtoul(), so
 * read strtol's and strtoul's man pages for the real story.
 */

MX_API long mx_string_to_long( char *string );

MX_API unsigned long mx_string_to_unsigned_long( char *string );

/* mx_round() rounds to the nearest integer, while mx_round_away_from_zero()
 * and mx_round_toward_zero() round to the integer in the specified direction.
 *
 * The mx_round_away_from_zero() function subtracts threshold from the
 * number before rounding it up, while mx_round_toward_zero() adds it.
 * This is to prevent numbers that are only non-integral due to roundoff
 * error from being incorrectly rounded to the wrong integer.
 *
 * The ANSI C functions floor() and ceil() cover the other two interesting
 * cases.
 */

MX_API long mx_round( double value );
MX_API long mx_round_away_from_zero( double value, double threshold );
MX_API long mx_round_toward_zero( double value, double threshold );

/* mx_multiply_safely multiplies two floating point numbers by each other
 * while testing for and avoiding infinities.
 */

MX_API double mx_multiply_safely( double multiplier1, double multiplier2 );

/* mx_divide_safely divides two floating point numbers by each other
 * while testing for and avoiding division by zero.
 */

MX_API double mx_divide_safely( double numerator, double denominator );

/* mx_difference() computes a relative difference function. */

MX_API double mx_difference( double value1, double value2 );

/* mx_match() does simple wildcard matching. */

MX_API int mx_match( char *pattern, char *string );

/* mx_parse_command_line() takes a string and creates argv[] and envp[]
 * style arrays from it.  Spaces and tabs are taken to be whitespace, while
 * text enclosed by double quotes are assumed to be a single token.  Tokens
 * with an embedded '=' character are assumed to be environment variables.
 * The argv and envp arrays are terminated by NULL entries.
 */

MX_API int mx_parse_command_line( char *command_line,
		int *argc, char ***argv, int *envc, char ***envp );

/* mx_free_pointer() is a wrapper for free() that attempts to verify
 * that the pointer is valid to be freed before trying to free() it.
 * The function returns TRUE if freeing the memory succeeded and 
 * FALSE if not.
 */

MX_API int mx_free_pointer( void *pointer );

/* mx_free() is a macro that calls mx_free_pointer() and then sets
 * the pointer to NULL if mx_free_pointer() returned TRUE.
 */

#define mx_free( ptr ) \
			do { \
				if ( mx_free_pointer( ptr ) ) { \
					(ptr) = NULL; \
				} \
			} while (0)

#if ( defined(OS_WIN32) && defined(_MSC_VER) ) || defined(OS_VXWORKS) \
	|| defined(OS_DJGPP) || (defined(OS_VMS) && (__VMS_VER < 70320000 ))

/* These provide definitions of snprintf() and vsnprintf() for systems
 * that do not come with them.  On most such systems, snprintf() and
 * vsnprintf() are merely redefined as sprintf() and vsprintf().
 * Obviously, this removes the buffer overrun safety on such platforms.
 * However, snprintf() and vsnprintf() are supported on most systems
 * and hopefully the buffer overruns will be detected on systems
 * that that support them.
 */

MX_API int snprintf( char *dest, size_t maxlen, const char *format, ... );

MX_API int vsnprintf( char *dest, size_t maxlen, const char *format,
							va_list args );

#endif

#if defined(OS_LINUX) || defined(OS_WIN32) || defined(OS_IRIX) \
	|| defined(OS_HPUX) || defined(OS_TRU64) || defined(OS_VMS) \
	|| defined(OS_QNX) || defined(OS_VXWORKS) || defined(OS_RTEMS) \
	|| defined(OS_DJGPP)

/* These provide definitions of strlcpy() and strlcat() for systems that
 * do not come with them.  For systems that do not come with them, the
 * OpenBSD source code for strlcpy() and strlcat() is bundled with the
 * base MX distribution in the directory mx/tools/generic/src.
 */

MX_API size_t strlcpy( char *dest, const char *src, size_t maxlen );

MX_API size_t strlcat( char *dest, const char *src, size_t maxlen );

#endif

/* == Debugging functions. == */

/* Note that in any call to MX_DEBUG(), _all_ the arguments together 
 * after the debug level are enclosed in _one_ extra set of parentheses.
 * If you don't do this, the code will not compile.
 *
 * For example,
 *    MX_DEBUG( 2, ("The current value of foo is %d\n", foo) );
 * 
 * This is so that the preprocessor will treat everything after "2,"
 * as one _big_ macro argument.  This is the closest one can come to
 * "varargs" macros in standard ANSI C.  Also, the line below that
 * reads "mx_debug_function  text ;" is _not_ in error and is _not_
 * missing any needed parentheses.  The preprocessor will get the
 * needed parentheses from the extra set of parentheses in the
 * original invocation of MX_DEBUG().  Go read up on just how macro
 * expansion by the C preprocessor is supposed to work if you want
 * to understand this better.  Also, the Frequently Asked Questions
 * file for comp.lang.c on Usenet has a few words about this.
 *
 * Yes, it's a trick, but it's a trick that works very well as long
 * you don't forget those extra set of parentheses.  Incidentally,
 * you still need the parentheses even if the format field in the
 * second argument is a constant string with no %'s in it.  That's
 * because if you don't you'll end up after the macro expansion
 * with something like:   mx_debug_function "This doesn't work" ;
 */

MX_API void mx_debug_function( char *format, ... ) MX_PRINTFLIKE( 1, 2 );

#ifndef DEBUG
#define MX_DEBUG( level, text )
#else
#define MX_DEBUG( level, text ) \
		if ( (level) <= mx_get_debug_level() )  { \
			mx_debug_function  text ; \
		}
#endif

MX_API void mx_set_debug_level( int debug_level );
MX_API int  mx_get_debug_level( void );

MX_API void mx_set_debug_output_function( void (*)( char * ) );
MX_API void mx_debug_default_output_function( char *string );

MX_API void mx_debug_pause( char *format, ... ) MX_PRINTFLIKE( 1, 2 );

/* === User interrupts. === */

#define MXF_USER_INT_NONE	0
#define MXF_USER_INT_ABORT	1
#define MXF_USER_INT_PAUSE	2

#define MXF_USER_INT_ERROR	(-1)

MX_API int  mx_user_requested_interrupt( void );
MX_API void mx_set_user_interrupt_function( int (*)( void ) );
MX_API int  mx_default_user_interrupt_function( void );

/* === Informational messages. === */

MX_API void mx_info( char *format, ... ) MX_PRINTFLIKE( 1, 2 );

MX_API void mx_info_dialog( char *text_prompt,
					char *gui_prompt,
					char *button_label );

MX_API void mx_info_entry_dialog( char *text_prompt,
					char *gui_prompt,
					int echo_characters,
					char *response,
					size_t max_response_length );

MX_API void mx_set_info_output_function( void (*)( char * ) );
MX_API void mx_info_default_output_function( char *string );

MX_API void mx_set_info_dialog_function( void (*)( char *, char *, char * ) );
MX_API void mx_info_default_dialog_function( char *, char *, char * );

MX_API void mx_set_info_entry_dialog_function(
			void (*)( char *, char *, int, char *, size_t ) );
MX_API void mx_info_default_entry_dialog_function(
					char *, char *, int, char *, size_t );


/* === Warning messages. === */

MX_API void mx_warning( char *format, ... ) MX_PRINTFLIKE( 1, 2 );

MX_API void mx_set_warning_output_function( void (*)( char * ) );
MX_API void mx_warning_default_output_function( char *string );

/* === Error messages. === */

#define MXU_ERROR_MESSAGE_LENGTH	200

#if USE_STACK_BASED_MX_ERROR

typedef struct {
	long code;		/* The error code. */
	const char *location;	/* Function name where the error occurred. */
	char message[MXU_ERROR_MESSAGE_LENGTH+1]; /* The specific error msg.*/
} mx_status_type;

#else /* not USE_STACK_BASED_MX_ERROR */

typedef struct {
	long code;		/* The error code. */
	const char *location;	/* Function name where the error occurred. */
	char *message;                            /* The specific error msg.*/
} mx_status_type;

#endif /* USE_STACK_BASED_MX_ERROR */

MX_API mx_status_type mx_error( long error_code,
				const char *location,
				char *format, ... ) MX_PRINTFLIKE( 3, 4 );

MX_API mx_status_type mx_error_quiet( long error_code,
				const char *location,
				char *format, ... ) MX_PRINTFLIKE( 3, 4 );

#define MX_CHECK_FOR_ERROR( function )				\
	do { 							\
		mx_status_type mx_private_status;		\
								\
		mx_private_status = (function);			\
								\
		if ( mx_private_status.code != MXE_SUCCESS )	\
			return mx_private_status;		\
	} while(0)

MX_API char *mx_strerror(long error_code, char *buffer, size_t buffer_length);

MX_API long mx_errno_to_mx_status_code( int errno_value );

MX_API const char *mx_errno_string( long errno_value );
MX_API const char *mx_status_code_string( long mx_status_code_value );

MX_API void mx_set_error_output_function( void (*)( char * ) );
MX_API void mx_error_default_output_function( char *string );

MX_API mx_status_type mx_successful_result( void );

#define MX_SUCCESSFUL_RESULT	mx_successful_result()

#if defined(OS_WIN32)
MX_API long mx_win32_error_message( long error_code,
					char *buffer, size_t buffer_length );
#endif

/*------------------------------------------------------------------------*/

/* Setup the parts of the MX runtime environment that do not depend
 * on the presence of an MX database.
 */

MX_API mx_status_type mx_initialize_runtime( void ); 

/*------------------------------------------------------------------------*/

/* mx_copy_file() copies an old file to a new file where new_file_mode
 * specifies the permissions for the new file using the same bit patterns
 * for the mode as the Posix open() and creat() calls.
 */

MX_API mx_status_type mx_copy_file( char *original_filename,
				char *new_filename,
				int new_file_mode );

MX_API mx_status_type mx_get_os_version_string( char *version_string,
					size_t max_version_string_length );

MX_API mx_status_type mx_get_os_version( int *os_major,
					int *os_minor,
					int *os_update );

MX_API mx_status_type mx_get_current_directory_name( char *filename_buffer,
						size_t max_filename_length );

MX_API int mx_process_exists( unsigned long process_id );

MX_API mx_status_type mx_kill_process( unsigned long process_id );

MX_API char *mx_username( char *buffer, size_t buffer_length );

MX_API unsigned long mx_process_id( void );

MX_API int mx_get_max_file_descriptors( void );

MX_API char *mx_ctime_string( void );

MX_API char *mx_current_time_string( char *buffer, size_t buffer_length );

MX_API char *mx_skip_string_fields( char *buffer, int num_fields );

/* mx_string_split() extracts the next token from a string using the
 * characters in 'delim' as token separators.  It is similar to strsep()
 * except for the fact that it treats a string of several delimiters in
 * a row as being only one delimiter.  By contrast, strsep() would say
 * that there were empty tokens between each of the delimiter characters.
 * 
 * Please note that the original contents of *string_ptr are modified.
 */

MX_API char *mx_string_split( char **string_ptr, const char *delim );

/* === Define error message numbers. === */

#define MXE_SUCCESS				1000	/* No error. */

#define MXE_NULL_ARGUMENT			1001
#define MXE_ILLEGAL_ARGUMENT			1002
#define MXE_CORRUPT_DATA_STRUCTURE		1003
#define MXE_UNPARSEABLE_STRING			1004
#define MXE_END_OF_DATA				1005
#define MXE_UNEXPECTED_END_OF_DATA		1006
#define MXE_NOT_FOUND				1007
#define MXE_TYPE_MISMATCH			1008
#define MXE_NOT_YET_IMPLEMENTED			1009
#define MXE_UNSUPPORTED				1010
#define MXE_OUT_OF_MEMORY			1011
#define MXE_WOULD_EXCEED_LIMIT			1012
#define MXE_LIMIT_WAS_EXCEEDED			1013
#define MXE_INTERFACE_IO_ERROR			1014
#define MXE_DEVICE_IO_ERROR			1015
#define MXE_FILE_IO_ERROR			1016
#define MXE_TERMINAL_IO_ERROR			1017
#define MXE_IPC_IO_ERROR			1018
#define MXE_IPC_CONNECTION_LOST			1019
#define MXE_NETWORK_IO_ERROR			1020
#define MXE_NETWORK_CONNECTION_LOST		1021
#define MXE_NOT_READY				1022
#define MXE_INTERRUPTED				1023
#define MXE_PAUSE_REQUESTED			1024
#define MXE_INTERFACE_ACTION_FAILED		1025
#define MXE_DEVICE_ACTION_FAILED		1026
#define MXE_FUNCTION_FAILED			1027
#define MXE_CONTROLLER_INTERNAL_ERROR		1028
#define MXE_PERMISSION_DENIED			1029
#define MXE_CLIENT_REQUEST_DENIED		1030
#define MXE_TRY_AGAIN				1031
#define MXE_TIMED_OUT				1032
#define MXE_HARDWARE_CONFIGURATION_ERROR	1033
#define MXE_HARDWARE_FAULT			1034
#define MXE_RECORD_DISABLED_DUE_TO_FAULT	1035
#define MXE_RECORD_DISABLED_BY_USER		1036
#define MXE_INITIALIZATION_ERROR		1037
#define MXE_READ_ONLY				1038
#define MXE_SOFTWARE_CONFIGURATION_ERROR	1039
#define MXE_OPERATING_SYSTEM_ERROR		1040
#define MXE_UNKNOWN_ERROR			1041
#define MXE_NOT_VALID_FOR_CURRENT_STATE		1042
#define MXE_CONFIGURATION_CONFLICT		1043
#define MXE_NOT_AVAILABLE			1044
#define MXE_STOP_REQUESTED			1045
#define MXE_BAD_HANDLE				1046
#define MXE_OBJECT_ABANDONED			1047
#define MXE_MIGHT_CAUSE_DEADLOCK		1048
#define MXE_ALREADY_EXISTS			1049

#endif /* __MX_UTIL_H__ */
