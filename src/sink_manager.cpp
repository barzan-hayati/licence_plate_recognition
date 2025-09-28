#include "../include/my_libraries/sink_manager.hpp"

#define SET_GPU_ID(object, gpu_id) \
    g_object_set(G_OBJECT(object), "gpu-id", gpu_id, NULL);
#define GPU_ID 0

SinkManager::SinkManager() {
    const auto& config = ConfigManager::get_instance().get_config();
    config.at("output_video_path").get_to(output_video_path);
    config.at("display_output").get_to(display_output);
    config.at("codec_rtsp_out").get_to(codec_rtsp_out);
}

void SinkManager::create_fake_sink() {
    fake_sink = gst_element_factory_make("fakesink",
                                         "fakesink-converter-broker-branch");
    g_object_set(G_OBJECT(fake_sink), "qos", 0, "sync", FALSE,
                 NULL);  //"name", "TEST",
}

bool SinkManager::create_sink(cudaDeviceProp prop, std::string host,
                              guint updsink_port_num) {
    if (display_output == 0) {
        output_sink = "fake_sink";
        sink = gst_element_factory_make("fakesink", "nvvideo-renderer");
        g_object_set(G_OBJECT(sink), "name", "fakesink", "qos", 0, "sync",
                     FALSE, NULL);
    } else if (display_output == 1) {
        output_sink = "displaying_window";
        sink = gst_element_factory_make("nveglglessink", "nvvideo-renderer");
        g_object_set(G_OBJECT(sink), "sync", FALSE, NULL);
        g_object_set(G_OBJECT(sink), "qos", 0, NULL);
    } else if (display_output == 2) {
        output_sink = "to_vido_file";
        sink = gst_element_factory_make("nvvideoencfilesinkbin",
                                        "nvvideo-renderer");
        g_object_set(G_OBJECT(sink), "container", 2, "output-file",
                     output_video_path.c_str(), NULL);
        g_object_set(G_OBJECT(sink), "name", "nvvideoencfilesinkbin", "qos", 0,
                     "sync", FALSE, NULL);
    } else if (display_output == 3) {
        output_sink = "to_rtsp";
        nvvidconv_postosd =
            gst_element_factory_make("nvvideoconvert", "convertor_postosd");
        if (!nvvidconv_postosd) {
            g_printerr("Unable to create nvvidconv_postosd.\n");
            return false;
        }

        //        Create a caps filter
        caps = gst_element_factory_make("capsfilter", "filter");
        g_object_set(
            caps, "caps",
            gst_caps_from_string(
                "video/x-raw(memory:NVMM), format=I420"),  // video/x-raw
            NULL);
        if (!caps) {
            g_printerr("Unable to create caps. Exiting.\n");
            return false;
        }

        //        Make the encoder
        if (!strcmp(codec_rtsp_out.c_str(), "H264")) {
            encoder = gst_element_factory_make("nvv4l2h264enc", "encoder");
            // encoder = gst_element_factory_make("x264enc", "h264 encoder");
            // g_object_set(G_OBJECT(encoder), "preset-level", 1, NULL);
            // g_object_set(G_OBJECT(encoder), "insert-sps-pps", 1, NULL);
            // g_object_set(G_OBJECT(encoder), "bufapi-version", 1, NULL);
            g_printerr("Creating H264 Encoder.\n");
        } else if (!strcmp(codec_rtsp_out.c_str(), "H265")) {
            encoder = gst_element_factory_make("nvv4l2h265enc", "encoder");
            g_printerr("Creating H265 Encoder.\n");
        } else {
            g_printerr(
                "RTSP Streaming Codec should be  H264/H265 , "
                "default=H264. Exiting.\n");
            return false;
        }
        g_object_set(encoder, "bitrate", bitrate, NULL);
        if (!encoder) {
            g_printerr("Unable to create encoder. Exiting.\n");
            return false;
        }

        //        Make the payload-encode video into RTP packets
        if (!strcmp(codec_rtsp_out.c_str(), "H264")) {
            rtppay = gst_element_factory_make("rtph264pay", "rtppay");
            g_printerr("Creating H264 rtppay.\n");
        } else if (!strcmp(codec_rtsp_out.c_str(), "H265")) {
            rtppay = gst_element_factory_make("rtph265pay", "rtppay");
            g_printerr("Creating H265 rtppay.\n");
        }
        if (!rtppay) {
            g_printerr("Unable to create rtppay. Exiting.\n");
            return false;
        }

        //        Make the UDP sink
        sink = gst_element_factory_make("udpsink", "udpsink");
        g_object_set(G_OBJECT(sink), "host", host.c_str(), NULL);
        g_object_set(G_OBJECT(sink), "port", updsink_port_num, NULL);
        g_object_set(G_OBJECT(sink), "async", FALSE, NULL);
        g_object_set(G_OBJECT(sink), "sync", 1, NULL);

        if (!sink) {
            g_printerr("Unable to create udpsink. Exiting.\n");
            return false;
        }
    }

    if (!prop.integrated && display_output < 3) {
        SET_GPU_ID(sink, GPU_ID);
    }

    if (!sink) {
        g_printerr("Could not create sink. Exiting.\n");
        return false;
    }
    return true;
}