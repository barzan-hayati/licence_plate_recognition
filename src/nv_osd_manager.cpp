#include "../include/my_libraries/nv_osd_manager.hpp"

#define SET_GPU_ID(object, gpu_id) \
    g_object_set(G_OBJECT(object), "gpu-id", gpu_id, NULL);
#define GPU_ID 0
#define OSD_PROCESS_MODE \
    1  // use GPU to draw rectangles, keypoints and text on frame if
       // OSD_PROCESS_MODE set to 1
#define OSD_DISPLAY_TEXT 1


NvOsdManager::NvOsdManager() {
}

bool NvOsdManager::create_nv_osd() {
    /* Create OSD to draw on the converted RGBA buffer */
    nvosd = gst_element_factory_make("nvdsosd", "nv-onscreendisplay");
    /* Finally render the osd output */
    g_object_set(G_OBJECT(nvosd), "process-mode", OSD_PROCESS_MODE,
                 "display-text", OSD_DISPLAY_TEXT, NULL);
    SET_GPU_ID(nvosd, GPU_ID);

    if (!nvosd) {
        g_printerr("Unable to create NVOSD. Exiting.\n");
        return false;
    }
    return true;
}