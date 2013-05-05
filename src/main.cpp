/*
	Libavcodec wrapper for Darkplaces by Timofeyev Pavel

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

#include "main.h"
#include "libavw.h" // provided by libav deps (contains version defines)

#ifdef _MSC_VER
#define __STDC_CONSTANT_MACROS
#endif

// libavcodec massive headers
#ifdef __cplusplus
extern "C"
{
#endif
	#include <avcodec.h>
	#include <avformat.h>
	#include <swscale.h>
#ifdef __cplusplus
}
#endif

// globals
bool              libav_initialized = false;
unsigned int      libav_codec_version = 0;
unsigned int      libav_format_version = 0;
unsigned int      libav_util_version = 0;
unsigned int      libav_swscale_version = 0;
avwCallbackPrint *libav_print = NULL;

// internal struct that holds video
typedef struct avwstream_s
{
	double           framerate;
	int64_t          numframes;
	unsigned int     framewidth;
	unsigned int     frameheight;
	int64_t          framenum;
	int              lasterror;

	// libavcodec reading context
	AVFormatContext *AV_FormatContext;
	AVIOContext     *AV_InputContext;
    int              AV_VideoStreamId;
	int              AV_AudioStreamId;
    AVCodecContext  *AV_CodecContext;
    AVCodec         *AV_Codec;
	AVFrame         *AV_InputFrame;
	AVFrame         *AV_OutputFrame;

	// I/O
	void              *file;
	avwCallbackIoRead *IO_Read;
	avwCallbackIoSeek *IO_Seek;
	avwCallbackIoSeekSize *IO_SeekSize;
}avwstream_t;

// scalers
#define LIBAVCODEC_SCALERS 10
int libav_scalers[LIBAVCODEC_SCALERS] =
{
	SWS_BILINEAR,
	SWS_BICUBIC,
	SWS_X,
	SWS_POINT,
	SWS_AREA,
	SWS_BICUBLIN,
	SWS_GAUSS,
	SWS_SINC,
	SWS_LANCZOS,
	SWS_SPLINE
};

// error codes
#define LIBAVW_ERROR_NONE                  0
#define LIBAVW_ERROR_DLL_VERSION_AVCODEC   1
#define LIBAVW_ERROR_DLL_VERSION_AVFORMAT  2
#define LIBAVW_ERROR_DLL_VERSION_AVUTIL    3
#define LIBAVW_ERROR_DLL_VERSION_SWSCALE   4
#define LIBAVW_ERROR_ALLOC_STREAM          5
#define LIBAVW_ERROR_ALLOC_INPUT_BUFFER    6
#define LIBAVW_ERROR_OPEN_INPUT            7
#define LIBAVW_ERROR_FIND_STREAM_INFO      8
#define LIBAVW_ERROR_FIND_VIDEO_STREAM     9
#define LIBAVW_ERROR_FIND_CODEC            10
#define LIBAVW_ERROR_OPEN_CODEC            11
#define LIBAVW_ERROR_ALLOC_INPUT_FRAME     12
#define LIBAVW_ERROR_ALLOC_OUTPUT_FRAME    13
#define LIBAVW_ERROR_BAD_FRAME_SIZE        14
#define LIBAVW_ERROR_BAD_IO_FUNCTIONS      15
#define LIBAVW_ERROR_DECODING_VIDEO_FRAME  16
#define LIBAVW_ERROR_LIB_NOT_INITIALIZED   17
#define LIBAVW_ERROR_NULL_STREAM           18
#define LIBAVW_ERROR_BAD_PIXEL_FORMAT      19
#define LIBAVW_ERROR_BAD_SCALER            20
#define LIBAVW_ERROR_CREATE_SCALE_CONTEXT  21
#define LIBAVW_ERROR_APPLYING_SCALE        22
#define LIBAVW_ERROR_TEST                  23

/*
=================================================================

 Video Stream

=================================================================
*/

