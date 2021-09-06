const lib = new Project('kinc-video-gstreamer');

lib.addDefine('KINC_VIDEO_GSTREAMER');

lib.addIncludeDir('src');
lib.addFile('src/kinc/backend/**');

// external dependencies
lib.addIncludeDir('/usr/include/gstreamer-1.0');
lib.addIncludeDir('/usr/include/glib-2.0');
lib.addIncludeDir('/usr/lib/x86_64-linux-gnu/glib-2.0/include');

lib.addLib('gstbase-1.0');
lib.addLib('gstreamer-1.0');
lib.addLib('gstvideo-1.0');
lib.addLib('gobject-2.0');
lib.addLib('glib-2.0');

resolve(lib);
