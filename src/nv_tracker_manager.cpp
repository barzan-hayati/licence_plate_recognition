#include "../include/my_libraries/nv_tracker_manager.hpp"

#define NVDS_USER_OBJECT_META_LANDMARKS_AND_SOURCE_ID \
    (nvds_get_user_meta_type(const_cast<gchar *>("NVIDIA.NVINFER.USER_META")))
#define SET_GPU_ID(object, gpu_id) \
    g_object_set(G_OBJECT(object), "gpu-id", gpu_id, NULL);
#define GPU_ID 0
#define PGIE_CLASS_ID_CAR 0
#define PGIE_DETECTED_CLASS_NUM 3
#define CAR_COMPONENT_ID 1
// #define CAR_TENSOR_SIZE 57
// #define MAX_CAR_PER_FRAME 100
#define PRIMARY_DETECTOR_UID 1

gint NvTrackerManager::frame_number = 0;
const gchar face_class_str[PGIE_DETECTED_CLASS_NUM][32] = {"CAR_NVINFER",
                                                           "Truck", "Cycle"};

NvTrackerManager::NvTrackerManager() {
    const auto &config = ConfigManager::get_instance().get_config();
    ll_config_file =
        config.at("tracker").at("ll-config-file").get<std::string>();
    ll_lib_file = config.at("tracker").at("ll-lib-file").get<std::string>();
}

bool NvTrackerManager::create_nv_tracker() {
    tracker = gst_element_factory_make("nvtracker", "tracker_plugin");
    g_object_set(G_OBJECT(tracker), "ll-config-file", ll_config_file.c_str(),
                 NULL);
    g_object_set(G_OBJECT(tracker), "ll-lib-file", ll_lib_file.c_str(), NULL);
    g_object_set(G_OBJECT(tracker), "display-tracking-id", 1, NULL);
    g_object_set(G_OBJECT(tracker), "gpu_id", GPU_ID, NULL);
    // g_object_set (G_OBJECT (tracker), "enable_batch_process", 1, NULL);

    if (!tracker) {
        g_printerr("Unable to create Tracker.\n");
        return false;
    }
    return true;
}