// LibAvW_ResetStream
void LibAvW_ResetStream(avwstream_t *stream)
{
	stream->framerate = 0;
	stream->numframes = 0;
	stream->framewidth = 0;
	stream->frameheight = 0;
	stream->framenum = 0;
	stream->lasterror = LIBAVW_ERROR_NONE;
	stream->AV_VideoStreamId = -1;
	stream->AV_AudioStreamId = -1;
	stream->AV_Codec = NULL;
	// AV_InputFrame
	if (stream->AV_InputFrame)
		av_free(stream->AV_InputFrame);
	stream->AV_InputFrame = NULL;
	// AV_OutputFrame
	if (stream->AV_OutputFrame)
		av_free(stream->AV_OutputFrame);
	stream->AV_OutputFrame = NULL;
	// AV_CodecContext
	if (stream->AV_CodecContext)
		avcodec_close(stream->AV_CodecContext);
	stream->AV_CodecContext = NULL;
	// AV_InputContext
	if (stream->AV_InputContext)
	{
		av_free(stream->AV_InputContext);
		stream->AV_InputContext = NULL;
		if (stream->AV_FormatContext)
			stream->AV_FormatContext->pb = NULL;
	}
	// AV_FormatContext
	if (stream->AV_FormatContext)
		av_close_input_file(stream->AV_FormatContext);
	stream->AV_InputContext = NULL;
	stream->AV_FormatContext = NULL;
	// IO
	stream->file = NULL;
	stream->IO_Read = NULL;
	stream->IO_Seek = NULL;
	stream->IO_SeekSize = NULL;
}

// LibAvW_StreamGetVideoWidth
DLL_EXPORT int LibAvW_StreamGetVideoWidth(void *stream)
{
	avwstream_t *s;

	// check
	if (!libav_initialized)
		return 0;
	s = (avwstream_t *)stream;
	if (!s)
		return 0;

	s->lasterror = LIBAVW_ERROR_NONE;
	return s->framewidth;
}

// LibAvW_StreamGetVideoHeight
DLL_EXPORT int LibAvW_StreamGetVideoHeight(void *stream)
{
	avwstream_t *s;

	// check
	if (!libav_initialized)
		return 0;
	s = (avwstream_t *)stream;
	if (!s)
		return 0;

	s->lasterror = LIBAVW_ERROR_NONE;
	return s->frameheight;
}

// LibAvW_StreamGetFramerate
DLL_EXPORT double LibAvW_StreamGetFramerate(void *stream)
{
	avwstream_t *s;

	// check
	if (!libav_initialized)
		return 0;
	s = (avwstream_t *)stream;
	if (!s)
		return 0;

	s->lasterror = LIBAVW_ERROR_NONE;
	return s->framerate;
}

// LibAvW_StreamGetError
DLL_EXPORT int LibAvW_StreamGetError(void *stream)
{
	avwstream_t *s;

	// check
	if (!libav_initialized)
		return -1;
	s = (avwstream_t *)stream;
	if (!s)
		return -1;

	return s->lasterror;
}

// LibAvW_PlaySeekNextFrame
DLL_EXPORT int LibAvW_PlaySeekNextFrame(void *stream)
{
	avwstream_t *s;
	int frame_finished = 0;
	AVPacket pkt;

	// check
	if (!libav_initialized)
		return 0;
	s = (avwstream_t *)stream;
	if (!s)
		return 0;

	// read AV_InputFrame
	av_init_packet(&pkt);
	while(av_read_frame(s->AV_FormatContext, &pkt) >= 0)
	{
		// is this a packet from video stream
		if (pkt.stream_index == s->AV_VideoStreamId)
		{
			// decode into AV_InputFrame
			if (avcodec_decode_video2(s->AV_CodecContext, s->AV_InputFrame, &frame_finished, &pkt) < 0)
			{
				s->lasterror = LIBAVW_ERROR_DECODING_VIDEO_FRAME;
				av_free_packet(&pkt);
				return 0;
			}
			if (frame_finished)
			{
				// finished decoding a AV_InputFrame
				s->framenum++;
				s->lasterror = LIBAVW_ERROR_NONE;
				av_free_packet(&pkt);
				return 1;
			}
		}
	}
	av_free_packet(&pkt);

	// reached end of stream
	s->lasterror = LIBAVW_ERROR_NONE;
	return 0;
}

