#include <glib.h>
#include <gst/gst.h>

#include <fstream>

#include "config_manager.hpp"
#include "cuda_runtime_api.h"

class SinkManager {
   private:
    std::string codec_rtsp_out;

   public:
    GstElement *sink = NULL, *nvvidconv_postosd = NULL, *caps = NULL,
               *encoder = NULL, *rtppay = NULL, *fake_sink = NULL;
    std::string output_sink, output_video_path;
    int display_output = 1, bitrate;
    SinkManager();
    bool create_sink(cudaDeviceProp prop, std::string, guint);
    void create_fake_sink();
    ~SinkManager();
};