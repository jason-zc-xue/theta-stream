# libuvc-theta-sample
## gst/gst_viewer
Decode and display sample using gstreamer. You may need gstreamer1.0 develpment packages to build and run.

## gst/gst_loopback
Feed decoded video to the v4l2loopback device so that v4l2-based application can use THETA video without modification.

CAUTION: gst_loopback may not run on all platforms, as decoder to v4l2loopback pipeline configuration is platform dependent,



### Notes

Do not follow libuvc-theta's instructions to a T, they literally copied the README from libuvc,
so make sure you replace any instance of libuvc with libuvc-theta

Also, for loopback to work, you need v4l2loopback, altho its not used anymore but good to know in
case





