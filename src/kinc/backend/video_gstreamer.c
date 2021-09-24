#include "video_gstreamer.h"

#if defined(KINC_VIDEO_GSTREAMER)

// this is required in kincfile.js
	// project.addDefine('KINC_VIDEO_GSTREAMER');

	// project.addIncludeDir('/usr/include/gstreamer-1.0');
	// project.addIncludeDir('/usr/include/glib-2.0');
	// project.addIncludeDir('/usr/lib/x86_64-linux-gnu/glib-2.0/include');

	// project.addLib('gstbase-1.0');
	// project.addLib('gstreamer-1.0');
	// project.addLib('gstvideo-1.0');
	// project.addLib('gobject-2.0');
	// project.addLib('glib-2.0');


#include <gst/gst.h>
#include <gst/video/video-info.h>

#include <kinc/backend/graphics4/ogl.h>
#include <kinc/log.h>
#include <kinc/system.h>

static bool gstreamer_initialized = false;

static void on_gst_buffer( GstElement *element, GstBuffer *buf, GstPad *pad, kinc_video_impl_t *ctx );
static void on_new_pad_handler( GstElement *element, GstPad *pad, kinc_video_impl_t *ctx );
static void on_new_frame( kinc_video_impl_t *ctx, void *buf );
static void message_handler( GstBus *bus, GstMessage *msg, kinc_video_impl_t *ctx );
static void outgoing_buffer_thread( kinc_video_impl_t *ctx );
static bool change_state( kinc_video_t *video, GstState state );

void kinc_video_init(kinc_video_t *video, const char *filename) {
	if (!gstreamer_initialized) {
		gst_init(NULL, NULL);
		gstreamer_initialized = true;
	}

	kinc_video_impl_t *ctx = &video->impl;
	ctx->pipeline = gst_pipeline_new(NULL);
	ctx->source = gst_element_factory_make("filesrc", "filesrc");
	ctx->decodebin = gst_element_factory_make("decodebin", "decodebin");
	ctx->videosink = gst_element_factory_make("fakesink", "videosink");
	ctx->bus = NULL;

	ctx->location = NULL;

	ctx->color_format = VIDEO_COLOR_FORMAT_Unknown;
	ctx->width = 0;
	ctx->height = 0;
	ctx->stride = 0;
	ctx->video_info_valid = false;

	ctx->buf = NULL;
	ctx->finished = false;
	ctx->paused = true;
	ctx->looping = false;

	// ctx->audiosink = gst_element_factory_make("alsasink", "audiosink");
	// ctx->audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
	// ctx->audioqueue = gst_element_factory_make("queue", "audioqueue");

	if (!ctx->pipeline || !ctx->source || !ctx->decodebin || !ctx->videosink
		// ||	!ctx->audiosink || !ctx->audioconvert || !ctx->audioqueue
	) {
		g_critical("one of the GStreamer decoding elements is missing");
		return;
	}

	g_object_set(G_OBJECT(ctx->source), "location", filename, NULL);

	gst_bin_add_many(
		GST_BIN(ctx->pipeline),
		ctx->source,
		ctx->decodebin,
		ctx->videosink,
		NULL, // ctx->audiosink,
		NULL, // ctx->audioconvert,
		NULL, // ctx->audioqueue,
		NULL /*videoqueue,*/
	);

	g_signal_connect(ctx->decodebin, "pad-added", G_CALLBACK(on_new_pad_handler), ctx);

	gst_element_link(ctx->source, ctx->decodebin);
	// gst_element_link(ctx->audioqueue, ctx->audioconvert);
	// gst_element_link(ctx->audioconvert, ctx->audiosink);

	ctx->bus = gst_element_get_bus(ctx->pipeline);
	gst_element_set_state(ctx->pipeline, GST_STATE_PAUSED);

	kinc_g4_texture_init(&ctx->texture, 16, 16, KINC_IMAGE_FORMAT_GREY8); // (DK) format doesn't actually matter as long as glGenTextures() is called?
}

