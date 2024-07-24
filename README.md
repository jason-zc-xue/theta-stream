# theta-stream

1. [Setup・セットアップ](#setup・セットアップ)
2. [Usage・使い方](#usage・使い方)
3. [Development Information・開発情報](#development-information・開発情報)
4. [About Us・メンバー](#about-us・メンバー)

## Setup・セットアップ

The following commands should be run from the project root directory.

1. Install necessary packages.
    ```bash
    sudo apt -y install cmake libusb-1.0-0-dev
    ```
2. Build [ricohapi's `libuvc-theta`][1] from source **using the following commands**. Do **not** follow the README in the `libuvc-theta` repo, as it contains mistakes (it is directly copied from the `libuvc` repo).
    ```bash
    git clone https://github.com/ricohapi/libuvc-theta.git
    cd libuvc-theta
    mkdir build && cd build
    cmake ..
    make && sudo make install
    ```
3. Build this project.
    ```bash
    cd gst
    mkdir build && cd build
    cmake .. && make
    ```

### *Old Build Instructions (DELETE LATER)*

In the `gst_old` directory, run the respective command for the desired result.

```bash
# To compile the executables
make
# To remove all non-source files to make again
make clean
```

#### **gst/gst_viewer**

Used to test basic functionality as the simplest of the build targets.
After the [appsrc is parsed](#appsrc-parsing), the video stream is queued to decode into raw video and displayed using `autovideosink` which selects a video player suited for the current device.

#### **gst/gst_loopback**

Used to sink the parsed H264 stream into a [v4l2loopback](#using-v4l2loopback) virtual video device. The virtual video device can then be read from in a separate gstreamer pipeline as a `v4l2src` element outputting H264 video.

#### **gst/gst_rtsp_streamer**

Creates an RTSP server.

## Usage・使い方

Simply execute `gst_viewer` in the command line.
The help flag (`-h`) gives more detailed usage information.

```bash
./gst_viewer -p loopback
```

### *Using `v4l2loopback`*

For loopback to work, [umlaeute's v4l2loopback][2] must be built from source. Follow the instructions in their README to do so. Then, to create a virtual `/dev/video1` device, run this command.

```bash
sudo modprobe v4l2loopback video_nr=1
```

## Development Information・開発情報

Below is information that may be helpful if you are trying to modify or understand how the code works.

### *General Information (TODO)*

#### **Appsrc Parsing**

- H264 from theta not usable by non-picky gstreamer elements so need parsing

### Full Function Descriptions

- TODO: #### headers

### Possible TODOs

#### Configure to work with libuvc and not libuvc-theta

- Still bugs w/ `libuvc-theta` and occasionally needing to make install it again, but also not necessarily working without also installing `libuvc`

#### More robust `gst_loopback`

- Non-hardcoded `/dev/video1` sinking of the vid stream
- Automatically modprobing own `v4l2loopback` number instead of manual step before

## About Us・メンバー

**Jason Xue** ([email](mailto:xue.j.ac@m.titech.ac.jp)) came to us on exchange for the Spring 2024 semester from the University of Winnipeg (Canada).
This GStreamer project, the goal of which was to vastly improve video streaming latency over LAN, was one of his projects.

[1]: <https://github.com/ricohapi/libuvc-theta> "ricohapi's libuvc-theta"
[2]: <https://github.com/umlaeute/v4l2loopback> "umlaeute's v4l2loopback"
