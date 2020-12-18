#include "sony_sample_framework.h"
#include "sony_tracking.h"

#include <display.h>
#include <string.h>

SampleState s_sampleState;


extern void (*custom_display_callback)(const void *callbackData);

void ARAppDisplayCallback(const void* data)
{
	SampleState* const s = &s_sampleState;
	myEventRaise(&s->app.render.flip_evt);
}


void sampleSetViewMatrix(gef::Matrix44 const* mat){
	SampleState* const s = &s_sampleState;
	AppData* dat = s->app.update.current;
	ASSERT(dat != NULL);
	dat->world.viewMat = *mat;
}
void sampleGetViewMatrix(gef::Matrix44* mat){
	SampleState* const s = &s_sampleState;
	AppData* dat = s->app.update.current;
	ASSERT(dat != NULL);
	*mat = dat->world.viewMat;
}
void sampleGetProjMatrix(gef::Matrix44* mat){
	SampleState* const s = &s_sampleState;
	AppData* dat = s->app.update.current;
	ASSERT(dat != NULL);
	*mat = dat->world.projMat;
}
void sampleSetTransform(int idx, gef::Matrix44* mat){
	SampleState* const s = &s_sampleState;
	AppData* dat = s->app.update.current;
	ASSERT(dat != NULL);
	dat->transforms.transform[idx] = *mat;
}

void sampleGetTransform(int idx, gef::Matrix44* mat) {
	SampleState* const s = &s_sampleState;
	AppData* dat = s->app.update.current;
	ASSERT(dat != NULL);
	*mat = dat->transforms.transform[idx];
}

void sampleSetMarkerData(int idx, bool lost, SceUInt32 target){
	SampleState* const s = &s_sampleState;
	AppData* dat = s->app.update.current;
	ASSERT(dat != NULL);
	dat->markers.marker[idx].lost = lost;
	dat->markers.marker[idx].target = target;
}

bool sampleIsMarkerFound(int idx)
{
	SampleState* const s = &s_sampleState;
	AppData* dat = s->app.update.current;
	ASSERT(dat != NULL);
	return !dat->markers.marker[idx].lost;
}

static void _appDataInitDat(AppData* dat){
	//SampleState* const s = &s_sampleState;
	{
		gef::Matrix44 pm;
		gef::Matrix44 sm;

		pm.PerspectiveFovGL(PROJECTION_FOV, (float)PROJECTION_WIDTH / (float)PROJECTION_HEIGHT, PROJECTION_ZNEAR, PROJECTION_ZFAR);
		sm.Scale(gef::Vector4(VIEW_SCALE_H, VIEW_SCALE_V, 1.0f));
		dat->world.projMat = pm * sm;
		dat->world.viewMat.SetIdentity();
	}

	for(int i=0;i<6;i++)
	{
		dat->transforms.transform[i].SetIdentity();
		dat->markers.marker[i].lost = 1;
		dat->markers.marker[i].target = -1;
	}
}

static void _appDataInit(void){
	SampleState* const s = &s_sampleState;
	myMutexCreate(&s->app.data.mtx);
	UInt i;
	{
		s->app.data.head = &s->app.data.buf[0];
		s->app.data.buf[0].prev = NULL;
		for(i = 1; i < bulkof(s->app.data.buf); i++){
			s->app.data.buf[i].prev = &s->app.data.buf[i - 1];
			s->app.data.buf[i - 1].next = &s->app.data.buf[i];
		}
		s->app.data.buf[i - 1].next = NULL;
		s->app.data.tail = &s->app.data.buf[i - 1];
	}

	for(i = 0; i < bulkof(s->app.data.buf); i++){
		_appDataInitDat(&s->app.data.buf[i]);
	}
}


static void _appDataRelease(void){
	SampleState* const s = &s_sampleState;
//	UInt i;
//	for(i = 0; i < bulkof(s->app.data.buf); i++){
//		_appDataReleaseDat(&s->app.data.buf[i]);
//	}
	myMutexRelease(&s->app.data.mtx);
}

static AppData* _appDataReserve(void){
	SampleState* const s = &s_sampleState;
	AppData* dat;
	myMutexLock(&s->app.data.mtx);
	{
		dat = s->app.data.tail;
		while(dat != NULL){
			if(dat->ref == 0){
				break;
			}
			dat = dat->prev;
		}
		if(dat != NULL){
			if(dat->next != NULL){
				dat->next->prev = dat->prev;
			}
			if(dat->prev != NULL){
				dat->prev->next = dat->next;
			}
			if(dat == s->app.data.head){
				s->app.data.head = dat->next;
			}
			if(dat == s->app.data.tail){
				s->app.data.tail = dat->prev;
			}
			dat->next = NULL;
			dat->prev = NULL;
		}
	}
	myMutexUnlock(&s->app.data.mtx);
	return dat;
}

