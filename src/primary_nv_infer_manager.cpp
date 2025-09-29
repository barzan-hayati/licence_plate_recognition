#include "../include/my_libraries/primary_nv_infer_manager.hpp"

// #define NVDS_USER_OBJECT_META_LANDMARKS_AND_SOURCE_ID
// (nvds_get_user_meta_type("NVIDIA.NVINFER.USER_META"))
#define NVDS_USER_OBJECT_META_LANDMARKS_AND_SOURCE_ID \
    (nvds_get_user_meta_type(const_cast<gchar *>("NVIDIA.NVINFER.USER_META")))

#define MAX_DISPLAY_LEN 64
#define PGIE_CLASS_ID_CAR 0
#define PGIE_DETECTED_CLASS_NUM 3
#define CAR_COMPONENT_ID 1
// #define CAR_TENSOR_SIZE 57
// #define MAX_CAR_PER_FRAME 100
#define PRIMARY_DETECTOR_UID 1

gint PrimaryNvInferManager::frame_number = 0;
unsigned int PrimaryNvInferManager::PGIE_NET_WIDTH = 1;
unsigned int PrimaryNvInferManager::PGIE_NET_HEIGHT = 1;
unsigned int PrimaryNvInferManager::MUXER_OUTPUT_WIDTH = 1;
unsigned int PrimaryNvInferManager::MUXER_OUTPUT_HEIGHT = 1;
guint PrimaryNvInferManager::use_device_mem = 0;
float PrimaryNvInferManager::threshold_car_detection = 0;
unsigned int PrimaryNvInferManager::nvds_lib_major_version = NVDS_VERSION_MAJOR;
unsigned int PrimaryNvInferManager::nvds_lib_minor_version = NVDS_VERSION_MINOR;

const gchar pgie_class_str[PGIE_DETECTED_CLASS_NUM][32] = {"CAR_NVINFER", "Truck", "Cycle"};

/* nvds_lib_major_version and nvds_lib_minor_version is the version number of
 * deepstream sdk */

unsigned int nvds_lib_major_version = NVDS_VERSION_MAJOR;
unsigned int nvds_lib_minor_version = NVDS_VERSION_MINOR;

PrimaryNvInferManager::PrimaryNvInferManager() {
    const auto &config = ConfigManager::get_instance().get_config();
    pgie_batch_size = config["pgie_batch_size"];
    primary_nv_infer_config_file =
        config["primary_nv_infer_config_file"].get<std::string>();
    PGIE_NET_WIDTH = config["PGIE_NET_WIDTH"];
    PGIE_NET_HEIGHT = config["PGIE_NET_HEIGHT"];
    MUXER_OUTPUT_WIDTH = config["MUXER_OUTPUT_WIDTH"];
    MUXER_OUTPUT_HEIGHT = config["MUXER_OUTPUT_HEIGHT"];
    threshold_car_detection = config["threshold_car_detection"];
}

bool PrimaryNvInferManager::create_primary_nv_infer(int num_sources) {
    /* Configure the nvinferserver element using the config file. */
    primary_detector =
        gst_element_factory_make("nvinfer", "primary-nvinference-engine");
	g_object_set (G_OBJECT (primary_detector), "config-file-path", primary_nv_infer_config_file.c_str(),
    "unique-id", PRIMARY_DETECTOR_UID, NULL);

    /* Override the batch-size set in the config file with the number of
     * sources. */
    g_object_get(G_OBJECT(primary_detector), "batch-size", &pgie_batch_size,
                 NULL);
    if (pgie_batch_size != num_sources) {
        g_printerr(
            "WARNING: Overriding infer-config batch-size (%d) with number of "
            "sources (%d)\n",
            pgie_batch_size, num_sources);
        g_object_set(G_OBJECT(primary_detector), "batch-size", num_sources,
                     NULL);
    }
    if (!primary_detector) {
        g_printerr("Could not create primary detector. Exiting.\n");
        return false;
    }
    return true;
}
