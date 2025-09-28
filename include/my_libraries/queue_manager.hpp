#include <gst/gst.h>

class QueueManager {
   private:
   public:
    GstElement* queue = NULL;
    QueueManager();
    QueueManager(char*);
    ~QueueManager();
};