// LibAvW_PlayGetFrameImage
DLL_EXPORT int LibAvW_PlayGetFrameImage(void *stream, int pixel_format, void *imagedata, int imagewidth, int imageheight, int scaler)
{
	avwstream_t *s;
	PixelFormat avpixelformat;
	int avscaler;

	// check
	if (!libav_initialized)
		return 0;
	s = (avwstream_t *)stream;
	if (!s)
		return 0;

	// get pixel format
	if (pixel_format == LIBAVW_PIXEL_FORMAT_BGR)
		avpixelformat = PIX_FMT_BGR24;
	else if (pixel_format == LIBAVW_PIXEL_FORMAT_BGRA)
		avpixelformat = PIX_FMT_BGRA;
	else
	{
		s->lasterror = LIBAVW_ERROR_BAD_PIXEL_FORMAT;
		return 0;
	}

	// get scaler
	if (scaler >= LIBAVW_SCALER_BILINEAR && scaler <= LIBAVW_SCALER_SPLINE)
		avscaler = libav_scalers[scaler];
	else
	{
		s->lasterror = LIBAVW_ERROR_CREATE_SCALE_CONTEXT;
		return 0;
	}

	// get AV_InputFrame
	avpicture_fill((AVPicture *)s->AV_OutputFrame, (uint8_t *)imagedata, avpixelformat, imagewidth, imageheight);
	SwsContext *scale_context = sws_getCachedContext(NULL, s->AV_InputFrame->width, s->AV_InputFrame->height, (PixelFormat)s->AV_InputFrame->format, s->framewidth, s->frameheight, avpixelformat, avscaler, NULL, NULL, NULL); 
	if (!scale_context)
	{
		s->lasterror = LIBAVW_ERROR_BAD_SCALER;
		return 0;
	}
	if (!sws_scale(scale_context, s->AV_InputFrame->data, s->AV_InputFrame->linesize, 0, s->AV_InputFrame->height, s->AV_OutputFrame->data, s->AV_OutputFrame->linesize))
	{
		s->lasterror = LIBAVW_ERROR_APPLYING_SCALE;
		sws_freeContext(scale_context); 
		return 0;
	}

	// allright
	s->lasterror = LIBAVW_ERROR_NONE;
	sws_freeContext(scale_context); 
	return 1;
}

// IO wrapper
int LibAvW_FS_Read(void *opaque, uint8_t *buf, int buf_size)
{
	avwstream_t *s = (avwstream_t *)opaque;
	return s->IO_Read(s->file, buf, buf_size);
}

int64_t LibAvW_FS_Seek(void *opaque, int64_t pos, int whence)
{
	avwstream_t *s = (avwstream_t *)opaque;

	if (whence == AVSEEK_SIZE)
	{
		if (s->IO_SeekSize)
			return s->IO_SeekSize(s->file);
		return -1;
	}
	if (whence == AVSEEK_FORCE)
		return -1;

	return s->IO_Seek(s->file, pos, whence);
}