void kinc_video_destroy(kinc_video_t *video) {
	kinc_video_impl_t *ctx = &video->impl;

	if (ctx->bus) {
		gst_object_unref(ctx->bus);
		ctx->bus = NULL;
	}

	if (ctx->pipeline) {
		gst_element_set_state(ctx->pipeline, GST_STATE_NULL);
		gst_object_unref(ctx->pipeline);
		ctx->pipeline = NULL;
		kinc_g4_texture_destroy(&ctx->texture);
	}
}

static bool change_state( kinc_video_t *video, GstState state ) {
	kinc_video_impl_t *ctx = &video->impl;
	GstStateChangeReturn ret = gst_element_set_state(ctx->pipeline, state);

	if (ret == GST_STATE_CHANGE_FAILURE) {
		kinc_log(KINC_LOG_LEVEL_ERROR, "failed to change the pipeline state");
		return false;
	}

	return true;
}

void kinc_video_play(kinc_video_t *video, bool loop) {
	if (change_state(video, GST_STATE_PLAYING)) {
		video->impl.paused = false;
		video->impl.looping = loop;
	}
}

void kinc_video_pause(kinc_video_t *video) {
	if (change_state(video, GST_STATE_PAUSED)) {
		video->impl.paused = true;
	}
}

void kinc_video_stop(kinc_video_t *video) {
	if (change_state(video, GST_STATE_NULL)) {
		video->impl.finished = true;
	}
}

int kinc_video_width(kinc_video_t *video) {
	return video->impl.width;
}

int kinc_video_height(kinc_video_t *video) {
	return video->impl.height;
}

kinc_g4_texture_t *kinc_video_current_image(kinc_video_t *video) {
	return &video->impl.texture;
}

#define NANOSECONDS_TO_SECONDS 1000000000

double kinc_video_duration(kinc_video_t *video) {
	gint64 nanoseconds = 0;
	gst_element_query_duration(video->impl.pipeline, GST_FORMAT_TIME, &nanoseconds);
	return nanoseconds / NANOSECONDS_TO_SECONDS;
}

double kinc_video_position(kinc_video_t *video) {
	gint64 nanoseconds = 0;
	gst_element_query_position(video->impl.pipeline, GST_FORMAT_TIME, &nanoseconds);
	return nanoseconds / NANOSECONDS_TO_SECONDS;
}

bool kinc_video_finished(kinc_video_t *video) {
	return video->impl.finished;
}

bool kinc_video_paused(kinc_video_t *video) {
	return video->impl.paused;
}

void kinc_video_update(kinc_video_t *video, double time) {
	kinc_video_impl_t *ctx = &video->impl;

	GstMessage *message = NULL;

	if (ctx->bus) {
		while (true) {
			message = gst_bus_pop(ctx->bus);

			if (message) {
				message_handler(ctx->bus, message, ctx);
			} else {
				break;
			}
		}
	}

	if (ctx->video_info_valid && ctx->buf) {
		on_new_frame(ctx, ctx->buf);
		ctx->buf = NULL;
	}
}

void kinc_internal_video_sound_stream_init(kinc_internal_video_sound_stream_t *stream, int channel_count, int frequency) {

}

void kinc_internal_video_sound_stream_destroy(kinc_internal_video_sound_stream_t *stream) {

}

void kinc_internal_video_sound_stream_insert_data(kinc_internal_video_sound_stream_t *stream, float *data, int sample_count) {

}

float kinc_internal_video_sound_stream_next_sample(kinc_internal_video_sound_stream_t *stream) {
	return 0.0f;
}

bool kinc_internal_video_sound_stream_ended(kinc_internal_video_sound_stream_t *stream) {
	return true;
}

static void
on_new_pad_handler( GstElement *element, GstPad *pad, kinc_video_impl_t *ctx ) {
	GstPad *sinkpad = NULL;
	GstCaps *caps = gst_pad_query_caps(pad, NULL);
	GstStructure *str = gst_caps_get_structure(caps, 0);

	if (g_strrstr(gst_structure_get_name(str), "video")) {
		sinkpad = gst_element_get_static_pad(ctx->videosink, "sink");

		g_object_set(G_OBJECT(ctx->videosink), "sync", true, "signal-handoffs", true, NULL);
		g_signal_connect(ctx->videosink, "preroll-handoff", G_CALLBACK(on_gst_buffer), ctx);
		g_signal_connect(ctx->videosink, "handoff", G_CALLBACK(on_gst_buffer), ctx);
	} // TODO (DK) audio is disabled for now
	// else {
	// 	sinkpad = gst_element_get_static_pad(ctx->audioqueue, "sink");
	// }

	if (sinkpad) {
		gst_caps_unref(caps);
		gst_pad_link(pad, sinkpad);
		gst_object_unref(sinkpad);
	}
}

