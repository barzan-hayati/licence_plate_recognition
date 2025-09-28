#include <glib.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <fstream>

#include "config_manager.hpp"
#include "cuda_runtime_api.h"

class RtspStreamingManager {
   private:
   public:
    static gboolean start_rtsp_streaming();
    static GstRTSPFilterResult client_filter(GstRTSPServer *, GstRTSPClient *,
                                             gpointer);
    static void destroy_sink_bin();

    static GstRTSPServer *server;
    static guint rtsp_port;
    static guint updsink_port_num;
    int bitrate;
    static std::string codec_rtsp_out;
    static guint udp_buffer_size;
    static guint clock_rate;
    static guint payload;
    static std::string mount_address;
    std::string host;
    RtspStreamingManager();
    ~RtspStreamingManager();
};