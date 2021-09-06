const lib = new Project('kinc-video-gstreamer');
lib.addCDefine('KINC_VIDEO_GSTREAMER');
lib.addShaders('shaders/**');
resolve(lib);
