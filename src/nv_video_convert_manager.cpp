#include "../include/my_libraries/nv_video_convert_manager.hpp"

#define SET_GPU_ID(object, gpu_id) \
    g_object_set(G_OBJECT(object), "gpu-id", gpu_id, NULL);
#define GPU_ID 0

NvVideoConvertManager::NvVideoConvertManager() {}

bool NvVideoConvertManager::create_nv_video_convert() {
    /* Use convertor to convert from NV12 to RGBA as required by nvosd */
    nvvidconv = gst_element_factory_make("nvvideoconvert", "nvvideo-converter");
    SET_GPU_ID(nvvidconv, GPU_ID);
    if (!nvvidconv) {
        g_printerr("Could not create nvvideoconvert.Exiting. \n");
        return false;
    }
    return true;
}