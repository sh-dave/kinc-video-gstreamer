#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if defined(KINC_VIDEO_GSTREAMER)

#include <kinc/graphics4/texture.h>

typedef struct _GstElement GstElement;
typedef struct _GstBus GstBus;
typedef struct _GstBuffer GstBuffer;

#define VIDEO_COLOR_FORMAT_FOUR_CC(a,b,c,d) \
		((unsigned long) ((a) | (b)<<8 | (c)<<16 | (d)<<24))

typedef enum video_color_format {
	VIDEO_COLOR_FORMAT_NV12 = VIDEO_COLOR_FORMAT_FOUR_CC('N', 'V', '1', '2'),
	VIDEO_COLOR_FORMAT_I420 = VIDEO_COLOR_FORMAT_FOUR_CC('I', '4', '2', '0'),
	VIDEO_COLOR_FORMAT_IYUV = VIDEO_COLOR_FORMAT_FOUR_CC('I', 'Y', 'U', 'V'),
	VIDEO_COLOR_FORMAT_YV12 = VIDEO_COLOR_FORMAT_FOUR_CC('Y', 'V', '1', '2'),
	VIDEO_COLOR_FORMAT_YUYV = VIDEO_COLOR_FORMAT_FOUR_CC('Y', 'U', 'Y', 'V'),
	VIDEO_COLOR_FORMAT_YUY2 = VIDEO_COLOR_FORMAT_FOUR_CC('Y', 'U', 'Y', '2'),
	VIDEO_COLOR_FORMAT_V422 = VIDEO_COLOR_FORMAT_FOUR_CC('V', '4', '2', '2'),
	VIDEO_COLOR_FORMAT_YUNV = VIDEO_COLOR_FORMAT_FOUR_CC('Y', 'U', 'N', 'V'),
	VIDEO_COLOR_FORMAT_UYVY = VIDEO_COLOR_FORMAT_FOUR_CC('U', 'Y', 'V', 'Y'),
	VIDEO_COLOR_FORMAT_Y422 = VIDEO_COLOR_FORMAT_FOUR_CC('Y', '4', '2', '2'),
	VIDEO_COLOR_FORMAT_UYNV = VIDEO_COLOR_FORMAT_FOUR_CC('U', 'Y', 'N', 'V'),

	VIDEO_COLOR_FORMAT_RGB888 = VIDEO_COLOR_FORMAT_FOUR_CC('R', 'G', 'B', '8'),
	VIDEO_COLOR_FORMAT_BGR888,
	VIDEO_COLOR_FORMAT_ARGB8888,
	VIDEO_COLOR_FORMAT_BGRA8888,

	VIDEO_COLOR_FORMAT_Unknown,
} video_color_format_t;

#undef VIDEO_COLOR_FORMAT_FOUR_CC

typedef struct {
	GstElement *pipeline;
	GstElement *source;
	GstElement *decodebin;
	GstElement *videosink;

	// GstElement *audiosink;
	// GstElement *audioconvert;
	// GstElement *audioqueue;

	GstBus *bus;

	const char *location;

	video_color_format_t color_format;
	int width;
	int height;
	int stride;
	int video_info_valid;

	GstBuffer *buf;
	kinc_g4_texture_t texture;

	bool finished;
	bool paused;
	bool looping;
} kinc_video_impl_t;

typedef struct kinc_internal_video_sound_stream {
	int nothing;
} kinc_internal_video_sound_stream_t;

typedef struct _GstBuffer GstBuffer;
typedef struct _GstCaps GstCaps;

video_color_format_t impl_discover_color_format( GstBuffer *buf, GstCaps *caps );

void kinc_internal_video_sound_stream_init(kinc_internal_video_sound_stream_t *stream, int channel_count, int frequency);

void kinc_internal_video_sound_stream_destroy(kinc_internal_video_sound_stream_t *stream);

void kinc_internal_video_sound_stream_insert_data(kinc_internal_video_sound_stream_t *stream, float *data, int sample_count);

float kinc_internal_video_sound_stream_next_sample(kinc_internal_video_sound_stream_t *stream);

bool kinc_internal_video_sound_stream_ended(kinc_internal_video_sound_stream_t *stream);

#include <kinc/video.h>

#ifdef __cplusplus
}
#endif

#endif /* #if defined(KINC_VIDEO_GSTREAMER) */
