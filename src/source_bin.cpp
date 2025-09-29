#include "../include/my_libraries/source_bin.hpp"

// Initialize static member (required for non-const static members)
// the linker needs real storage for them.

// string statics
std::string SourceBin::ip_address;
std::string SourceBin::port;
std::string SourceBin::uri_list;
std::string SourceBin::sensor_id_list;
std::string SourceBin::sensor_name_list;
std::string SourceBin::config_file_path;

// int statics
int SourceBin::max_batch_size = 0;
int SourceBin::batched_push_timeout = 0;
int SourceBin::rtsp_reconnect_interval = 0;
int SourceBin::rtsp_reconnect_attempts = 0;
int SourceBin::drop_frame_interval = 0;
int SourceBin::width = 0;
int SourceBin::height = 0;
int SourceBin::latency = 0;
int SourceBin::cudadec_memtype = 0;
int SourceBin::buffer_pool_size = 0;
int SourceBin::max_latency = 0;
int SourceBin::num_extra_surfaces = 0;
int SourceBin::num_surfaces_per_frame = 0;
int SourceBin::live_source = 0;

// bool statics
bool SourceBin::drop_pipeline_eos = false;
bool SourceBin::file_loop = false;
bool SourceBin::disable_audio = false;

GstElement *SourceBin::nvmultiurisrcbin = NULL;

void SourceBin::configs() {
    // const auto &config = ConfigManager::get_instance().get_config();
    // MUXER_OUTPUT_HEIGHT = config["MUXER_OUTPUT_HEIGHT"];
    // MUXER_OUTPUT_WIDTH = config["MUXER_OUTPUT_WIDTH"];
}

// Definition of static function
void SourceBin::decodebin_child_added(GstChildProxy *child_proxy,
                                      GObject *object, gchar *name,
                                      gpointer user_data) {
    (void)child_proxy;  // This explicitly marks it as unused

    StreamData *data = static_cast<StreamData *>(user_data);
    gint source_id = data->source_id;
    // gint source_id = (*(gint *) user_data);

    g_print(
        "Decodebin child added %s for stream_id %d"
        "\n",
        name, source_id);
    if (g_strrstr(name, "decodebin") == name) {
        g_signal_connect(G_OBJECT(object), "child-added",
                         G_CALLBACK(decodebin_child_added), user_data);
    }

    if (g_strrstr(name, "nvv4l2decoder") == name) {
        if (data->prop.integrated) {
            g_object_set(object, "enable-max-performance", TRUE, NULL);
            g_object_set(object, "bufapi-version", TRUE, NULL);
            g_object_set(object, "drop-frame-interval", 0, NULL);
            g_object_set(object, "num-extra-surfaces", 0, NULL);
        } else {
            g_object_set(object, "gpu-id", GPU_ID, NULL);
        }
    }
}

// Definition of static function
void SourceBin::cb_newpad(GstElement *decodebin, GstPad *pad,
                          gpointer user_data, gboolean *flag) {
    (void)decodebin;  // This explicitly marks it as unused
    (void)flag;       // This explicitly marks it as unused
    StreamData *data = static_cast<StreamData *>(user_data);
    gint source_id = data->source_id;
    GstElement *streammux = data->streammux;
    // gint source_id = (*(gint *) data);
    g_print(
        "In cb_newpad for stream_id %d"
        "\n",
        source_id);
    GstCaps *caps = gst_pad_query_caps(pad, NULL);
    const GstStructure *str = gst_caps_get_structure(caps, 0);
    const gchar *name = gst_structure_get_name(str);

    //    g_print ("decodebin new pad %s\n", name);
    g_print(
        "decodebin new pad %s for stream_id %d"
        "\n",
        name, source_id);
    if (!strncmp(name, "video", 5)) {
        gchar pad_name[16] = {0};
        GstPad *sinkpad = NULL;
        g_snprintf(pad_name, 15, "sink_%u", source_id);
        sinkpad = gst_element_request_pad_simple(
            streammux, pad_name);  // gst_element_get_request_pad

        if (!sinkpad) {
            g_printerr(
                "Streammux request sink pad failed for stream_id %d or "
                ". Exiting. \n",
                source_id);
        }
        if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK) {
            g_printerr(
                "Failed to link decodebin bin to pipeline for stream_id %d or "
                ". Exiting.\n",
                source_id);
        } else {
            //            g_print ("Decodebin linked to pipeline\n");
            g_print(
                "Decodebin linked to pipeline for stream_id %d"
                "\n",
                source_id);
        }
        gst_object_unref(sinkpad);
    }
}

