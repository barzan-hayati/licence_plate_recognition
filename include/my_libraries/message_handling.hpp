
#include <glib.h>
#include <gst-nvmessage.h>
#include <gst/gst.h>

#include <fstream>
#include <iostream>

class MessageHandling {
   private:
    typedef struct {
        bool g_run_forever;
        GMainLoop *loop;
    } StreamData;

   public:
    static gboolean bus_call(GstBus *, GstMessage *, gpointer);
    void create_message_handler(GstElement *, bool, GMainLoop *);
    GstBus *bus = NULL;
    static int counter_total;
    static int counter_eos;
    static bool pipeline_is_run;
    static int counter_warning;
    static GMutex eos_lock;
    static int counter_error;
    static int counter_element;
    guint bus_watch_id;
    MessageHandling();
    void source_remove();
    ~MessageHandling();
};