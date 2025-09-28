#include <gst/gst.h>

#include <fstream>
#include <iostream>

#include "config_manager.hpp"

class StreammuxManager {
   private:
   public:
    GstElement *streammux = NULL;
    int MUXER_OUTPUT_WIDTH;
    int MUXER_OUTPUT_HEIGHT;
    StreammuxManager();
    bool create_streammux(int);
    ~StreammuxManager();
};