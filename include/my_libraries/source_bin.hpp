// #ifndef MYCLASS_H
// #define MYCLASS_H
#include <glib.h>
#include <gst/gst.h>

#include <fstream>
#include <iostream>

#include "config_manager.hpp"
#include "cuda_runtime_api.h"
#include "source_config.hpp"

#define GPU_ID 0

class SourceBin {
   public:
    typedef struct {
        gint source_id;
        GstElement *streammux;
        struct cudaDeviceProp prop;
    } StreamData;

    static GstElement *nvmultiurisrcbin;
    // Static function declaration
    static void decodebin_child_added(GstChildProxy *, GObject *, gchar *,
                                      gpointer);
    static void cb_newpad(GstElement *, GstPad *, gpointer, gboolean *);
    static GstElement *create_uridecode_bin(guint, gchar *, GstElement *,
                                            cudaDeviceProp prop);

   private:
    static void configs();

    static int max_batch_size, live_source, batched_push_timeout,
        rtsp_reconnect_interval, rtsp_reconnect_attempts, drop_frame_interval,
        width, height, latency, cudadec_memtype, buffer_pool_size, max_latency,
        num_extra_surfaces, num_surfaces_per_frame;
    static bool drop_pipeline_eos, file_loop, disable_audio;
    static std::string ip_address, port, uri_list, sensor_id_list,
        sensor_name_list, config_file_path;
    // Static data member (if needed)
    // static int staticCounter;
};

// #endif // MYCLASS_H