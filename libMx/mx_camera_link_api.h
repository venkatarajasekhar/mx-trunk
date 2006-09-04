/*
 * Name:    mx_camera_link_api.h
 *
 * Purpose: Camera Link standard API definitions for those vendors
 *          that do not provide a proper header file.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_CAMERA_LINK_API_H__
#define __MX_CAMERA_LINK_API_H__

/* Data types for Camera Link functions. */

#define hSerRef		void *
#define INT32		int32_t
#define UINT32		uint32_t
#define INT8		char

/* Status codes for Camera Link. */

#define CL_ERR_NO_ERR			0
#define CL_ERR_BUFFER_TOO_SMALL		(-10001)
#define CL_ERR_MANU_DOES_NOT_EXIST	(-10002)
#define CL_ERR_PORT_IN_USE		(-10003)
#define CL_ERR_TIMEOUT			(-10004)
#define CL_ERR_INVALID_INDEX		(-10005)
#define CL_ERR_INVALID_REFERENCE	(-10006)
#define CL_ERR_ERROR_NOT_FOUND		(-10007)
#define CL_ERR_BAUD_RATE_NOT_SUPPORTED	(-10008)
#define CL_ERR_OUT_OF_MEMORY		(-10009)

#define CL_ERR_UNABLE_TO_LOAD_DLL	(-10098)
#define CL_ERR_FUNCTION_NOT_FOUND	(-10099)

/* Camera Link function prototypes. */

MX_API INT32 MX_STDCALL clFlushPort( hSerRef serialRef );
MX_API INT32 MX_STDCALL clGetErrorText( const INT8* manuName, INT32 errorCode,
				INT8 *errorText, UINT32 *errorTextSize );
MX_API INT32 MX_STDCALL clGetManufacturerInfo( INT8 *ManufacturerName,
				UINT32 *bufferSize, UINT32 *version );
MX_API INT32 MX_STDCALL clGetNumBytesAvail( hSerRef serialRef,
				UINT32 *numBytes );
MX_API INT32 MX_STDCALL clGetNumPorts( UINT32 *numPorts );
MX_API INT32 MX_STDCALL clGetNumSerialPorts( UINT32 *numSerialPorts );
MX_API INT32 MX_STDCALL clGetPortInfo( UINT32 serialIndex,
				INT8 *manufacturerName, UINT32 *nameBytes,
				INT8 *portID, UINT32 *IDBytes,
				UINT32 *version );
MX_API INT32 MX_STDCALL clGetSerialPortIdentifier( UINT32 serialIndex,
				INT8 *portID, UINT32 *bufferSize );
MX_API INT32 MX_STDCALL clGetSupportedBaudRates( hSerRef serialRef,
				UINT32 *baudRates );
MX_API void  MX_STDCALL clSerialClose( hSerRef serialRef );
MX_API INT32 MX_STDCALL clSerialInit( UINT32 serialIndex,
				hSerRef *serialRefPtr );
MX_API INT32 MX_STDCALL clSerialRead( hSerRef serialRef, INT8 *buffer,
				UINT32 *numBytes, UINT32 SerialTimeout );
MX_API INT32 MX_STDCALL clSerialWrite( hSerRef serialRef, INT8 *buffer,
				UINT32 *numBytes, UINT32 SerialTimeout );
MX_API INT32 MX_STDCALL clSetBaudRate( hSerRef serialRef, UINT32 baudRate );

#endif /* __MX_CAMERA_LINK_API_H__ */

