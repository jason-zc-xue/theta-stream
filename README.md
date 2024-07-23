# theta-stream

## Usage information

Below is information that may be helpful for getting the executables to work the way you want it to.

### Notes on building the project

#### Building libuvc-theta

Do not follow the README found on [ricohapi's libuvc-theta][1] as it is copied directly from libuvc's
repository. Please run these commands instead:

```bash
git clone https://github.com/ricohapi/libuvc-theta.git
cd libuvc
mkdir build
cd build
cmake ..
make && sudo make install
```

#### Using v4l2loopback

For loopback to work, the [umlaeute's v4l2loopback][2] repository needs to be built. Follow the
instructions on the README then to create a virtual /dev/video1 device, run this command:

```bash
sudo modprobe v4l2loopback video_nr=1
```

Now you can run gst/gst_loopback and the video stream should be available as a v4l2 device.

#### Building the executables

In the gst/ directory, run the respective command for the result desired.

```bash
# To compile the executables
make
# To remove all non-source files to make again
make clean
```

### Build Target Descriptions

#### gst/gst_viewer

Used to test basic functionality as the simplest of the build targets. After the
[appsrc is parsed](#appsrc-parsing), the video stream is queued to decode into raw video and
displayed using autovideosink which selects a video player suited for the current device.

#### gst/gst_loopback

Used to sink the parsed H264 stream into a [v4l2loopback](#using-v4l2loopback) virtual video device.
The virtual video device can then be read from in a separate gstreamer pipeline as a v4l2src element
outputting H264 video.

#### gst/gst_rtsp_streamer

- Creates rtsp server

## Development information

Below is information that may be helpful if you are looking into modifying or understanding how the
code works to debug or something.

### General information

#### Appsrc Parsing

- H264 from theta not usable by non-picky gstreamer elements so need parsing

### Full Function Descriptions

- TODO: #### headers

### Possible TODOs

#### Configure to work with libuvc and not libuvc-theta

- Still bugs w/ libuvc-theta and occasionally needing to make install it again, but also not necessarily working without also installing libuvc

#### More robust gst_loopback

- Not hardcoded /dev/video1 sinking of the vid stream
- Automatically modprobing own v4l2loopback number instead of manual step before

[1]: <https://github.com/ricohapi/libuvc-theta> "ricohapi's libuvc-theta"
[2]: <https://github.com/umlaeute/v4l2loopback> "umlaeute's v4l2loopback"
