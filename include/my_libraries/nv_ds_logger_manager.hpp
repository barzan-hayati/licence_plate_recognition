#include <gst/gst.h>

class NvDsLoggerManager {
   private:
   public:
    GstElement *nvdslogger = NULL;
    NvDsLoggerManager();
    bool create_nv_ds_logger();
    ~NvDsLoggerManager();
};