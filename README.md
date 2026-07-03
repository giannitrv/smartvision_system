# SmartVision System

SmartVision is a C++ based smart camera application designed for Rockchip SBCs, in my case a RADXA 4D with a RK3576 CPU. It captures video from a V4L2 camera, runs NPU-accelerated AI object detection, and target tracking on demand and displays the result over RTSP stream via ethernet.

## Key Features

* **Hardware-Accelerated RTSP Streaming:** Uses GStreamer and Rockchip MPP (`mpph265enc`) for highly efficient H.265 video encoding.
* **NPU AI Object Detection:** Utilizes the Rockchip NPU (via RKNN Toolkit 2) for real-time YOLOv8 object detection.
* **Pan/Tilt Target Tracking:** Interfaces with a PCA9685 I2C servo controller to drive a pan-tilt gimbal, keeping targets centered using a customizable PID controller.
* **Hardware-Accelerated Auto-Zoom:** Uses Rockchip RGA (2D Graphics Engine) to automatically digitally zoom in/out to maintain a consistent target size on screen.
* **Remote Command Interface:** Includes a built-in broadcaster and command server for remote control (zoom, gimbal positioning, and tracking modes).

---

## Project Structure

The project is organizied in multiple subfolders, each dedicated to a specific functionality of the smart vision system. Every folder has its own `CMakeLists.txt` file to simplify the build process.

---

## Hardware Requirements

* **SBC:** Rockchip-based SBC with NPU and MPP support (e.g., RK3576).
* **Camera:** V4L2 compatible camera. The default pipeline is optimized for 1280x720@30fps MJPG output.
* **Pan/Tilt Module:** I2C-based servo driver (PCA9685).

---

## Installation & Deployment

Cross-compilation is executed inside a Docker container so no packages need to be installed on the host. The application is configured to use the ethernet interface to transmit the video stream towards a host computer, setting a static IP address on your SBC is recommended.

### 1. Build and Deploy (Host Machine)

Run `build.sh` to cross-compile the project for `aarch64`. Once built, copy the executable, AI models, configuration, and dependencies list to your SBC.

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

Before running the application on your SBC, you must install the runtime system libraries.

SSH into your Rockchip SBC and run:

```bash
sudo apt update
grep -v -e '^#' -e '^$' requirements.txt | xargs sudo apt install -y
```
*(Note: Because the application is cross-compiled, you only need the runtime packages, not the `-dev` headers which may cause `libgbm` conflicts with Rockchip's Mali drivers).*

---

## Configuration (`config.json`)

The system behavior is customizable via `config.json`. More information about the parameters can be found in the Smartvision System Manual.

---

## Usage

Run the executable on your SBC:

```bash
cd /home/user/smartvision
./smartvision
```

Once running, the application will initialize the camera, load the NPU model, and start the RTSP server. You should see the output:

```text
Stream ready at rtsp://[IP_ADDRESS]:8554/video
```

You can view the stream locally with the deidicated application Smartvision Client or with any media player like **VLC** or `ffplay`.

---

## License

This project is licensed under the GNU Affero General Public License v3.0 (AGPL-3.0). See the [LICENSE](LICENSE) file for the full text.