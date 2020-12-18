/* SCE CONFIDENTIAL
 PlayStation(R)Vita Programmer Tool Runtime Library Release 02.100.071
 * Copyright (C) 2011 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#ifndef _SONY_TRACKING_H
#define _SONY_TRACKING_H

void smartInitialize(void);
void smartRelease(void);
void smartUpdate(CameraBuffer* image);
void smartTrackingReset();

#endif // _SONY_TRACKING_H