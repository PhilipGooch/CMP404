/* SCE CONFIDENTIAL
 PlayStation(R)Vita Programmer Tool Runtime Library Release 02.100.071
 * Copyright (C) 2011 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sce_atomic.h>
#include <libsysmodule.h>

#define SCE_DBG_ASSERTS_ENABLED (1)

//#define ENABLE_DBGPRINTF
#include "sony_sample_framework.h"
//#include "../common/sample_main.h"

#include <libsmart.h>
#include <smart/target_tracking.h>
#pragma comment(lib, "SceSmart_stub")

#include <maths/vector4.h>
#include <maths/quaternion.h>


//#define ENABLE_RAZOR_CPU

//#define DELAY_FRAME			(0)
#define DELAY_FRAME			(1)
//#define DELAY_FRAME			(2)


#if defined(ENABLE_RAZOR_CPU)
#	include <libperf.h>
#endif

#define SMART_TRACKING_THREAD_PRIORITY		(SCE_KERNEL_DEFAULT_PRIORITY_USER)
#define SMART_TRACKING_THREAD_STACK_SIZE	(SCE_KERNEL_STACK_SIZE_MAX)
#define SMART_TRACKING_THREAD_CPU_AFFINITY_MASK	(SCE_KERNEL_THREAD_CPU_AFFINITY_MASK_DEFAULT)

#define SMART_TARGET_MAX		(6)

#define SMART_DICT_TRACKING_MAX	(0)
#define SMART_WAAR_TRACKING_MAX	(6)
#define SMART_TRACKING_MAX		SMART_DICT_TRACKING_MAX + SMART_WAAR_TRACKING_MAX

char const* smartSampleName(void){
	return "WAAR";
}

enum{
	TARGET_TRACKING_OP_NOP,
	TARGET_TRACKING_OP_START,
	TARGET_TRACKING_OP_STOP,
	TARGET_TRACKING_OP_RESET,
};

typedef struct{
	SceUInt32 timestamp;

	SInt detect;

	struct{
		UInt lost;
		SInt target;
		gef::Vector4 pos;
		gef::Quaternion rot;
		gef::Vector4 wpos;
		gef::Quaternion wrot;

		SceUInt32 timestamp;
		SceSmartTargetTrackingResult res;
	}tracking[SMART_TRACKING_MAX];

	struct{
		SceInt32 id;
	}target[SMART_TARGET_MAX];

#if DELAY_FRAME != 0
	struct{
		CameraBuffer* image_queue[DELAY_FRAME];
	}update;
#endif

	struct{
		volatile Bool exit;
		volatile Bool run;
		SceUID uID;
		MyMutex mtx;
		MyEvent evt;

		volatile SceSmartTargetTrackingResult res[SMART_TRACKING_MAX];
		volatile SceUInt32 timestamp;
		volatile int32_t op;
	}trackingThread;

	struct{
		struct{
			float volatile fps;
			SceUInt32 volatile pt;
			SceUInt32 t1;
		}tracking;
	}debug;

}TargetTrackingState;

static TargetTrackingState s_targetTrackingState;

static void _sampleDrawMarkers(void);

static int32_t _smartTrackingThread(uint32_t argSize, void* pArgBlock){
	TargetTrackingState* const s = &s_targetTrackingState;
	SceUInt64 timestamp = 0;
	Bool run = FALSE;

	UNUSED(argSize);
	UNUSED(pArgBlock);

	DEBUG_PRINTF("%s: start\n", __FUNCTION__);

	for(;;){
		myEventWait(&s->trackingThread.evt, INFINITE);
		if(s->trackingThread.exit != FALSE){
			break;
		}

		{
			int32_t op = sceAtomicExchange32(&s->trackingThread.op, TARGET_TRACKING_OP_NOP);

			if((run == FALSE) && (op == TARGET_TRACKING_OP_RESET)){
				op = TARGET_TRACKING_OP_START;
			}

			switch(op){
			case TARGET_TRACKING_OP_NOP:
				break;
			case TARGET_TRACKING_OP_START:
				if(run == FALSE){
					SCECHK(sceSmartTargetTrackingStart(SMART_DICT_TRACKING_MAX));
					run = TRUE;
				}
				break;
			case TARGET_TRACKING_OP_STOP:
				if(run != FALSE){
					SCECHK(sceSmartTargetTrackingStop());
					run = FALSE;
				}
				break;
			case TARGET_TRACKING_OP_RESET:
				if(run != FALSE){
					SCECHK(sceSmartTargetTrackingReset());
				}
				break;
			default:
				break;
			}
			s->trackingThread.run = run;
		}

		if(run != FALSE){
			SceSmartTargetTrackingResult res[SMART_TRACKING_MAX];
			SceInt32 num = 0;
			CameraBuffer* image;
			SceCameraRead* read;

			image = sampleGetCameraImage();
			read = sampleGetCameraBufferRead(image);

			if(read->qwTimestamp != timestamp){
				SceUInt32 t0;
				SceUInt32 t1;
				SceUInt32 t2;
				SceInt32 i;

				timestamp = read->qwTimestamp;

				t2 = sceKernelGetProcessTimeLow();

#if defined(ENABLE_RAZOR_CPU)
				SCECHK(sceRazorCpuPushMarkerWithHud("Update", 0, SCE_RAZOR_MARKER_DISABLE_HUD));
#endif
				{
					SceSmartTargetTrackingInput args;
					args.motion.states = sampleGetCameraMotion(image);
					args.motion.count = SCE_MOTION_MAX_NUM_STATES;
					args.image.data = read->pvIBase;
					args.image.timestamp = read->qwTimestamp;
					num = sceSmartTargetTrackingRun2(&args);
				}
				TOUCH(num);
				ASSERT(0 <= num);
#if 0
				num = sceSmartTargetTrackingGetResults(res, bulkof(res));
				ASSERT(0 <= num);
				for(i = num; i < bulkof(res); i++){
					res[i].detectedTargetID = -1;
					res[i].state = SCE_SMART_TARGET_TRACKING_STATE_IDLE;
				}
#else
				for(i = 0; (i < bulkof(res)) && (i < bulkof(s->target)); i++){
					SCECHK(sceSmartTargetTrackingQuery(&res[i], s->target[i].id));
				}
				for(; i < bulkof(res); i++){
					res[i].detectedTargetID = -1;
					res[i].state = SCE_SMART_TARGET_TRACKING_STATE_IDLE;
				}
#endif

#if defined(ENABLE_RAZOR_CPU)
				SCECHK(sceRazorCpuPopMarker());
#endif

				t0 = sceKernelGetProcessTimeLow();
				t1 = s->debug.tracking.t1;
				s->debug.tracking.t1 = t0;
				s->debug.tracking.fps = 1000000.0f / (t0 - t1);
				s->debug.tracking.pt = t0 - t2;
			}

			sampleUnlockCameraBuffer(image);

			{
				myMutexLock(&s->trackingThread.mtx);
				if(s->trackingThread.op == TARGET_TRACKING_OP_NOP){
					memcpy((void*)s->trackingThread.res, res, sizeof(s->trackingThread.res));
					s->trackingThread.timestamp = timestamp;
				}
				myMutexUnlock(&s->trackingThread.mtx);
			}
		}
	}

	if(run != FALSE){
		SCECHK(sceSmartTargetTrackingStop());
	}

	DEBUG_PRINTF("%s: exit\n", __FUNCTION__);

	return 0;
}

static void _smartSendTargetTrackingOp(int32_t op){
	TargetTrackingState* const s = &s_targetTrackingState;
	{
		UInt i;
		myMutexLock(&s->trackingThread.mtx);
		s->trackingThread.op = op;
		for(i = 0; i < bulkof(s->trackingThread.res); i++){
			s->trackingThread.res[i].state = SCE_SMART_TARGET_TRACKING_STATE_IDLE;
		}
		myMutexUnlock(&s->trackingThread.mtx);
	}

	{
		{
			UInt i;
			for(i = 0; i < bulkof(s->tracking); i++){
				s->tracking[i].target = -1;
				s->tracking[i].lost = 1;
			}
			s->detect = -1;
			sampleSetCameraAutoControlHold(TRUE);
		}
	}
}

static void _smartUpdateResult(CameraBuffer* image){
	TargetTrackingState* const s = &s_targetTrackingState;

#if DELAY_FRAME != 0
	{
		CameraBuffer* dimage = s->update.image_queue[0];
		UInt i = 0;
#if DELAY_FRAME > 1
		for(i = 0; i < DELAY_FRAME - 1; i++){
			s->update.image_queue[i] = s->update.image_queue[i + 1];
		}
#endif
		sampleLockCameraBuffer(image);
		s->update.image_queue[i] = image;
		image = dimage;
		sampleOverrideCameraImage(image);
		sampleUnlockCameraBuffer(image);
	}
#else
	UNUSED(image);
#endif

	{
		UInt i;
		myMutexLock(&s->trackingThread.mtx);
		ASSERT(bulkof(s->trackingThread.res) >= bulkof(s->tracking));
		for(i = 0; i < bulkof(s->tracking); i++){
//			s->tracking[i].res = s->trackingThread.res[i];
			s->tracking[i].res.detectedTargetID = s->trackingThread.res[i].detectedTargetID;
			s->tracking[i].res.pos.x = s->trackingThread.res[i].pos.x;
			s->tracking[i].res.pos.y = s->trackingThread.res[i].pos.y;
			s->tracking[i].res.pos.z = s->trackingThread.res[i].pos.z;
			s->tracking[i].res.rot.x = s->trackingThread.res[i].rot.x;
			s->tracking[i].res.rot.y = s->trackingThread.res[i].rot.y;
			s->tracking[i].res.rot.z = s->trackingThread.res[i].rot.z;
			s->tracking[i].res.rot.w = s->trackingThread.res[i].rot.w;
			s->tracking[i].res.state = s->trackingThread.res[i].state;
			s->tracking[i].timestamp = s->trackingThread.timestamp;
		}
		myMutexUnlock(&s->trackingThread.mtx);
	}

	myEventRaise(&s->trackingThread.evt);

	{
		UInt i;
		for(i = 0; i < bulkof(s->tracking); i++){
			if(s->tracking[i].res.state == SCE_SMART_TARGET_TRACKING_STATE_TARGET_TRACKING){
				{
					SceSmartTargetTrackingResult* res = &s->tracking[i].res;
					s->tracking[i].wpos = gef::Vector4(res->pos.x, res->pos.y, res->pos.z);
					s->tracking[i].wrot = gef::Quaternion(res->rot.x, res->rot.y, res->rot.z, res->rot.w);
				}

				{
					SceInt32 detected = s->tracking[i].res.detectedTargetID;
					if((s->tracking[i].target < 0) || (s->target[s->tracking[i].target].id != detected)){
						UInt j;
						for(j = 0; j < bulkof(s->target); j++){
							if(s->target[j].id == detected){
								break;
							}
						}
						s->tracking[i].target = (SInt)j;
						if(s->detect < 0){
							s->detect = (SInt)j;
							sampleSetCameraAutoControlHold(TRUE);
						}
					}
				}

				{
					gef::Vector4 v0 = s->tracking[i].wpos;
					gef::Quaternion q0 = s->tracking[i].wrot;
					s->tracking[i].pos = v0;
					s->tracking[i].rot = q0;
				}

				s->tracking[i].lost = 0;

			}else{
				s->tracking[i].target = -1;
				s->tracking[i].lost = 1;
			}
		}
	}
}

static void _smartRegisterTargets(void){
	TargetTrackingState* const s = &s_targetTrackingState;
	static SceInt32 const list[] = {
		SCE_SMART_MARKER_01,
		SCE_SMART_MARKER_02,
		SCE_SMART_MARKER_03,
		SCE_SMART_MARKER_04,
		SCE_SMART_MARKER_05,
		SCE_SMART_MARKER_06,
	};
	UInt i;
	ASSERT(bulkof(list) <= SMART_TARGET_MAX);
	for(i = 0; i < bulkof(list); i++){
		s->target[i].id = list[i];
		SCECHK(sceSmartTargetTrackingRegisterTarget(list[i]));
	}
	for(; i < SMART_TARGET_MAX; i++){
		s->target[i].id = SCE_SMART_INVALID_TARGET_ID;
	}
}

static void _smartUnregisterTargets(void){
	TargetTrackingState* const s = &s_targetTrackingState;
	UInt i;
	for(i = 0; i < bulkof(s->target); i++){
		SceInt32 targetId = s->target[i].id;
		if(targetId == SCE_SMART_INVALID_TARGET_ID){
			continue;
		}
		SCECHK(sceSmartTargetTrackingUnregisterTarget(targetId));
	}
}

void smartInitialize(void){
	TargetTrackingState* const s = &s_targetTrackingState;

	memset(&s_targetTrackingState, 0, sizeof(s_targetTrackingState));

#if defined(SCE_SYSMODULE_SMART)
	SCECHK(sceSysmoduleLoadModule(SCE_SYSMODULE_SMART));
#endif

	SCECHK(sceSmartInit(NULL));

	_smartRegisterTargets();

	{
		UInt i;
		s->detect = -1;
		for(i = 0; i < bulkof(s->tracking); i++){
			s->tracking[i].lost = 1;
			s->tracking[i].target = -1;
			s->tracking[i].res.state = SCE_SMART_TARGET_TRACKING_STATE_IDLE;
		}
		for(i = 0; i < bulkof(s->trackingThread.res); i++){
			s->trackingThread.res[i].state = SCE_SMART_TARGET_TRACKING_STATE_IDLE;
		}

		s->trackingThread.op = TARGET_TRACKING_OP_NOP;
		s->trackingThread.run = FALSE;
	}

	{
		s->trackingThread.exit = FALSE;
		myEventCreate(&s->trackingThread.evt);
		myMutexCreate(&s->trackingThread.mtx);
		s->trackingThread.uID = sceKernelCreateThread("smartTracking", _smartTrackingThread, SMART_TRACKING_THREAD_PRIORITY, SMART_TRACKING_THREAD_STACK_SIZE, 0, SMART_TRACKING_THREAD_CPU_AFFINITY_MASK, NULL);
		ASSERT(s->trackingThread.uID >= SCE_OK);
		SCECHK(sceKernelStartThread(s->trackingThread.uID, 0, NULL));
	}
}

void smartRelease(void){
	TargetTrackingState* const s = &s_targetTrackingState;

	if(s->trackingThread.uID >= SCE_OK){
		s->trackingThread.exit = TRUE;
		myEventRaise(&s->trackingThread.evt);
		SCECHK(sceKernelWaitThreadEnd(s->trackingThread.uID, NULL, NULL));
		SCECHK(sceKernelDeleteThread(s->trackingThread.uID));
		s->trackingThread.uID = -1;
	}

	myMutexRelease(&s->trackingThread.mtx);
	myEventRelease(&s->trackingThread.evt);

	_smartUnregisterTargets();

	SCECHK(sceSmartRelease());

#if defined(SCE_SYSMODULE_SMART)
	SCECHK(sceSysmoduleUnloadModule(SCE_SYSMODULE_SMART));
#endif

	memset(&s_targetTrackingState, 0, sizeof(s_targetTrackingState));
}

void smartUpdate(CameraBuffer* image){
	_smartUpdateResult(image);

	_sampleDrawMarkers();
}

void smartTrackingReset()
{
	_smartSendTargetTrackingOp(TARGET_TRACKING_OP_RESET);
}


static void _sampleDrawMarkers(void){
	TargetTrackingState* const s = &s_targetTrackingState;
	UInt i;

	// targets
	for(i = 0; i < bulkof(s->tracking); i++){
		sampleSetMarkerData(i, s->tracking[i].lost, s->tracking[i].target);
		if((s->tracking[i].target >= 0) && (s->tracking[i].lost == 0)){

			if(s->tracking[i].target < bulkof(s->target)){
				SceSmartTargetInfo info;
				SCECHK(sceSmartGetTargetInfo(s->target[s->tracking[i].target].id, &info));
				float hw = info.physicalWidth * 0.5f;
				float hh = info.physicalHeight * 0.5f;
			}

			gef::Matrix44 m0, m1, m2;
			gef::Vector4 v0;
			gef::Quaternion q0;

			q0.Conjugate(s->tracking[i].rot);
			v0 = s->tracking[i].pos * -1.0f;

			m1.Rotation(q0);
			m2.SetIdentity();
			m2.SetTranslation(v0);
			m0 = m2 * m1;

			sampleSetTransform(i, &m0);
		}
	}
}

