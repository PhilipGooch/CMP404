/* SCE CONFIDENTIAL
 PlayStation(R)Vita Programmer Tool Runtime Library Release 02.100.071
 * Copyright (C) 2011 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#ifndef _SONY_SAMPLE_FRAMEWORK_H
#define _SONY_SAMPLE_FRAMEWORK_H

#include <stdint.h>
#include <kernel.h>
#include <camera.h>
#include <libdbg.h>
#include <libsmart.h>
#include <stddef.h>

#include <maths/matrix44.h>

#include "sony_sample_framework_types.h"
#include "sony_camera.h"



// FORWARD DECLARATIONS
class CameraBuffer;



// AR DECLARATIONS
#define CAMERA_RESOLUTION		(SCE_CAMERA_RESOLUTION_VGA)
#define CAMERA_FRAMERATE		(SCE_CAMERA_FRAMERATE_60)
#define CAMERA_WIDTH			(640)
#define CAMERA_HEIGHT			(480)
//#define CAMERA_DEVICE			(SCE_CAMERA_DEVICE_FRONT)
#define CAMERA_DEVICE			(SCE_CAMERA_DEVICE_BACK)
#define CAMERA_BUFFER_COUNT		(8)
#define CAMERA_FORMAT			(SCE_CAMERA_FORMAT_YUV420_PLANE)

#define CAMERA_ATTRIBUTE_ISO			(SCE_CAMERA_ATTRIBUTE_ISO_AUTO)
#define CAMERA_ATTRIBUTE_WHITEBALANCE	(SCE_CAMERA_ATTRIBUTE_WHITEBALANCE_AUTO)

#define CAMERA_THREAD_PRIORITY		(SCE_KERNEL_DEFAULT_PRIORITY_USER - 10)
#define CAMERA_THREAD_AFFINITY_MASK	(SCE_KERNEL_THREAD_CPU_AFFINITY_MASK_DEFAULT)
#define CAMERA_THREAD_STACK_SIZE	(SCE_KERNEL_STACK_SIZE_DEFAULT)

#define APP_DATA_COUNT			(3)

#define VSYNC_THREAD_PRIORITY		(SCE_KERNEL_DEFAULT_PRIORITY_USER - 10)
#define VSYNC_THREAD_AFFINITY_MASK	(SCE_KERNEL_THREAD_CPU_AFFINITY_MASK_DEFAULT)
#define VSYNC_THREAD_STACK_SIZE		(SCE_KERNEL_STACK_SIZE_DEFAULT)

#define PROJECTION_FOV			SCE_SMART_IMAGE_FOV
#define PROJECTION_ZNEAR		(0.01f)
#define PROJECTION_ZFAR			(10.0f)
#define PROJECTION_WIDTH		SCE_SMART_IMAGE_WIDTH
#define PROJECTION_HEIGHT		SCE_SMART_IMAGE_HEIGHT

#define DISPLAY_WIDTH			960
#define DISPLAY_HEIGHT			544

#define VIEW_SCALE_H			(1.0f)
#define VIEW_SCALE_V			((float)CAMERA_HEIGHT / CAMERA_WIDTH * DISPLAY_WIDTH / DISPLAY_HEIGHT)
//#define VIEW_SCALE_V			((float)1.0f)



#define MY_EVENT_FLAG_WAITMODE_AND		(SCE_KERNEL_EVF_WAITMODE_AND)
#define MY_EVENT_FLAG_WAITMODE_OR		(SCE_KERNEL_EVF_WAITMODE_OR)



struct MarkerData
{
	bool lost;
	SceInt32 target;
};

struct AppData{
	AppData* prev;
	AppData* next;

	UInt ref;
	CameraBuffer* currentImage;

	struct{
		gef::Matrix44 viewMat;
		gef::Matrix44 projMat;
	}world;

	struct
	{
		gef::Matrix44 transform[6];
	}transforms;

	struct
	{
		MarkerData marker[6];
	} markers;

};

typedef struct{


	struct{
		struct{
			Bool volatile exit;
			SceUID uID;
			MyEvent evt;
		}thread;
	}vsync;

	struct{
		SceCameraInfo info;
		CameraBuffer db[CAMERA_BUFFER_COUNT];
		CameraBuffer* head;
		CameraBuffer* tail;

		MyMutex mtx;
	}camera;

	struct{

		struct{
			UInt frame;
			float fps;
			struct{
				SceUInt32 pt;
			}update;
			struct{
				SceUInt32 pt;
			}render;
		}debug;

		struct{
			SceUInt32 t1;
		}time;

		struct{
			MyMutex mtx;
			AppData buf[APP_DATA_COUNT];
			AppData* head;
			AppData* tail;
		}data;

		struct{
			AppData* current;
		}update;

		struct{
			AppData* data;
			MyEvent flip_evt;
		}render;

	}app;


}SampleState;

#if defined(ENABLE_DBGPRINTF)
#	define DEBUG_PRINTF(a, ...)		myDebugPrintf(__FUNCTION__, __LINE__, (a), ##__VA_ARGS__)
#	define ASSERT(a)				{ (a) ? (void)0 : (void)(myDebugPrintf(__FUNCTION__, __LINE__, "assertion failed: " #a "\n"), __breakpoint(0), abort()); }
#	define ASSERT_MSG(a, frm, ...)	{ (a) ? (void)0 : (void)(myDebugPrintf(__FUNCTION__, __LINE__, frm, ##__VA_ARGS__), __breakpoint(0), abort()); }
#elif defined(_DEBUG)
#	define DEBUG_PRINTF				SCE_DBG_LOG_INFO
#	define ASSERT					SCE_DBG_ASSERT
#	define ASSERT_MSG				SCE_DBG_ASSERT_MSG
#else
#	define DEBUG_PRINTF(a, ...)
#	define ASSERT(a)
#	define ASSERT_MSG(a, ...)
#endif

#define SCECHK(a)		{ int sceres = (a); (void)sceres; ASSERT_MSG(sceres >= SCE_OK, "SceError: 0x%08x: " #a "\n", sceres); }
#define SCEWRN(a)		{ int sceres = (a); if(sceres < SCE_OK){ DEBUG_PRINTF("SceError: 0x%08x: " #a "\n", sceres); } }

#if defined(__GNUC__) || defined(__SNC__)
#	define DECLARE_ALIGNED(a, s)	a __attribute__((aligned(s)))
#	define ALWAYS_INLINE			__attribute__((always_inline))
#	define INLINE					extern inline
#endif

#define UNUSED(a)		(void)(a)
#define TOUCH(a)		(void)(sizeof(a))
#define ALIGN(x, a)		(((x) + ((a) - 1)) & ~((a) - 1))

#define INFINITE		(-1)

#define bulkof(a)		(sizeof(a) / sizeof(a[0]))
#define alignof(t)		offsetof(struct { char c; t x; }, x)






INLINE void myEventCreate(MyEvent* evt){
	SceInt32 res;
	SCECHK(res = sceKernelCreateEventFlag(__FUNCTION__, SCE_KERNEL_EVF_ATTR_TH_FIFO | SCE_KERNEL_EVF_ATTR_MULTI, 0, NULL));
	evt->uid = res;
}
INLINE void myEventRelease(MyEvent* evt){
	if(evt->uid >= 0){
		SCECHK(sceKernelDeleteEventFlag(evt->uid));
		evt->uid = -1;
	}
}
INLINE void myEventRaise(MyEvent* evt){
	SCECHK(sceKernelSetEventFlag(evt->uid, 1));
}
INLINE void myEventClear(MyEvent* evt){
	SCECHK(sceKernelClearEventFlag(evt->uid, 0));
}
INLINE Bool myEventWait(MyEvent* evt, SInt msec){
	SceInt32 res;
	if(msec == 0){
		res = sceKernelPollEventFlag(evt->uid, 1, SCE_KERNEL_EVF_WAITMODE_OR | SCE_KERNEL_EVF_WAITMODE_CLEAR_PAT, NULL);
		ASSERT((res == SCE_OK) || (res == SCE_KERNEL_ERROR_EVF_COND));
	}else if(msec < 0){
		res = sceKernelWaitEventFlag(evt->uid, 1, SCE_KERNEL_EVF_WAITMODE_OR | SCE_KERNEL_EVF_WAITMODE_CLEAR_PAT, NULL, NULL);
		ASSERT(res == SCE_OK);
	}else{
		SceUInt32 timeout;
		timeout = (SceUInt32)msec * 1000;
		res = sceKernelWaitEventFlag(evt->uid, 1, SCE_KERNEL_EVF_WAITMODE_OR | SCE_KERNEL_EVF_WAITMODE_CLEAR_PAT, NULL, &timeout);
		ASSERT((res == SCE_OK) || (res == SCE_KERNEL_ERROR_WAIT_TIMEOUT));
	}
	return (res == SCE_OK) ? TRUE : FALSE;
}

INLINE void myEventFlagCreate(MyEventFlag* flg){
	SceInt32 res;
	SCECHK(res = sceKernelCreateEventFlag(__FUNCTION__, SCE_KERNEL_EVF_ATTR_TH_FIFO | SCE_KERNEL_EVF_ATTR_MULTI, 0, NULL));
	flg->uid = res;
}
INLINE void myEventFlagRelease(MyEventFlag* flg){
	if(flg->uid >= 0){
		SCECHK(sceKernelDeleteEventFlag(flg->uid));
		flg->uid = -1;
	}
}
INLINE void myEventFlagSet(MyEventFlag* flg, UInt32 pattern){
	SCECHK(sceKernelSetEventFlag(flg->uid, pattern));
}
INLINE void myEventFlagClear(MyEventFlag* flg, UInt32 pattern){
	SCECHK(sceKernelClearEventFlag(flg->uid, pattern));
}
INLINE Bool myEventFlagWait(MyEventFlag* flg, UInt32 pattern, UInt32 waitmode, SInt msec){
	SceInt32 res;
	if(msec == 0){
		res = sceKernelPollEventFlag(flg->uid, pattern, waitmode | SCE_KERNEL_EVF_WAITMODE_CLEAR_PAT, NULL);
		ASSERT((res == SCE_OK) || (res == SCE_KERNEL_ERROR_EVF_COND));
	}else if(msec < 0){
		res = sceKernelWaitEventFlag(flg->uid, pattern, waitmode | SCE_KERNEL_EVF_WAITMODE_CLEAR_PAT, NULL, NULL);
		ASSERT((res == SCE_OK));
	}else{
		SceUInt32 timeout;
		timeout = (SceUInt32)msec * 1000;
		res = sceKernelWaitEventFlag(flg->uid, pattern, waitmode | SCE_KERNEL_EVF_WAITMODE_CLEAR_PAT, NULL, &timeout);
	ASSERT((res == SCE_OK) || (res == SCE_KERNEL_ERROR_WAIT_TIMEOUT));
	}
	return (res == SCE_OK) ? TRUE : FALSE;
}

INLINE void myMutexCreate(MyMutex* mtx){
	SCECHK(sceKernelCreateLwMutex(mtx, __FUNCTION__, SCE_KERNEL_LW_MUTEX_ATTR_TH_FIFO, 0, NULL));
}
INLINE void myMutexRelease(MyMutex* mtx){
	SCECHK(sceKernelDeleteLwMutex(mtx));
}
INLINE void myMutexLock(MyMutex* mtx){
	SCECHK(sceKernelLockLwMutex(mtx, 1, NULL));
}
INLINE void myMutexUnlock(MyMutex* mtx){
	SCECHK(sceKernelUnlockLwMutex(mtx, 1));
}
INLINE Bool myMutexTryLock(MyMutex* mtx, SInt msec){
	SceInt32 res;
	if(msec == 0){
		res = sceKernelTryLockLwMutex(mtx, 1);
		ASSERT((res == SCE_OK) || (res == SCE_KERNEL_ERROR_LW_MUTEX_FAILED_TO_OWN));
	}else if(msec < 0){
		res = sceKernelLockLwMutex(mtx, 1, NULL);
		ASSERT((res == SCE_OK));
	}else{
		SceUInt32 timeout;
		timeout = (SceUInt32)msec * 1000;
		res = sceKernelLockLwMutex(mtx, 1, &timeout);
		ASSERT((res == SCE_OK) || (res == SCE_KERNEL_ERROR_WAIT_TIMEOUT));
	}
	return (res == SCE_OK) ? TRUE : FALSE;
}


void sampleInitialize();
void sampleRelease();
AppData* sampleUpdateBegin();
void sampleUpdateEnd(AppData* dat);
AppData* sampleRenderBegin();
void sampleRenderEnd();

void sampleSetTransform(int idx, gef::Matrix44* mat);
void sampleGetTransform(int idx, gef::Matrix44* mat);
void sampleSetMarkerData(int idx, bool lost, SceUInt32 target);
bool sampleIsMarkerFound(int idx);

#endif // _SONY_SAMPLE_FRAMEWORK_H
