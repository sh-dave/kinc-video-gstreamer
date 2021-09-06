#include <kinc/video.h>

#if defined(KINC_VIDEO_GSTREAMER)

#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <kinc/log.h>

video_color_format_t
impl_discover_color_format( GstBuffer *buf, GstCaps *caps ) {
	gint depth;
	gint bpp;
	gint red_mask;
	gint green_mask;
	gint blue_mask;
	gint alpha_mask;
	video_color_format_t ret = VIDEO_COLOR_FORMAT_Unknown;

	//GstMapInfo info;
	//if (gst_buffer_map(buf, &info, GST_MAP_READ)) {
	gchar *tmp = gst_caps_to_string(caps);
	kinc_log(KINC_LOG_LEVEL_INFO, "%s", tmp);
	//  g_free(pTmp);
	//}

	GstStructure *structure = gst_caps_get_structure(caps, 0);

	if (gst_structure_has_name(structure, "video/x-raw-rgb")) {
		gst_structure_get_int(structure, "bpp", &bpp);
		gst_structure_get_int(structure, "depth", &depth);
		gst_structure_get_int(structure, "red_mask", &red_mask);
		gst_structure_get_int(structure, "green_mask", &green_mask);
		gst_structure_get_int(structure, "blue_mask", &blue_mask);

		switch (depth) {
			case 24:
				if (red_mask == 0x00ff0000 && green_mask == 0x0000ff00 && blue_mask == 0x000000ff) {
					kinc_log(KINC_LOG_LEVEL_INFO, "color format is RGB");
					ret = VIDEO_COLOR_FORMAT_RGB888;
				} else if (red_mask == 0x000000ff && green_mask == 0x0000ff00 && blue_mask == 0x00ff0000) {
					kinc_log(KINC_LOG_LEVEL_INFO, "color format is BGR");
					ret = VIDEO_COLOR_FORMAT_BGR888;
				} else {
					kinc_log(KINC_LOG_LEVEL_ERROR, "unhandled 24 bit RGB color format");
				}

				break;

			case 32:
				gst_structure_get_int(structure, "alpha_mask", &alpha_mask);

				if (red_mask == 0xff000000 && green_mask == 0x00ff0000 && blue_mask == 0x0000ff00) {
					kinc_log(KINC_LOG_LEVEL_INFO, "color format is RGBA");
					ret = VIDEO_COLOR_FORMAT_ARGB8888;
				} else if (red_mask == 0x00ff0000 && green_mask == 0x0000ff00 && blue_mask == 0x000000ff) {
					kinc_log(KINC_LOG_LEVEL_INFO, "color format is BGRA");
					ret = VIDEO_COLOR_FORMAT_BGRA8888;
				} else {
					kinc_log(KINC_LOG_LEVEL_ERROR, "unhandled 32 bit RGB color format");
				}

				break;

			default:
				kinc_log(KINC_LOG_LEVEL_ERROR, "unhandled RGB-format of depth %d", depth);
				break;
		}
	} else if (gst_structure_has_name(structure, "video/x-raw")) {
		GstVideoInfo info;
		gst_video_info_from_caps(&info, caps);
		guint32 fourcc = info.finfo->format;
		kinc_log(KINC_LOG_LEVEL_INFO, "%i", fourcc);

		//  gst_structure_get_fourcc(pStructure, "format", &uiFourCC);

		switch (fourcc) {
			case GST_VIDEO_FORMAT_NV12:
			case GST_MAKE_FOURCC('N', 'V', '1', '2'):
				kinc_log(KINC_LOG_LEVEL_INFO, "NV12 (0x%X)", fourcc);
				ret = VIDEO_COLOR_FORMAT_NV12;
				break;

			case GST_VIDEO_FORMAT_I420:
			case GST_MAKE_FOURCC('I', '4', '2', '0'):
				kinc_log(KINC_LOG_LEVEL_INFO, "I420 (0x%X)", fourcc);
				ret = VIDEO_COLOR_FORMAT_I420;
				break;

			case GST_VIDEO_FORMAT_IYU1:
			case GST_VIDEO_FORMAT_IYU2:
			case GST_MAKE_FOURCC('I', 'Y', 'U', 'V'):
				kinc_log(KINC_LOG_LEVEL_INFO, "IYUV (0x%X)", fourcc);
				ret = VIDEO_COLOR_FORMAT_IYUV;
				break;

			case GST_VIDEO_FORMAT_YV12:
			case GST_MAKE_FOURCC('Y', 'V', '1', '2'):
				kinc_log(KINC_LOG_LEVEL_INFO, "YV12 (0x%X)", fourcc);
				ret = VIDEO_COLOR_FORMAT_YV12;
				break;

			case GST_MAKE_FOURCC('Y', 'U', 'Y', 'V'):
				kinc_log(KINC_LOG_LEVEL_INFO, "YUYV (0x%X)", fourcc);
				ret = VIDEO_COLOR_FORMAT_YUYV;
				break;

			case GST_VIDEO_FORMAT_YUY2:
			case GST_MAKE_FOURCC('Y', 'U', 'Y', '2'):
				kinc_log(KINC_LOG_LEVEL_INFO, "YUY2 (0x%X)", fourcc);
				ret = VIDEO_COLOR_FORMAT_YUY2;
				break;

			case GST_MAKE_FOURCC('V', '4', '2', '2'):
				kinc_log(KINC_LOG_LEVEL_INFO, "V422 (0x%X)", fourcc);
				ret = VIDEO_COLOR_FORMAT_V422;
				break;

			case GST_MAKE_FOURCC('Y', 'U', 'N', 'V'):
				kinc_log(KINC_LOG_LEVEL_INFO, "YUNV (0x%X)", fourcc);
				ret = VIDEO_COLOR_FORMAT_YUNV;
				break;

			case GST_MAKE_FOURCC('U', 'Y', 'V', 'Y'):
				kinc_log(KINC_LOG_LEVEL_INFO, "UYVY (0x%X)", fourcc);
				ret = VIDEO_COLOR_FORMAT_UYVY;
				break;

			case GST_VIDEO_FORMAT_Y42B:
			case GST_MAKE_FOURCC('Y', '4', '2', '2'):
				kinc_log(KINC_LOG_LEVEL_INFO, "Y422 (0x%X)", fourcc);
				ret = VIDEO_COLOR_FORMAT_Y422;
				break;

			case GST_MAKE_FOURCC('U', 'Y', 'N', 'V'):
				kinc_log(KINC_LOG_LEVEL_INFO, "UYNV (0x%X)", fourcc);
				ret = VIDEO_COLOR_FORMAT_YUNV;
				break;

			default:
				kinc_log(KINC_LOG_LEVEL_ERROR, "unhandled YUV-format");
				break;
		}
	} else {
		kinc_log(KINC_LOG_LEVEL_ERROR, "unsupported caps name %s", gst_structure_get_name(structure));
	}

	gst_caps_unref(caps);

	return ret;
}

#endif
