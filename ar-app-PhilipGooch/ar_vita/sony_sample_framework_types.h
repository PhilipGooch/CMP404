/* SCE CONFIDENTIAL
 PlayStation(R)Vita Programmer Tool Runtime Library Release 02.100.071
 * Copyright (C) 2011 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#ifndef _SONY_SAMPLE_FRAMEWORK_TYPES_H
#define _SONY_SAMPLE_FRAMEWORK_TYPES_H

typedef uint_fast32_t	UInt;
typedef int_fast32_t	SInt;
typedef uintptr_t		UIntPtr;
typedef intptr_t		SIntPtr;

typedef uint_fast64_t	UInt64;
typedef uint_fast32_t	UInt32;
typedef uint_fast16_t	UInt16;
typedef uint_fast8_t	UInt8;
typedef int_fast64_t	SInt64;
typedef int_fast32_t	SInt32;
typedef int_fast16_t	SInt16;
typedef int_fast8_t		SInt8;

typedef uint64_t		UI64;
typedef uint32_t		UI32;
typedef uint16_t		UI16;
typedef uint8_t			UI8;
typedef int64_t			SI64;
typedef int32_t			SI32;
typedef int16_t			SI16;
typedef int8_t			SI8;

typedef struct{
	SceUID uid;
}MyEvent;

typedef struct{
	SceUID uid;
}MyEventFlag;

typedef SceKernelLwMutexWork MyMutex;

typedef enum{
	FALSE = 0,
	TRUE
}Bool;

#endif // _SONY_SAMPLE_FRAMEWORK_TYPES_H