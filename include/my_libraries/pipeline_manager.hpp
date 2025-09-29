#include <glib.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include <fstream>
#include <chrono>
#include <iostream>

#include "cuda_runtime_api.h"
#include "gstnvdsmeta.h"
#include "config_manager.hpp"
#include "streammux_manager.hpp"
#include "source_bin.hpp"
#include "tiler_manager.hpp"
#include "nv_video_convert_manager.hpp"
#include "nv_osd_manager.hpp"
#include "queue_manager.hpp"
#include "nv_ds_logger_manager.hpp"
#include "sink_manager.hpp"
#include "rtsp_streaming_manager.hpp"
#include "message_handling.hpp"
#include "primary_nv_infer_manager.hpp"


class PipelineManager {
   private:
    gboolean g_run_forever = FALSE;
    GMainLoop *loop = NULL;
    GstElement *pipeline = NULL;
    GMutex eos_lock;
    static double tee_fps;
    static double video_converter_fps;
    static double osd_fps;
    StreammuxManager *streammux_manager = new StreammuxManager();
    TilerManager *tiler_manager = new TilerManager();
    NvVideoConvertManager *nv_video_convert_manager =
        new NvVideoConvertManager();
    NvOsdManager *nv_osd_manager = new NvOsdManager();
    QueueManager queue_array[5];
    NvDsLoggerManager *nv_ds_logger_manager = new NvDsLoggerManager();
    SinkManager *sink_manager = new SinkManager();
    RtspStreamingManager *rtsp_streaming_manager = new RtspStreamingManager();
    MessageHandling *message_handling = new MessageHandling();
    PrimaryNvInferManager *primary_nv_infer_manager = new PrimaryNvInferManager();
    typedef struct {
        TilerManager *tiler_manager;
    } DataPointer;

   public:
    int current_device = -1;
    struct cudaDeviceProp prop;

    PipelineManager();
    PipelineManager(int, char **);
    bool create_pipeline();
    bool create_pipeline_elements(int, char **);
    bool connect_tee_to_queue();
    bool setup_pipeline();
    bool playing_pipeline(int, char **);
    bool status_playing;
    void set_cuda_device();
    static guint64 frame_count_osd_sink;
    static guint64 frame_count_fps_probe;
    static guint64 frame_count_buffer_probe;
    const gchar *new_mux_str;
    gboolean use_new_mux;
    GstPad *pgie_src_pad = NULL;
    GstPad *sgie_src_pad = NULL;
    static std::chrono::time_point<std::chrono::steady_clock>
        last_time_osd_sink;
    static std::chrono::time_point<std::chrono::steady_clock>
        last_time_fps_probe;
    static std::chrono::time_point<std::chrono::steady_clock>
        last_time_buffer_probe;
    static gboolean event_thread_func(gpointer);
    static gboolean check_pipeline_state(gpointer);
    static GstPadProbeReturn tee_sink_fps(GstPad *, GstPadProbeInfo *,
                                          gpointer);
    static GstPadProbeReturn video_converter_src_fps(GstPad *,
                                                     GstPadProbeInfo *,
                                                     gpointer);
    static GstPadProbeReturn osd_sink_fps(GstPad *, GstPadProbeInfo *,
                                          gpointer);
    void get_fps_tee();
    void get_fps_video_converter();
    void get_fps_osd();
    bool check_playing_pipeline();
    ~PipelineManager();
};