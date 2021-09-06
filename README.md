# kinc-video-gstreamer

Kha/Kinc video implementation for linux native. Currently only supports OpenGL and `I420` / `NV12` encoded videos, but it's easily extensible by providing more shaders.

## attention

Requires all video.h/c.h code in kinc to be wrapped with an `#if !defined(KINC_VIDEO_GSTREAMER) ... #endif` until i put up a PR for that.

## system installation

```
apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
```

[Here](https://gstreamer.freedesktop.org/documentation/installing/on-linux.html?gi-language=c) is additional documentation.

## usage

#### your khafile.js

```
project.addLibrary('kinc-video-gstreamer');
await project.addProject('kinc-video-gstreamer');
```

#### library kincfile.js

The provided `kincfile.js` might need different include paths, check your distribution for that.

#### example code

```
var update_task = -1;
var video = null;

kha.Assets.loadVideoFromPath(url,
	function( loaded_video ) {
		video = loaded_video;
		updateTask = kha.Scheduler.addTimeTask(update_video, 0, 1 / 60);
	},
	...
);


function update_video() {
	// call video.update() periodically for message handling
	video.update(1 / 60);

	if (video.isFinished()) {
		video.unload();
		kha.Scheduler.removeTimeTask(update_task);
	}
}
```