// Definition of static function
GstElement *SourceBin::create_uridecode_bin(guint index, gchar *filename,
                                            GstElement *streammux,
                                            cudaDeviceProp prop) {
    GstElement *decodebin = NULL;
    gchar decodebin_name[16] = {};
    // Create data structure for callbacks
    StreamData *stream_data = new StreamData{(int)index, streammux, prop};

    //    g_print ("creating uridecodebin for [%s]\n", filename);
    g_print("Creating uridecodebin for stream_id %d or stream %s \n", index,
            filename);
    // g_source_id_list[index] = index;
    g_snprintf(decodebin_name, 15, "source-bin-%02d", index);
    decodebin = gst_element_factory_make("uridecodebin", decodebin_name);
    g_object_set(G_OBJECT(decodebin), "uri", filename, NULL);
    g_signal_connect(G_OBJECT(decodebin), "pad-added", G_CALLBACK(cb_newpad),
                     stream_data);  //&g_source_id_list[index]
    g_signal_connect(
        G_OBJECT(decodebin), "child-added", G_CALLBACK(decodebin_child_added),
        stream_data);  //&g_source_id_list[index] //&stream_data->source_id
    // g_source_enabled[index] = TRUE;

    return decodebin;
}

// static void check_versions() {
//     guint major, minor, micro, nano;
//     gst_version(&major, &minor, &micro, &nano);
//     g_print("GStreamer version: %u.%u.%u.%u\n", major, minor, micro, nano);

//     // Check if nvmultiurisrcbin is available
//     GstElementFactory *factory =
//     gst_element_factory_find("nvmultiurisrcbin"); if (factory) {
//         // Get the plugin name from the factory
//         const gchar *plugin_name =
//         gst_plugin_feature_get_plugin_name(GST_PLUGIN_FEATURE(factory));

//         // Find the plugin in the registry
//         GstRegistry *registry = gst_registry_get();
//         GstPlugin *plugin = gst_registry_find_plugin(registry, plugin_name);

//         if (plugin) {
//             const gchar *version = gst_plugin_get_version(plugin);
//             g_print("nvmultiurisrcbin plugin: %s, version: %s\n",
//             plugin_name, version); gst_object_unref(plugin);
//         } else {
//             g_print("nvmultiurisrcbin found (plugin: %s), but couldn't get
//             version\n", plugin_name);
//         }
//         gst_object_unref(factory);
//     } else {
//         g_print("nvmultiurisrcbin not found\n");
//     }
// }

// static void check_versions() {
//     guint major, minor, micro, nano;
//     gst_version(&major, &minor, &micro, &nano);
//     g_print("GStreamer version: %u.%u.%u.%u\n", major, minor, micro, nano);

//     // Check nvmultiurisrcbin version
//     GstPluginFeature *feature = gst_registry_find_feature(
//         gst_registry_get(), "nvmultiurisrcbin", GST_TYPE_ELEMENT_FACTORY);
//     if (feature) {
//         const gchar *plugin_name =
//         gst_plugin_feature_get_plugin_name(feature); GstPlugin *plugin =
//         gst_registry_find_plugin(gst_registry_get(), plugin_name); if
//         (plugin) {
//             const gchar *version = gst_plugin_get_version(plugin);
//             g_print("nvmultiurisrcbin plugin version: %s\n", version);
//             gst_object_unref(plugin);
//         }
//         gst_object_unref(feature);
//     }
// }