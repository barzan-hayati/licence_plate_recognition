#include "../include/my_libraries/tiler_manager.hpp"

#define SET_GPU_ID(object, gpu_id) \
    g_object_set(G_OBJECT(object), "gpu-id", gpu_id, NULL);
#define GPU_ID 0

TilerManager::TilerManager() {}

bool TilerManager::create_tiler(int num_sources, const int MUXER_OUTPUT_WIDTH,
                                const int MUXER_OUTPUT_HEIGHT) {
    /* Use nvtiler to composite the batched frames into a 2D tiled array based
     * on the source of the frames. */
    tiler = gst_element_factory_make("nvmultistreamtiler", "nvtiler");
    tiler_rows = (guint)sqrt(num_sources);
    tiler_columns = (guint)ceil(1.0 * num_sources / tiler_rows);
    /* we set the tiler properties here */
    g_object_set(G_OBJECT(tiler), "rows", tiler_rows, "columns", tiler_columns,
                 "width", MUXER_OUTPUT_WIDTH, "height", MUXER_OUTPUT_HEIGHT,
                 NULL);
    SET_GPU_ID(tiler, GPU_ID);
    // g_object_set (G_OBJECT (tiler), "show-source", 1);
    //   g_object_set (G_OBJECT (tiler), "show-source", 0, NULL);

    if (!tiler) {
        g_printerr("Could not create Tiler. Exiting.\n");
        return false;
    }
    return true;
}