/* scrninfo.h - define ScreenInfo structure */

#pragma once

#include "text_info.h"

struct ScreenInfo
{
	void *ScreenMatrix;
	struct text_info TextInfo;
};

/* end of scrninfo.h */