// LibAvW_PlayVideo
DLL_EXPORT int LibAvW_PlayVideo(void *stream, void *file, avwCallbackIoRead *IoRead, avwCallbackIoSeek *IoSeek, avwCallbackIoSeekSize *IoSeekSize)
{
	avwstream_t *s;
	unsigned char *inputbuf;
	unsigned int i;

	// check
	if (!libav_initialized)
		return 0;
	s = (avwstream_t *)stream;
	if (!s)
		return 0;

	// reset stream
	LibAvW_ResetStream(s);

	// set I/O functions
	s->file = file;
	s->IO_Read = IoRead;
	s->IO_Seek = IoSeek;
	s->IO_SeekSize = IoSeekSize;
	if (!s->file || !s->IO_Read || !s->IO_Seek)
	{
		LibAvW_ResetStream(s);
		s->lasterror = LIBAVW_ERROR_BAD_IO_FUNCTIONS;
		return 0;
	}

	// allocate input context
	#define AV_IOBUFSIZE 4096*16
	inputbuf = (unsigned char *)av_malloc(AV_IOBUFSIZE + FF_INPUT_BUFFER_PADDING_SIZE);
	memset(inputbuf, 0, AV_IOBUFSIZE + FF_INPUT_BUFFER_PADDING_SIZE);
	if (!inputbuf)
	{
		LibAvW_ResetStream(s);
		s->lasterror = LIBAVW_ERROR_ALLOC_INPUT_BUFFER;
		return 0;
	}
	s->AV_FormatContext = avformat_alloc_context();
	s->AV_InputContext = avio_alloc_context(inputbuf, AV_IOBUFSIZE, 0, s, LibAvW_FS_Read, NULL, LibAvW_FS_Seek);
	s->AV_FormatContext->pb = s->AV_InputContext;

	// open input
    if (avformat_open_input(&s->AV_FormatContext, "tmp", NULL, NULL) != 0)
	{
		LibAvW_ResetStream(s);
		s->lasterror = LIBAVW_ERROR_OPEN_INPUT;
		return 0;
	}

    // get stream information
#ifdef LIBAV95
	if (avformat_find_stream_info(s->AV_FormatContext, NULL) < 0)
#else
	 if (av_find_stream_info(s->AV_FormatContext) < 0)
#endif
	{
		LibAvW_ResetStream(s);
		s->lasterror = LIBAVW_ERROR_FIND_STREAM_INFO;
		return 0;
	}

    // find the first video stream
    s->AV_VideoStreamId = -1;
	s->AV_AudioStreamId = -1;
    for (i = 0; i < s->AV_FormatContext->nb_streams; i++)
	{
        if (s->AV_FormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			if (s->AV_VideoStreamId < 0)
				s->AV_VideoStreamId = i;
		if (s->AV_FormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
			if (s->AV_AudioStreamId < 0)
				s->AV_AudioStreamId = i;
	}
    if (s->AV_VideoStreamId == -1)
	{
		LibAvW_ResetStream(s);
		s->lasterror = LIBAVW_ERROR_FIND_VIDEO_STREAM;
		return 0;
	}
    s->AV_CodecContext = s->AV_FormatContext->streams[s->AV_VideoStreamId]->codec;

	// get decoder for video stream
    s->AV_Codec = avcodec_find_decoder(s->AV_CodecContext->codec_id);
    if (s->AV_Codec == NULL)
    {
		LibAvW_ResetStream(s);
		s->lasterror = LIBAVW_ERROR_FIND_CODEC;
		return 0;
	}

	// open AV_Codec
    // inform the AV_Codec that we can handle truncated bitstreams -- i.e.,
    // bitstreams where AV_InputFrame boundaries can fall in the middle of packets
    if (s->AV_Codec->capabilities & CODEC_CAP_TRUNCATED)
		s->AV_CodecContext->flags |= CODEC_FLAG_TRUNCATED;
#ifdef LIBAV95
	if (avcodec_open2(s->AV_CodecContext, s->AV_Codec, NULL) < 0)
#else
	if (avcodec_open(s->AV_CodecContext, s->AV_Codec) < 0)
#endif
    {
		LibAvW_ResetStream(s);
		s->lasterror = LIBAVW_ERROR_OPEN_CODEC;
		return 0;
	}

    // allocate UWV video AV_InputFrame
	// get required buffer size and allocate buffer
    // assign appropriate parts of buffer to image planes
    s->AV_InputFrame = avcodec_alloc_frame();
	if (!s->AV_InputFrame)
	{
		LibAvW_ResetStream(s);
		s->lasterror = LIBAVW_ERROR_ALLOC_INPUT_FRAME;
        return 0;
	}

	// allocate output RGBA AV_InputFrame
	s->AV_OutputFrame = avcodec_alloc_frame();
	if (!s->AV_OutputFrame)
	{
		LibAvW_ResetStream(s);
		s->lasterror = LIBAVW_ERROR_ALLOC_OUTPUT_FRAME;
        return 0;
	}

	// all right, start AV_Codec
	s->framenum = 0;
	s->framewidth = s->AV_CodecContext->width;
	s->frameheight = s->AV_CodecContext->height;
	s->numframes = s->AV_FormatContext->streams[s->AV_VideoStreamId]->nb_frames;

	// determine framerate
	s->framerate = (float)s->AV_FormatContext->streams[s->AV_VideoStreamId]->avg_frame_rate.num / (float)s->AV_FormatContext->streams[s->AV_VideoStreamId]->avg_frame_rate.den;
	if (s->framerate <= 1.0f || s->framerate > 100.0f)
	{
		s->framerate = (float)s->AV_FormatContext->streams[s->AV_VideoStreamId]->r_frame_rate.num / (float)s->AV_FormatContext->streams[s->AV_VideoStreamId]->r_frame_rate.den;
		if (s->framerate <= 1.0f || s->framerate > 100.0f)
			s->framerate = 15;
	}

	// final checks
	if (s->framewidth <= 0 || s->frameheight <= 0)
	{
		LibAvW_ResetStream(s);
		s->lasterror = LIBAVW_ERROR_BAD_FRAME_SIZE;
        return 0;
	}

	// allright
	s->lasterror = LIBAVW_ERROR_NONE;
	return 1;
}

// LibAvW_CreateStream
DLL_EXPORT int LibAvW_CreateStream(void **stream)
{
	avwstream_t *s;

	// check
	if (!libav_initialized)
		return LIBAVW_ERROR_LIB_NOT_INITIALIZED;
	// allocate
	s = (avwstream_t *)malloc(sizeof(avwstream_t));
	memset(s, 0, sizeof(avwstream_t));
	if (s == NULL)
		return LIBAVW_ERROR_ALLOC_STREAM;
	*stream = s;
	return LIBAVW_ERROR_NONE;
}

// LibAvW_RemoveStream
DLL_EXPORT void LibAvW_RemoveStream(void *stream)
{
	avwstream_t *s;

	if (!libav_initialized)
		return;
	s = (avwstream_t *)stream;
	LibAvW_ResetStream(s);
	free(s);
}

/*
=================================================================

 Generic functions

=================================================================
*/

// LibAvW_ErrorCallback
void sanitize(char *line)
{
    while(*line)
	{
        if (*line < 0x08 || (*line > 0x0D && *line < 0x20))
            *line='?';
        line++;
    }
}
void LibAvW_ErrorCallback(void* ptr, int level, const char* fmt, va_list vl)
{
	int print_prefix = 1;
    char line[1024];
	
	// we only want warning, error, fatal and panic
	if (!libav_print)
		return;
	if (level > AV_LOG_WARNING)
		return;
#ifdef LIBAV95
	vsprintf_s(line, sizeof(line), fmt, vl);
#else
	av_log_format_line(ptr, level, fmt, vl, line, sizeof(line), &print_prefix);
#endif
    sanitize(line);
	if (level == AV_LOG_WARNING)
		libav_print(LIBAVW_PRINT_WARNING, line);
	else if (level == AV_LOG_ERROR)
		libav_print(LIBAVW_PRINT_ERROR, line);
	else if (level == AV_LOG_FATAL)
		libav_print(LIBAVW_PRINT_FATAL, line);
	else
		libav_print(LIBAVW_PRINT_PANIC, line);
}

// LibAvW_ErrorString
DLL_EXPORT const char *LibAvW_ErrorString(int errorcode)
{
	if (errorcode == LIBAVW_ERROR_NONE)                 return "no error";
	if (errorcode == LIBAVW_ERROR_DLL_VERSION_AVCODEC)  return "unsupported avcodec DLL version";
	if (errorcode == LIBAVW_ERROR_DLL_VERSION_AVFORMAT) return "unsupported avformat DLL version";
	if (errorcode == LIBAVW_ERROR_DLL_VERSION_AVUTIL)   return "unsupported avutil DLL version";
	if (errorcode == LIBAVW_ERROR_DLL_VERSION_SWSCALE)  return "unsupported swscale DLL version";
	if (errorcode == LIBAVW_ERROR_ALLOC_STREAM)         return "unable to allocate memory for stream info structure";
	if (errorcode == LIBAVW_ERROR_ALLOC_INPUT_BUFFER)   return "unable to allocate input buffer";
	if (errorcode == LIBAVW_ERROR_OPEN_INPUT)           return "unable to inititialize input";
	if (errorcode == LIBAVW_ERROR_FIND_STREAM_INFO)     return "unable to find stream information";
	if (errorcode == LIBAVW_ERROR_FIND_VIDEO_STREAM)    return "unable to find video stream";
	if (errorcode == LIBAVW_ERROR_FIND_CODEC)           return "unable to find a AV_Codec for video stream";
	if (errorcode == LIBAVW_ERROR_OPEN_CODEC)           return "unable to open a AV_Codec for video stream";
	if (errorcode == LIBAVW_ERROR_ALLOC_INPUT_FRAME)    return "failed to allocate input AV_InputFrame";
	if (errorcode == LIBAVW_ERROR_ALLOC_OUTPUT_FRAME)   return "failed to allocate output AV_InputFrame";
	if (errorcode == LIBAVW_ERROR_BAD_FRAME_SIZE)       return "bad AV_InputFrame size";
	if (errorcode == LIBAVW_ERROR_BAD_IO_FUNCTIONS)     return "bad I/O functions";
	if (errorcode == LIBAVW_ERROR_DECODING_VIDEO_FRAME) return "error decoding AV_InputFrame";
	if (errorcode == LIBAVW_ERROR_LIB_NOT_INITIALIZED)  return "library not initialized";
	if (errorcode == LIBAVW_ERROR_NULL_STREAM)          return "null stream";
	if (errorcode == LIBAVW_ERROR_BAD_PIXEL_FORMAT)     return "bad pixel format";
	if (errorcode == LIBAVW_ERROR_BAD_SCALER)           return "bad scaler";
	if (errorcode == LIBAVW_ERROR_CREATE_SCALE_CONTEXT) return "unable to create scale context";
	if (errorcode == LIBAVW_ERROR_APPLYING_SCALE)       return "unable to apply scale";
	if (errorcode == LIBAVW_ERROR_TEST)                 return "debug break";
	return "unknown error code";
}

// LibAvW_Init
DLL_EXPORT int LibAvW_Init(avwCallbackPrint *printfunction)
{
	if (libav_initialized)
		return LIBAVW_ERROR_NONE;

	libav_codec_version  = avcodec_version();
	libav_format_version = avformat_version();
	libav_util_version   = avutil_version();
	libav_swscale_version  = swscale_version();

	// only can use version we were linked against
	if (libav_codec_version != LIBAVCODEC_VERSION_INT)
		return LIBAVW_ERROR_DLL_VERSION_AVCODEC;
	if (libav_format_version != LIBAVFORMAT_VERSION_INT)
		return LIBAVW_ERROR_DLL_VERSION_AVFORMAT;
	if (libav_util_version != LIBAVUTIL_VERSION_INT)
		return LIBAVW_ERROR_DLL_VERSION_AVUTIL;
	if (libav_swscale_version != LIBSWSCALE_VERSION_INT)
		return LIBAVW_ERROR_DLL_VERSION_SWSCALE;

	// allright, init libavcodec
	avcodec_register_all();
	av_register_all();
	av_log_set_callback(LibAvW_ErrorCallback);
	libav_initialized = true;
	libav_print = printfunction;
	return LIBAVW_ERROR_NONE;
}

// get a string containing libavcodec version wrapper was built for
DLL_EXPORT const char *LibAvW_AvcVersion(void)
{
	return LIBAVW_AVBUILDINFO;
}

// get wrapper version
DLL_EXPORT float LibAvW_Version(void)
{
	return (float)0.5;
}