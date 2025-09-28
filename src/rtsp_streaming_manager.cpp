#include "../include/my_libraries/rtsp_streaming_manager.hpp"

#define SET_GPU_ID(object, gpu_id) \
    g_object_set(G_OBJECT(object), "gpu-id", gpu_id, NULL);
#define GPU_ID 0

GstRTSPServer *RtspStreamingManager::server;
std::string RtspStreamingManager::codec_rtsp_out = "";
std::string RtspStreamingManager::mount_address = "";
guint RtspStreamingManager::udp_buffer_size = 1;
guint RtspStreamingManager::clock_rate = 1;
guint RtspStreamingManager::rtsp_port = 1;
guint RtspStreamingManager::updsink_port_num = 1;
guint RtspStreamingManager::payload = 1;

RtspStreamingManager::RtspStreamingManager() {
    const auto &config = ConfigManager::get_instance().get_config();

    config.at("codec_rtsp_out").get_to(codec_rtsp_out);
    config.at("mount_address").get_to(mount_address);
    config.at("udp_buffer_size").get_to(udp_buffer_size);
    config.at("clock_rate").get_to(clock_rate);
    config.at("bitrate").get_to(bitrate);
    config.at("rtsp_port").get_to(rtsp_port);
    config.at("updsink_port_num").get_to(updsink_port_num);
    config.at("payload").get_to(payload);
    config.at("host").get_to(host);
}

gboolean RtspStreamingManager::start_rtsp_streaming() {
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;
    char udpsrc_pipeline[512];
    char port_num_Str[64] = {0};
    char *encoder_name;
    (void)encoder_name;

    if (udp_buffer_size == 0) udp_buffer_size = 512 * 1024;

    sprintf(port_num_Str, "%d", rtsp_port);
    sprintf(udpsrc_pipeline,
            "( udpsrc name=pay0 port=%d buffer-size=%u "
            "caps=\"application/x-rtp, media=video, "
            "clock-rate=%d, encoding-name=%s, payload=%d \" )",
            updsink_port_num, udp_buffer_size, clock_rate,
            codec_rtsp_out.c_str(), payload);  // H264
    // g_print(udpsrc_pipeline);
    g_print("%s\n", udpsrc_pipeline);
    server = gst_rtsp_server_new();
    g_object_set(server, "service", port_num_Str, NULL);
    mounts = gst_rtsp_server_get_mount_points(server);
    factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory, udpsrc_pipeline);
    gst_rtsp_mount_points_add_factory(mounts, mount_address.c_str(), factory);
    g_object_unref(mounts);
    gst_rtsp_server_attach(server, NULL);
    g_print(
        "\n *** DeepStream: Launched RTSP Streaming at rtsp://localhost:%d%s "
        "***\n\n",
        rtsp_port, mount_address.c_str());
    return TRUE;
}

GstRTSPFilterResult RtspStreamingManager::client_filter(GstRTSPServer *server,
                                                        GstRTSPClient *client,
                                                        gpointer user_data) {
    (void)server;     // This explicitly marks it as unused
    (void)client;     // This explicitly marks it as unused
    (void)user_data;  // This explicitly marks it as unused
    return GST_RTSP_FILTER_REMOVE;
}

void RtspStreamingManager::destroy_sink_bin() {
    GstRTSPMountPoints *mounts;
    GstRTSPSessionPool *pool;

    mounts = gst_rtsp_server_get_mount_points(server);
    gst_rtsp_mount_points_remove_factory(mounts, mount_address.c_str());
    g_object_unref(mounts);
    gst_rtsp_server_client_filter(server, client_filter, NULL);
    pool = gst_rtsp_server_get_session_pool(server);
    gst_rtsp_session_pool_cleanup(pool);
    g_object_unref(pool);
}