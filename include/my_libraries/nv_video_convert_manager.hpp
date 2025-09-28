#include <gst/gst.h>

class NvVideoConvertManager {
   private:
   public:
    GstElement *nvvidconv = NULL;
    NvVideoConvertManager();
    bool create_nv_video_convert();
    ~NvVideoConvertManager();
};