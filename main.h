/*
	Libavcodec integration for Darkplaces by Timofeyev Pavel

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA
*/

#include <stdlib.h>
#ifdef _MSC_VERSION
#include "stdint.h"
#else
#include <stdint.h>
#endif

#define DLL_EXPORT extern "C" __declspec(dllexport)

// scaler type
#define LIBAVW_SCALER_BILINEAR   0
#define LIBAVW_SCALER_BICUBIC    1
#define LIBAVW_SCALER_X          2
#define LIBAVW_SCALER_POINT      3
#define LIBAVW_SCALER_AREA       4
#define LIBAVW_SCALER_BICUBLIN   5
#define LIBAVW_SCALER_GAUSS      6
#define LIBAVW_SCALER_SINC       7
#define LIBAVW_SCALER_LANCZOS    8
#define LIBAVW_SCALER_SPLINE     9

// output format
#define LIBAVW_PIXEL_FORMAT_BGR  0
#define LIBAVW_PIXEL_FORMAT_BGRA 1

// print levels
#define LIBAVW_PRINT_WARNING     1
#define LIBAVW_PRINT_ERROR       2
#define LIBAVW_PRINT_FATAL       3
#define LIBAVW_PRINT_PANIC       4

// exported callback functions:
typedef void    avwCallbackPrint(int, const char *);
typedef int     avwCallbackIoRead(void *, uint8_t *, int);
typedef int64_t avwCallbackIoSeek(void *, int64_t, int);
typedef int64_t avwCallbackIoSeekSize(void *);

// exported functions:

// init library, returns error code
DLL_EXPORT int LibAvW_Init(avwCallbackPrint *printfunction);

// get string for error code
DLL_EXPORT const char *LibAvW_ErrorString(int errorcode);

// get a string containing libavcodec version wrapper was built for
DLL_EXPORT const char *LibAvW_AvcVersion(void);

// get wrapper version
DLL_EXPORT float LibAvW_Version(void);

// create stream, returns error code
DLL_EXPORT int LibAvW_CreateStream(void **stream);

// flush and remove stream
DLL_EXPORT void LibAv_RemoveStream(void *stream);

// get video parameters of stream
DLL_EXPORT int LibAvW_StreamGetVideoWidth(void *stream);
DLL_EXPORT int LibAvW_StreamGetVideoHeight(void *stream);
DLL_EXPORT double LibAvW_StreamGetFramerate(void *stream);

// get last function errorcode from stream
DLL_EXPORT int LibAvW_StreamGetError(void *stream);

// simple API to play video
DLL_EXPORT int LibAvW_PlayVideo(void *stream, void *file, avwCallbackIoRead *IoRead, avwCallbackIoSeek *IoSeek, avwCallbackIoSeekSize *IoSeekSize);
DLL_EXPORT int LibAvW_PlaySeekNextFrame(void *stream);
DLL_EXPORT int LibAvW_PlayGetFrameImage(void *stream, int pixel_format, void *imagedata, int imagewidth, int imageheight, int scaler);