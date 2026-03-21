#include "smartvision.h"
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

// ─── Constants ────────────────────────────────────────────────────────────────
static constexpr int STREAM_WIDTH = 1280;
static constexpr int STREAM_HEIGHT = 720;
static constexpr int STREAM_FPS = 30;
const char *model_path = "/home/radxa/smartvision/models/yolov8n.rknn";

static SmartVision smartvision(STREAM_WIDTH, STREAM_HEIGHT, STREAM_FPS, model_path);
static uint64_t frame_count_ = 0;
const GstClockTime frame_duration = GST_SECOND / static_cast<GstClockTime>(STREAM_FPS);

/* called when we need to give data to appsrc */
static void need_data(GstElement *appsrc, guint unused) {
  cv::Mat frame;

  frame = smartvision.getLatestFrame();

  gsize size = frame.total() * frame.elemSize();

  GstBuffer *buffer = gst_buffer_new_allocate(NULL, size, NULL);

  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_WRITE);
  memcpy(map.data, frame.data, size);
  gst_buffer_unmap(buffer, &map);

  GST_BUFFER_PTS(buffer) = frame_count_ * frame_duration;
  GST_BUFFER_DURATION(buffer) = frame_duration;

  frame_count_++;

  GstFlowReturn ret;
  g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);

  gst_buffer_unref(buffer);
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media,
                            gpointer user_data) {
  GstElement *element, *appsrc;

  /* get the element used for providing the streams of the media */
  element = gst_rtsp_media_get_element(media);

  /* get our appsrc, we named it 'mysrc' with the name property */
  appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "source");

  /* this instructs appsrc that we will be dealing with timed buffer */
  gst_util_set_object_arg(G_OBJECT(appsrc), "format", "time");
  /* configure the caps of the video */
  g_object_set(G_OBJECT(appsrc), "caps",
               gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING,
                                   "BGR", "width", G_TYPE_INT, STREAM_WIDTH,
                                   "height", G_TYPE_INT, STREAM_HEIGHT,
                                   "framerate", GST_TYPE_FRACTION, STREAM_FPS,
                                   1, NULL),
               NULL);

  /* install the callback that will be called when a buffer is needed */
  g_signal_connect(appsrc, "need-data", (GCallback)need_data, NULL);
  gst_object_unref(appsrc);
  gst_object_unref(element);
}

int main(int argc, char *argv[]) {
  GMainLoop *loop;
  GstRTSPServer *server;
  GstRTSPMountPoints *mounts;
  GstRTSPMediaFactory *factory;

  smartvision.start();

  gst_init(&argc, &argv);

  loop = g_main_loop_new(NULL, FALSE);

  /* create a server instance */
  server = gst_rtsp_server_new();

  /* get the mount points for this server, every server has a default object
   * that be used to map uri mount points to media factories */
  mounts = gst_rtsp_server_get_mount_points(server);

  /* make a media factory for a test stream. The default media factory can use
   * gst-launch syntax to create pipelines.
   * any launch line works as long as it contains elements named pay%d. Each
   * element with pay%d names will be a stream */
  factory = gst_rtsp_media_factory_new();
  gst_rtsp_media_factory_set_launch(
      factory, "( appsrc name=source is-live=true block=true format=time ! "
               " videoconvert !"
               " video/x-raw,format=NV12 !"
               " mpph265enc rc-mode=vbr bps=4000000 gop=30 !"
               " rtph265pay name=pay0 pt=96 )");

  /* notify when our media is ready, This is called whenever someone asks for
   * the media and a new pipeline with our appsrc is created */
  g_signal_connect(factory, "media-configure", (GCallback)media_configure,
                   NULL);

  /* attach the test factory to the /stream url */
  gst_rtsp_mount_points_add_factory(mounts, "/stream", factory);

  /* don't need the ref to the mounts anymore */
  g_object_unref(mounts);

  /* attach the server to the default maincontext */
  gst_rtsp_server_attach(server, NULL);

  /* start serving */
  g_print("Stream ready at rtsp://[IP_ADDRESS]:8554/stream\n");
  g_main_loop_run(loop);

  smartvision.stop();

  g_main_loop_unref(loop);

  g_print("Application stopped cleanly\n");

  return 0;
}
