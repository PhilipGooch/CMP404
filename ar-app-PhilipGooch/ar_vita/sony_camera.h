/* SCE CONFIDENTIAL
 PlayStation(R)Vita Programmer Tool Runtime Library Release 02.100.071
 * Copyright (C) 2011 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#ifndef _SONY_CAMERA_H
#define _SONY_CAMERA_H

#include <camera.h>
#include <gxm.h>
#include <motion.h>

#include "sony_sample_framework_types.h"

struct CameraBuffer
{
	CameraBuffer* prev;
	CameraBuffer* next;
	void* buf;
	SceUID uID;
	SceCameraRead read;
	SceGxmTexture tex_yuv;

	int32_t volatile ref;

	SceMotionSensorState motion[SCE_MOTION_MAX_NUM_STATES];
};

CameraBuffer* _cameraReserveBuffer(void);
void _cameraCancelBuffer(CameraBuffer* buf);
void _cameraRegistBuffer(CameraBuffer* buf);
CameraBuffer* cameraGetLatestImage(void);
void cameraBufferAddRef(CameraBuffer* buf);
void cameraBufferRelease(CameraBuffer* buf);
	void cameraInitialize(void);
void cameraRelease(void);
Bool cameraUpdate(void);

CameraBuffer* sampleGetCameraImage(void);
SceMotionSensorState* sampleGetCameraMotion(CameraBuffer* buf);
SceCameraRead* sampleGetCameraBufferRead(CameraBuffer* buf);
void sampleLockCameraBuffer(CameraBuffer* buf);
void sampleUnlockCameraBuffer(CameraBuffer* bud);
void sampleUnlockCameraBuffer(CameraBuffer* buf);
void sampleSetCameraAutoControlHold(Bool hold);
void sampleOverrideCameraImage(CameraBuffer* image);

#endif // _SONY_CAMERA_H