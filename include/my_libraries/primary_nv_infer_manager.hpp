#include <gst/gst.h>

#include <fstream>
#include <iostream>

#include "config_manager.hpp"
// #include "gstnvdsinfer.h"
#include <algorithm>
#include <cmath>

#include "gstnvdsmeta.h"
#include "nvds_version.h"
#include "nvdsinfer_custom_impl.h"
#include "custom_gstnvdsinfer.hpp"

class PrimaryNvInferManager {
   private:
   public:
    struct Point2D {
        double x;  // X coordinate
        double y;  // Y coordinate

        // Constructor
        Point2D(double x_val = 0.0, double y_val = 0.0) : x(x_val), y(y_val) {}
    };
    GstElement *primary_detector = NULL;
    int pgie_batch_size;

    static unsigned int PGIE_NET_WIDTH;
    static unsigned int PGIE_NET_HEIGHT;
    static unsigned int MUXER_OUTPUT_WIDTH;
    static unsigned int MUXER_OUTPUT_HEIGHT;
    static unsigned int nvds_lib_major_version;
    static unsigned int nvds_lib_minor_version;
    static gint frame_number;
    static guint use_device_mem;
    std::string primary_nv_infer_config_file;
    static float threshold_car_detection;
    PrimaryNvInferManager();
    bool create_primary_nv_infer(int);
    ~PrimaryNvInferManager();
};