static void _appDataRegist(AppData* dat){
	SampleState* const s = &s_sampleState;
	myMutexLock(&s->app.data.mtx);
	{
		ASSERT(dat != NULL);
		ASSERT(dat->prev == NULL);
		ASSERT(dat->next == NULL);
		ASSERT(s->app.data.head != NULL);
		ASSERT(s->app.data.head->prev == NULL);
		ASSERT(s->app.data.tail != NULL);
		ASSERT(s->app.data.tail->next == NULL);
		dat->next = s->app.data.head;
		s->app.data.head->prev = dat;
		s->app.data.head = dat;
	}
	myMutexUnlock(&s->app.data.mtx);
}

static AppData* _appDataLockHead(void){
	SampleState* const s = &s_sampleState;
	AppData* dat;
	myMutexLock(&s->app.data.mtx);
	{
		dat = s->app.data.head;
		ASSERT(dat->ref == 0);
		dat->ref++;
	}
	myMutexUnlock(&s->app.data.mtx);
	return dat;
}
static void _appDataUnlockData(AppData* dat){
	SampleState* const s = &s_sampleState;
	myMutexLock(&s->app.data.mtx);
	{
		ASSERT(dat->ref > 0);
		dat->ref--;
	}
	myMutexUnlock(&s->app.data.mtx);
}

static int32_t _vsyncThread(uint32_t argSize, void* pArgBlock){
	SampleState* const s = &s_sampleState;
	UNUSED(argSize);
	UNUSED(pArgBlock);

	DEBUG_PRINTF("%s: start\n", __FUNCTION__);

	for(;;){
		SCECHK(sceDisplayWaitVblankStart());

		if(s->vsync.thread.exit != FALSE){
			break;
		}

#if 1
		s->app.debug.frame++;
#endif
		myEventRaise(&s->vsync.thread.evt);
	}

	DEBUG_PRINTF("%s: exit\n", __FUNCTION__);

	return 0;
}

void vsyncInitialize(void){
	SampleState* const s = &s_sampleState;
	myEventCreate(&s->vsync.thread.evt);
	{
		s->vsync.thread.exit = FALSE;
		s->vsync.thread.uID = sceKernelCreateThread("vsync", _vsyncThread, VSYNC_THREAD_PRIORITY, VSYNC_THREAD_STACK_SIZE, 0, VSYNC_THREAD_AFFINITY_MASK, NULL);
		ASSERT(s->vsync.thread.uID >= SCE_OK);

		SCECHK(sceKernelStartThread(s->vsync.thread.uID, 0, NULL));
	}
}

void vsyncRelease(void){
	SampleState* const s = &s_sampleState;
	if(s->vsync.thread.uID >= 0){
		s->vsync.thread.exit = TRUE;
		SCECHK(sceKernelWaitThreadEnd(s->vsync.thread.uID, NULL, NULL));
		SCECHK(sceKernelDeleteThread(s->vsync.thread.uID));
		s->vsync.thread.uID = -1;
	}
	myEventRelease(&s->vsync.thread.evt);
}

void vsyncWait(void){
	SampleState* const s = &s_sampleState;
	myEventWait(&s->vsync.thread.evt, INFINITE);
}

void sampleInitialize(){
	SampleState* const s = &s_sampleState;
	memset(&s_sampleState, 0, sizeof(s_sampleState));


	vsyncInitialize();
	cameraInitialize();

	_appDataInit();
	custom_display_callback = ARAppDisplayCallback;
	{
		myEventCreate(&s->app.render.flip_evt);
		myEventRaise(&s->app.render.flip_evt);
	}
}

void sampleRelease(){

	SampleState* const s = &s_sampleState;
	myEventRelease(&s->app.render.flip_evt);
	custom_display_callback = NULL;
	_appDataRelease();

	cameraRelease();
	vsyncRelease();

	memset(&s_sampleState, 0, sizeof(s_sampleState));
}

AppData* sampleUpdateBegin(){
	SampleState* const s = &s_sampleState;
	AppData* dat;

	vsyncWait();

	dat = _appDataReserve();
	ASSERT(dat != NULL);

	if(dat->currentImage != NULL){
		cameraBufferRelease(dat->currentImage);
	}
	cameraUpdate();
	dat->currentImage = cameraGetLatestImage();

	s->app.update.current = dat;

	return dat;
}

void sampleUpdateEnd(AppData* dat){
	SampleState* const s = &s_sampleState;

	s->app.update.current = NULL;
	_appDataRegist(dat);
}

AppData* sampleRenderBegin() {
	SampleState* const s = &s_sampleState;
	AppData* dat;

	myEventWait(&s->app.render.flip_evt, INFINITE);

	if(s->app.render.data != NULL){
		_appDataUnlockData(s->app.render.data);
	}
	s->app.render.data = _appDataLockHead();
	dat = s->app.render.data;

	return dat;
}

void sampleRenderEnd() {
}