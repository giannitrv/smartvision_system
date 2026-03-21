# Smartvision System

## Installation
Cross compilation is done using Docker container. Model directory must be copied to the target device in the same folder as the executable.

```bash
# Cross compile the project with the script
./build.sh

# Copy the executable and model directory to the target device
scp build_aarch64/smartvision user@<IP_ADDRESS>:/home/user/smartvision
scp -r models user@<IP_ADDRESS>:/home/user/smartvision
```

## Camera

Camera can output 1920x1080@30fps only in MJPG format. Conversion is required from gstreamer to NV12 for MPP encoder.

```cpp
gst_rtsp_media_factory_set_launch(
    factory,
    "( v4l2src device=/dev/video0 ! "
    "image/jpeg,width=1920,height=1080,framerate=30/1 ! "
    "jpegdec ! "
    "videoconvert ! "
    "video/x-raw,format=NV12 ! "
    "mpph265enc rc-mode=vbr bps=4000000 gop=30 ! "
    "rtph265pay config-interval=1 name=pay0 pt=96 )"
);
```