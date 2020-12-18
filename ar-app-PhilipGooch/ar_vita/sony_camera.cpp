/* SCE CONFIDENTIAL
 PlayStation(R)Vita Programmer Tool Runtime Library Release 02.100.071
 * Copyright (C) 2011 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include "sony_sample_framework.h"

#include <sce_atomic.h>
#include <string.h>


#ifdef __cplusplus
extern "C"
{
#endif

void *graphicsAlloc(SceKernelMemBlockType type, uint32_t size, uint32_t alignment, uint32_t attribs, SceUID *uid);
void graphicsFree(SceUID uid);

#ifdef __cplusplus
} // extern "C"
#endif

extern SampleState s_sampleState;

static CameraBuffer* _cameraReserveBuffer(void){
	SampleState* const s = &s_sampleState;
	CameraBuffer* buf = NULL;
	myMutexLock(&s->camera.mtx);
	{
		buf = s->camera.tail;
		while(buf != NULL){
			if(buf->ref == 0){
				break;
			}
			buf = buf->prev;
		}
		if(buf != NULL){
			if(buf->next != NULL){
				buf->next->prev = buf->prev;
			}
			if(buf->prev != NULL){
				buf->prev->next = buf->next;
			}
			if(buf == s->camera.head){
				s->camera.head = buf->next;
			}
			if(buf == s->camera.tail){
				s->camera.tail = buf->prev;
			}
		}
	}
	myMutexUnlock(&s->camera.mtx);
	return buf;
}

static void _cameraCancelBuffer(CameraBuffer* buf){
	SampleState* const s = &s_sampleState;
	myMutexLock(&s->camera.mtx);
	{
		if(s->camera.tail == NULL){
			ASSERT(s->camera.head == NULL);
			buf->prev = NULL;
			buf->next = NULL;
			s->camera.head = buf;
			s->camera.tail = buf;
		}else{
			ASSERT(s->camera.tail->next == NULL);
			s->camera.tail->next = buf;
			buf->prev = s->camera.tail;
			buf->next = NULL;
			s->camera.tail = buf;
		}
	}
	myMutexUnlock(&s->camera.mtx);
}

static void _cameraRegistBuffer(CameraBuffer* buf){
	SampleState* const s = &s_sampleState;
	myMutexLock(&s->camera.mtx);
	{
		if(s->camera.head == NULL){
			ASSERT(s->camera.tail == NULL);
			buf->prev = NULL;
			buf->next = NULL;
			s->camera.head = buf;
			s->camera.tail = buf;
		}else{
			ASSERT(s->camera.head->prev == NULL);
			s->camera.head->prev = buf;
			buf->prev = NULL;
			buf->next = s->camera.head;
			s->camera.head = buf;
		}
	}
	myMutexUnlock(&s->camera.mtx);
}

CameraBuffer* cameraGetLatestImage(void){
	SampleState* const s = &s_sampleState;
	CameraBuffer* buf = NULL;
	myMutexLock(&s->camera.mtx);
	{
		buf = s->camera.head;
		if(buf != NULL){
#if 1
			sceAtomicIncrement32(&buf->ref);
#else
			buf->ref++;
#endif
		}
	}
	myMutexUnlock(&s->camera.mtx);
	return buf;
}

void cameraBufferAddRef(CameraBuffer* buf){
	//SampleState* const s = &s_sampleState;
	if(buf == NULL){
		return;
	}
	ASSERT(buf->ref > 0);
#if 1
	sceAtomicIncrement32(&buf->ref);
#else
	myMutexLock(&s->camera.mtx);
	{
		buf->ref++;
	}
	myMutexUnlock(&s->camera.mtx);
#endif
}

void cameraBufferRelease(CameraBuffer* buf){
	//SampleState* const s = &s_sampleState;
	ASSERT(buf->ref > 0);
#if 1
	sceAtomicDecrement32(&buf->ref);
#else
	myMutexLock(&s->camera.mtx);
	{
		buf->ref_count--;
	}
	myMutexUnlock(&s->camera.mtx);
#endif
}


void cameraInitialize(void){
	SampleState* const s = &s_sampleState;
	UInt i;
	UInt uv_size;

	memset(&s->camera.info, 0, sizeof(s->camera.info));
	s->camera.info.sizeThis = sizeof(SceCameraInfo);
	s->camera.info.wPriority = SCE_CAMERA_PRIORITY_SHARE;
	s->camera.info.wFormat = CAMERA_FORMAT;
	s->camera.info.wResolution = CAMERA_RESOLUTION;
	s->camera.info.wFramerate = CAMERA_FRAMERATE;
	s->camera.info.wRange = SCE_CAMERA_DATA_RANGE_FULL;
	s->camera.info.wBuffer = SCE_CAMERA_BUFFER_SETBYREAD;
	s->camera.info.wPitch = 0;
	uv_size = CAMERA_WIDTH * CAMERA_HEIGHT / 4;

	for(i = 0; i < CAMERA_BUFFER_COUNT; i++){
		memset(&s->camera.db[i].read, 0, sizeof(s->camera.db[i].read));
		s->camera.db[i].read.sizeThis = sizeof(SceCameraRead);
		s->camera.db[i].read.dwMode = SCE_CAMERA_READ_MODE_WAIT_NEXTFRAME_ON;
		s->camera.db[i].read.dwExposureTimeMode = SCE_CAMERA_READ_GET_EXPOSURE_TIME_OFF;
		s->camera.db[i].read.sizeIBase = CAMERA_WIDTH * CAMERA_HEIGHT;
		s->camera.db[i].read.sizeUBase = uv_size;
		s->camera.db[i].read.sizeVBase = uv_size;
		//s->camera.db[i].buf = graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW, CAMERA_WIDTH * CAMERA_HEIGHT + uv_size * 2, SCE_GXM_TEXTURE_ALIGNMENT, SCE_GXM_MEMORY_ATTRIB_READ, &s->camera.db[i].uID);
		s->camera.db[i].buf = graphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RWDATA, CAMERA_WIDTH * CAMERA_HEIGHT + uv_size * 2, SCE_GXM_TEXTURE_ALIGNMENT, SCE_GXM_MEMORY_ATTRIB_READ, &s->camera.db[i].uID);
		memset(s->camera.db[i].buf, 0, CAMERA_WIDTH * CAMERA_HEIGHT + uv_size * 2);
		{
			void* base = (void*)((UIntPtr)s->camera.db[i].buf);
			s->camera.db[i].read.pvIBase = base;
			s->camera.db[i].read.pvUBase = (void*)((UIntPtr)base + CAMERA_WIDTH * CAMERA_HEIGHT);
			s->camera.db[i].read.pvVBase = (void*)((UIntPtr)base + CAMERA_WIDTH * CAMERA_HEIGHT + uv_size);
		}
		{
			void* base = (void*)((UIntPtr)s->camera.db[i].buf);
			SCECHK(sceGxmTextureInitLinear(&s->camera.db[i].tex_yuv, base, SCE_GXM_TEXTURE_FORMAT_YUV420P3_CSC0, CAMERA_WIDTH, CAMERA_HEIGHT, 0));
		}
		SCECHK(sceGxmTextureSetMinFilter(&s->camera.db[i].tex_yuv, SCE_GXM_TEXTURE_FILTER_LINEAR));
		SCECHK(sceGxmTextureSetMagFilter(&s->camera.db[i].tex_yuv, SCE_GXM_TEXTURE_FILTER_LINEAR));
		SCECHK(sceGxmTextureSetMipFilter(&s->camera.db[i].tex_yuv, SCE_GXM_TEXTURE_MIP_FILTER_DISABLED));
		SCECHK(sceGxmTextureSetUAddrMode(&s->camera.db[i].tex_yuv, SCE_GXM_TEXTURE_ADDR_CLAMP));
		SCECHK(sceGxmTextureSetVAddrMode(&s->camera.db[i].tex_yuv, SCE_GXM_TEXTURE_ADDR_CLAMP));
	}
	{
		CameraBuffer* prv = &s->camera.db[0];
		prv->prev = NULL;
		for(i = 1; i < CAMERA_BUFFER_COUNT; i++){
			CameraBuffer* cur = &s->camera.db[i];
			prv->next = cur;
			cur->prev = prv;
			prv = cur;
		}
		prv->next = NULL;
		s->camera.head = &s->camera.db[0];
		s->camera.tail = &s->camera.db[CAMERA_BUFFER_COUNT - 1];
	}

	myMutexCreate(&s->camera.mtx);

	SCECHK(sceCameraOpen(CAMERA_DEVICE, &s->camera.info));

	SCECHK(sceCameraStart(CAMERA_DEVICE));

	SCECHK(sceCameraSetAutoControlHold(CAMERA_DEVICE, SCE_CAMERA_ATTRIBUTE_AUTOCONTROLHOLD_OFF));

	SCECHK(sceCameraSetISO(CAMERA_DEVICE, CAMERA_ATTRIBUTE_ISO));

	SCECHK(sceCameraSetWhiteBalance(CAMERA_DEVICE, CAMERA_ATTRIBUTE_WHITEBALANCE));

	//SCECHK(sceCameraSetNightmode(CAMERA_DEVICE,SCE_CAMERA_ATTRIBUTE_NIGHTMODE_LESS10));
}

void cameraRelease(void){
	SampleState* const s = &s_sampleState;
	UInt i;

	SCECHK(sceCameraStop(CAMERA_DEVICE));
	SCECHK(sceCameraClose(CAMERA_DEVICE));

	myMutexRelease(&s->camera.mtx);

	for(i = 0; i < CAMERA_BUFFER_COUNT; i++){
		graphicsFree(s->camera.db[i].uID);
		s->camera.db[i].uID = -1;
		s->camera.db[i].buf = NULL;
	}
}

Bool cameraUpdate(void){
	SampleState* const s = &s_sampleState;
	CameraBuffer* buf = NULL;
	TOUCH(s);

	buf = _cameraReserveBuffer();
	ASSERT(buf != NULL);
	if(buf == NULL){
		return FALSE;
	}

	{
		int res;
#if defined(ENABLE_RAZOR_CPU)
		SCECHK(sceRazorCpuPushMarkerWithHud("sceCameraRead", 0, SCE_RAZOR_MARKER_DISABLE_HUD));
#endif
		res = sceCameraRead(CAMERA_DEVICE, &buf->read);

#if defined(ENABLE_RAZOR_CPU)
		SCECHK(sceRazorCpuPopMarker());
#endif
		if(res != SCE_OK){
			DEBUG_PRINTF("sceCameraRead() = %08x\n", res);
			_cameraCancelBuffer(buf);
			return FALSE;
		}
		if(buf->read.dwStatus < SCE_CAMERA_STATUS_IS_ACTIVE){
			DEBUG_PRINTF("dwStatus < SCE_CAMERA_STATUS_IS_ACTIVE: %08x\n", buf->read.dwStatus);
			_cameraCancelBuffer(buf);
			return FALSE;
		}
#if 1
		if((buf->read.dwStatus != SCE_CAMERA_STATUS_IS_ACTIVE) && (buf->read.dwStatus != SCE_CAMERA_STATUS_IS_NOT_STABLE)){
			//DEBUG_PRINTF("dwStatus: %08x: drop\n", buf->read.dwStatus);
			_cameraCancelBuffer(buf);
			return FALSE;
		}
#endif

		SCECHK(sceMotionGetSensorState(buf->motion, bulkof(buf->motion)));

		_cameraRegistBuffer(buf);
	}
	return TRUE;
}

CameraBuffer* sampleGetCameraImage(void){
	//SampleState* const s = &s_sampleState;
	return cameraGetLatestImage();
}
SceMotionSensorState* sampleGetCameraMotion(CameraBuffer* buf){
	if(buf == NULL){
		return NULL;
	}
	return buf->motion;
}
SceCameraRead* sampleGetCameraBufferRead(CameraBuffer* buf){
	if(buf == NULL){
		return NULL;
	}
	return &buf->read;
}
void sampleLockCameraBuffer(CameraBuffer* buf){
	//SampleState* const s = &s_sampleState;
	if(buf == NULL){
		return;
	}
	cameraBufferAddRef(buf);
}
void sampleUnlockCameraBuffer(CameraBuffer* buf){
	//SampleState* const s = &s_sampleState;
	if(buf == NULL){
		return;
	}
	cameraBufferRelease(buf);
}
void sampleSetCameraAutoControlHold(Bool hold){
	//DEBUG_PRINTF((hold != FALSE) ? "camera auto control hold on\n" : "camera auto control hold off\n");
#if defined(ENABLE_RAZOR_CPU)
	SCECHK(sceRazorCpuPushMarkerWithHud("sceCameraSetAutoControlHold", 0, SCE_RAZOR_MARKER_DISABLE_HUD));
#endif
	SCECHK(sceCameraSetAutoControlHold(CAMERA_DEVICE, (hold != FALSE) ? SCE_CAMERA_ATTRIBUTE_AUTOCONTROLHOLD_ON : SCE_CAMERA_ATTRIBUTE_AUTOCONTROLHOLD_OFF));
#if defined(ENABLE_RAZOR_CPU)
	SCECHK(sceRazorCpuPopMarker());
#endif
}


void sampleOverrideCameraImage(CameraBuffer* image){
	SampleState* const s = &s_sampleState;
	AppData* dat = s->app.update.current;
	ASSERT(dat != NULL);
	if(image != NULL){
		cameraBufferAddRef(image);
	}
	if(dat->currentImage != NULL){
		cameraBufferRelease(dat->currentImage);
	}
	dat->currentImage = image;
}