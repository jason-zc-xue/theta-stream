# theta-stream

1. [Setup・セットアップ](#setup・セットアップ)
2. [Usage・使い方](#usage・使い方)
    - [Profiles・プロフィール](#profiles・プロフィール)
4. [Development Information・開発情報](#development-information・開発情報)
5. [About Us・メンバー](#about-us・メンバー)

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

## Usage・使い方

Simply execute `gst_viewer` in the command line.
The help flag (`-h`) gives more detailed usage information.

```bash
./gst_viewer -p loopback
```

### *Profiles・プロフィール*

#### **`viewer`**

Used to test basic functionality.
After the [appsrc is parsed](#appsrc-parsing), the video stream is queued to decode into raw video and displayed using `autovideosink` which selects a video player suited for the current device.

Known as `gst/gst_viewer` in the old code.

#### **`loopback`**

Used to sink the parsed H264 stream into a [v4l2loopback](#using-v4l2loopback) virtual video device.
The virtual video device can then be read from in a separate gstreamer pipeline as a `v4l2src` element outputting H264 video.

Known as `gst/gst_loopback` in the old code.

#### **`streamer`**

Creates an RTSP server.

Known as `gst/gst_rtsp_streamer` in the old code.

The given address should be the streamer's IP address
- IPv4: port forward your address with the port `8554`
        `location=rtsp://xxx.xxx.xxx.xxx:8554/stream"`
- IPv6: `location=rtsp://[xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx]/stream`

Note: "stream" can be any unique alphanumeric identifier

### *Using `v4l2loopback`*

For loopback to work, [umlaeute's v4l2loopback][2] must be built from source.
Follow the instructions in their README to do so.
Then, to create a virtual `/dev/video1` device, run this command.

```bash
sudo modprobe v4l2loopback video_nr=1
```

## Development Information・開発情報

Below is information that may be helpful if you are trying to modify or understand how the code works.

### *Appsrc Parsing*

RICOH Theta V's H264 video stream seems to use an H264 version that isn't compatible with GStreamer, so we add the `h264parse` element to the pipeline so that picky GStreamer elements can use it.

## About Us・メンバー

**Jason Xue** ([email](mailto:xue.j.ac@m.titech.ac.jp)) came to us on exchange for the Spring 2024 semester from the University of Waterloo (Canada).
This GStreamer project, the goal of which was to vastly improve video streaming latency over LAN, was one of his projects.

[1]: <https://github.com/ricohapi/libuvc-theta> "ricohapi's libuvc-theta"
[2]: <https://github.com/umlaeute/v4l2loopback> "umlaeute's v4l2loopback"