static void
on_gst_buffer( GstElement *element, GstBuffer *buf, GstPad *pad, kinc_video_impl_t *ctx ) {
	if (!ctx->video_info_valid) {
		kinc_log(KINC_LOG_LEVEL_INFO, "received first frame of video");

		GstCaps *caps = gst_pad_get_current_caps(pad);

		if (caps) {
			GstStructure *structure = gst_caps_get_structure(caps, 0);
			gst_structure_get_int(structure, "width", &(ctx->width));
			gst_structure_get_int(structure, "height", &(ctx->height));

			GstVideoInfo info;
			gst_video_info_from_caps(&info, caps);

			ctx->stride = info.stride[0];

			kinc_g4_texture_t *tex = &ctx->texture;
			tex->tex_width = ctx->stride; // ctx->width;
			tex->tex_height = ctx->height;
			tex->tex_depth = 0;

			ctx->color_format = impl_discover_color_format(buf, caps);
			ctx->video_info_valid = true;
		} else {
			kinc_log(KINC_LOG_LEVEL_ERROR, "on_gst_buffer() - could not get caps for pad");
		}
	} else {
		ctx->buf = buf;
	}
}

static void
on_new_frame( kinc_video_impl_t *ctx, void *buf ) {
	GstMapInfo info;
	bool loaded = false;

	switch (ctx->color_format) {
		case VIDEO_COLOR_FORMAT_I420:
		case VIDEO_COLOR_FORMAT_NV12:
			if (gst_buffer_map(buf, &info, GST_MAP_READ)) {
				int target = GL_TEXTURE_2D;
				kinc_ticks_t start = kinc_timestamp();

				glBindTexture(target, ctx->texture.impl.texture);
				glCheckErrors();
				glTexImage2D(target, 0, GL_R8, ctx->stride, ctx->height * 1.5, 0, GL_RED, GL_UNSIGNED_BYTE, info.data);
				glCheckErrors();
				gst_buffer_unmap(buf, &info);
				loaded = true;
			} else {
				kinc_log(KINC_LOG_LEVEL_WARNING, "gst_buffer_map failed");
			}

			break;

		// case VIDEO_COLOR_FORMAT_UYVY:
		// 	if (gst_buffer_map(buf, &info, GST_MAP_READ)) {
		// 		glBindTexture(GL_TEXTURE_2D, ctx->texture.impl.texture);
		// 		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, ctx->width * 2, ctx->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, info.data);
		// 		// yuvImage = QImage(info.data, m_vidTextures[vidIx].width*2, m_vidTextures[vidIx].height, QImage::Format_Indexed8);
		// 		gst_buffer_unmap(buf, &info);
		// 		loaded = true;
		// 	}

		// 	break;
		default:
			kinc_log(KINC_LOG_LEVEL_WARNING, "unhandled video_color_format %i", ctx->color_format);
			break;
	}

	if (!loaded) {
		kinc_log(KINC_LOG_LEVEL_INFO, "failed to upload new frame");
	}
}

static void
message_handler( GstBus *bus, GstMessage *msg, kinc_video_impl_t *ctx ) {
	gchar *debug = NULL;
	GError *err = NULL;

	switch (GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_EOS:
			if (ctx->looping) {
				gst_element_seek_simple(ctx->pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 0);
			} else {
				ctx->finished = true;
			}
			// kinc_video_stop(ctx); // TODO (DK)
			kinc_log(KINC_LOG_LEVEL_INFO, "video fininshed: GST_MESSAGE_EOS");
			break;

		case GST_MESSAGE_ERROR:
			gst_message_parse_error(msg, &err, &debug);
			kinc_log(KINC_LOG_LEVEL_ERROR, err->message);
			g_error_free(err);
			break;

		default:
			kinc_log(KINC_LOG_LEVEL_INFO, "ignoring bus message %i", GST_MESSAGE_TYPE(msg));
	}
}

#endif
