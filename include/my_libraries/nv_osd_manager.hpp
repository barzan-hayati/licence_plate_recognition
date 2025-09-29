#include <gst/gst.h>

#include <fstream>
#include <iostream>

#include "gstnvdsmeta.h"
#include "config_manager.hpp"
#include "custom_gstnvdsinfer.hpp"
#include "nvdsmeta_schema.h"

class NvOsdManager {
   private:
   public:
    GstElement *nvosd = NULL;
    static bool write_full_frame_to_disk, write_cropped_objects_to_disk;
    NvOsdManager();
    bool create_nv_osd();
    ~NvOsdManager();
    static gint frame_number;
};