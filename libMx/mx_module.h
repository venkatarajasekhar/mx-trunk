/*
 * Name:     mx_module.h
 *
 * Purpose:  Supports dynamically loadable MX modules.
 *
 * Author:   William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010-2012, 2014-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MODULE_H__
#define __MX_MODULE_H__

#include "mx_dynamic_library.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_MODULE_NAME_LENGTH		40
#define MXU_EXTENSION_NAME_LENGTH	40

#define MXF_EXT_IS_DISABLED		0x1
#define MXF_EXT_HAS_SCRIPT_LANGUAGE	0x2

typedef struct {
	char name[MXU_EXTENSION_NAME_LENGTH+1];
	struct mx_extension_function_list_type *extension_function_list;
	struct mx_module_type *module;
	MX_RECORD *record_list;
	unsigned long extension_flags;
	void *ext_private;
} MX_EXTENSION;

typedef struct mx_extension_function_list_type {
	mx_status_type ( *initialize )( MX_EXTENSION * );
	mx_status_type ( *finalize )( MX_EXTENSION * );
	mx_status_type ( *call )( MX_EXTENSION *, int argc, void **argv );
	mx_status_type ( *call_string )( MX_EXTENSION*, char * );
} MX_EXTENSION_FUNCTION_LIST;

typedef struct mx_module_type {
	char name[MXU_MODULE_NAME_LENGTH+1];
	unsigned long mx_version;

	MX_DRIVER *driver_table;
	MX_EXTENSION *extension_table;

	MX_DYNAMIC_LIBRARY *library;
	MX_RECORD *record_list;
} MX_MODULE;

typedef mx_bool_type (*MX_MODULE_INIT)( MX_MODULE * );

MX_API mx_status_type mx_load_module( char *filename,
					MX_RECORD *record_list,
					MX_MODULE **module );

MX_API mx_status_type mx_get_module( char *module_name,
					MX_RECORD *record_list,
					MX_MODULE **module );

MX_API mx_status_type mx_get_extension( char *extension_name,
					MX_RECORD *record_list,
					MX_EXTENSION **extension );

MX_API mx_status_type mx_finalize_extensions( MX_RECORD *record_list );

MX_API MX_EXTENSION *mx_get_default_script_extension( void );

MX_API void mx_set_default_script_extension( MX_EXTENSION *extension );

#ifdef __cplusplus
}
#endif

#endif /* __MX_MODULE_H__ */

