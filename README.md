# SmartVision System

SmartVision is a high-performance, C++ based smart camera application designed for Rockchip Single Board Computers (SBCs), such as the RK3576. It captures video from a V4L2 camera, runs NPU-accelerated AI object detection (YOLOv8), and automatically tracks targets using a hardware pan-tilt gimbal and digital auto-zoom.

## Key Features

* **Hardware-Accelerated RTSP Streaming:** Uses GStreamer and Rockchip MPP (`mpph265enc`) for highly efficient H.265 video encoding.
* **NPU AI Object Detection:** Utilizes the Rockchip NPU (via RKNN Toolkit 2) for real-time YOLOv8 object detection.
* **Pan/Tilt Target Tracking:** Interfaces with a PCA9685 I2C servo controller to drive a pan-tilt gimbal, keeping targets centered using a customizable PID controller.
* **Hardware-Accelerated Auto-Zoom:** Uses Rockchip RGA (2D Graphics Engine) to automatically digitally zoom in/out to maintain a consistent target size on screen.
* **Remote Command Interface:** Includes a built-in broadcaster and command server for remote control (zoom, gimbal positioning, and tracking modes).

---

## Hardware Requirements

* **SBC:** Rockchip-based SBC with NPU and MPP support (e.g., RK3576).
* **Camera:** V4L2 compatible camera. *(Note: The default pipeline is optimized for 1920x1080@30fps MJPG output).*
* **Pan/Tilt Module:** I2C-based servo driver (PCA9685). Default configuration expects the device at address `0x40` on `/dev/i2c-8`.

---

## Installation & Deployment

Cross-compilation is recommended to keep your host environment clean. The repository includes a `build.sh` script that automates compilation using a Docker container.

### 1. Build and Deploy (Host Machine)

Run the build script to cross-compile the project for `aarch64`. Once built, copy the executable, AI models, configuration, and dependencies list to your SBC.

```bash
# Cross-compile the project using Docker
./build.sh

# Copy over the necessary files to the target SBC
scp build_aarch64/smartvision user@<IP_ADDRESS>:/home/user/smartvision/
scp -r models user@<IP_ADDRESS>:/home/user/smartvision/
scp config.json user@<IP_ADDRESS>:/home/user/smartvision/
scp requirements.txt user@<IP_ADDRESS>:/home/user/smartvision/
```

### 2. Target Dependencies (SBC)

Before running the application on your SBC, you must install the runtime system libraries. We've provided a `requirements.txt` file tailored for the SBC runtime.

SSH into your Rockchip SBC and run:

```bash
sudo apt update
grep -v -e '^#' -e '^$' requirements.txt | xargs sudo apt install -y
```
*(Note: Because the application is cross-compiled, you only need the runtime packages, not the `-dev` headers which may cause `libgbm` conflicts with Rockchip's Mali drivers).*

---

## Configuration (`config.json`)

The system's behavior is highly customizable via `config.json`. You can tune the camera resolution, framerate, model path, and the PID controller for the Pan/Tilt tracking.

```json
{
    "width": 1280,
    "height": 720,
    "fps": 30,
    "model_path": "./models/yolov8n.rknn",
    "pan_angle": 90,
    "tilt_angle": 90,
    "pan_kp": 21.0,
    "pan_ki": 0.0,
    "pan_kd": 0.5,
    "tilt_kp": 14.0,
    "tilt_ki": 0.0,
    "tilt_kd": 0.3
}
```

* **PID Tuning:** If the gimbal tracks too aggressively or oscillates, lower the proportional terms (`pan_kp`, `tilt_kp`). If it tracks too slowly, increase them. The derivative terms (`_kd`) help dampen the motion.

---

## Usage

Run the executable on your SBC:

```bash
cd /home/user/smartvision
./smartvision
```

Once running, the application will initialize the camera, load the NPU model, and start the RTSP server. You should see an output similar to:

```text
Stream ready at rtsp://[IP_ADDRESS]:8554/video
```

You can view the stream locally by opening this URL in a media player like **VLC** or `ffplay`.

---

## Technical Details: Camera Pipeline

The system is configured to read an MJPG stream from the camera. Conversion from GStreamer to NV12 format is required for the Rockchip MPP hardware H.265 encoder.

The internal GStreamer factory pipeline looks like this:
```cpp
gst_rtsp_media_factory_set_launch(
    factory,
    "( appsrc name=source is-live=true block=true format=time ! "
    " videoconvert ! "
    " video/x-raw,format=NV12 ! "
    " mpph265enc rc-mode=vbr bps=4000000 gop=30 ! "
    " rtph265pay name=pay0 pt=96 )"
);
```
*(If your camera only outputs raw YUV formats, you may need to adjust the V4L2 opening parameters in `smartvision.cc`).*

---

## License

This project is licensed under the GNU Affero General Public License v3.0 (AGPL-3.0). See the [LICENSE](LICENSE) file for the full text.