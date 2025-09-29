#include "../include/my_libraries/streammux_manager.hpp"

#define SET_GPU_ID(object, gpu_id) \
    g_object_set(G_OBJECT(object), "gpu-id", gpu_id, NULL);
#define GPU_ID 0
#define MUXER_BATCH_TIMEOUT_USEC 40000

StreammuxManager::StreammuxManager() {
    const auto& config = ConfigManager::get_instance().get_config();

    MUXER_OUTPUT_HEIGHT = config["MUXER_OUTPUT_HEIGHT"];
    MUXER_OUTPUT_WIDTH = config["MUXER_OUTPUT_WIDTH"];
}

bool StreammuxManager::create_streammux(int num_sources) {
    /* Create nvstreammux instance to form batches from one or more sources. */
    streammux = gst_element_factory_make("nvstreammux", "stream-muxer");
    g_object_set(G_OBJECT(streammux), "batch-size", num_sources, NULL);
    g_object_set(G_OBJECT(streammux), "enable-padding", 1, NULL);
    //    g_object_set (G_OBJECT (streammux), "drop-pipeline-eos",
    //    g_run_forever, NULL);
    g_object_set(G_OBJECT(streammux), "live-source", 1, NULL);
    g_object_set(G_OBJECT(streammux), "width", MUXER_OUTPUT_WIDTH, "height",
                 MUXER_OUTPUT_HEIGHT, "batched-push-timeout",
                 MUXER_BATCH_TIMEOUT_USEC, NULL);
    SET_GPU_ID(streammux, GPU_ID);

    if (!streammux) {
        g_printerr("Unable to create streammux.Exiting.");
        return false;
    }
    return true;
}