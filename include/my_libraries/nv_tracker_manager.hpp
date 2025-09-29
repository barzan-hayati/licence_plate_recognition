#include <gst/gst.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <tuple>  // for std::tuple
#include <vector>

#include "config_manager.hpp"
#include "gstnvdsmeta.h"
#include "nvdsmeta.h"
#include "nvdsmeta_schema.h"

class NvTrackerManager {
   private:
   public:
    GstElement *tracker = NULL;
    static gint frame_number;
    std::string ll_config_file;
    std::string ll_lib_file;
    NvTrackerManager();
    ~NvTrackerManager();
    bool create_nv_tracker();
};