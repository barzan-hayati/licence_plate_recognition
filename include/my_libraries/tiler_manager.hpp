#include <gst/gst.h>

#include <cmath>

class TilerManager {
   private:
    guint tiler_rows, tiler_columns;

   public:
    GstElement *tiler = NULL;
    TilerManager();
    bool create_tiler(int num_sources, const int, const int);
    ~TilerManager();
};