#include "../include/my_libraries/message_handling.hpp"

int MessageHandling::counter_total = 0;
int MessageHandling::counter_eos = 0;
int MessageHandling::counter_warning = 0;
int MessageHandling::counter_error = 0;
int MessageHandling::counter_element = 0;
bool MessageHandling::pipeline_is_run = false;

MessageHandling::MessageHandling() {}

void MessageHandling::source_remove() { g_source_remove(bus_watch_id); }

// Definition of static function
gboolean MessageHandling::bus_call(GstBus *bus, GstMessage *msg,
                                   gpointer user_data) {
    (void)bus;  // This explicitly marks it as unused
    StreamData *data = static_cast<StreamData *>(user_data);
    GMainLoop *passed_loop = data->loop;
    // GMainLoop *loop= (GMainLoop *) data;
    gchar *debug;
    GError *error;
    counter_total++;
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            counter_eos++;
            g_print("GST_MESSAGE_EOS \n");
            if (data->g_run_forever == FALSE) {
                g_print("End of stream  \n");
                pipeline_is_run = false;
                g_main_loop_quit(passed_loop);
            }
            break;
        case GST_MESSAGE_WARNING:
            counter_warning++;
            g_print("GST_MESSAGE_WARNING \n");
            gst_message_parse_warning(msg, &error, &debug);
            g_printerr("WARNING from element %s: %s\n",
                       GST_OBJECT_NAME(msg->src), error->message);
            g_free(debug);
            g_printerr("Warning: %s\n", error->message);
            g_error_free(error);
            break;
        case GST_MESSAGE_ERROR:
            counter_error++;
            g_print("GST_MESSAGE_ERROR \n");
            gst_message_parse_error(msg, &error, &debug);
            g_printerr("ERROR from element %s: %s\n", GST_OBJECT_NAME(msg->src),
                       error->message);
            if (debug) g_printerr("Error details: %s\n", debug);
            g_free(debug);
            g_error_free(error);
            g_main_loop_quit(passed_loop);
            break;
        case GST_MESSAGE_ELEMENT:
            counter_element++;
            g_print("GST_MESSAGE_ELEMENT \n");
            if (gst_nvmessage_is_stream_eos(msg)) {
                guint stream_id;
                if (gst_nvmessage_parse_stream_eos(msg, &stream_id)) {
                    //                g_print ("Got EOS from stream %d\n",
                    //                stream_id);
                    g_print("Got EOS from stream %d \n", stream_id);
                    // g_mutex_lock (&eos_lock);
                    // g_eos_list[stream_id] = TRUE;
                    // g_mutex_unlock (&eos_lock);
                    //                g_timeout_add_seconds (10, add_sources,
                    //                (gpointer) g_source_bin_list);
                    //                add_sources((gpointer) g_source_bin_list);
                    // g_print ("camera_list.at(%d).connection_status is "
                    //          "%d \n",
                    //          stream_id);
                }
            }
            break;
        default:
            // g_print ("GST_MESSAGE_TYPE (msg) is %d  ", GST_MESSAGE_TYPE
            // (msg));
            // g_message("Received message of type: %s",
            //           gst_message_type_get_name(GST_MESSAGE_TYPE(msg)));

            // g_print (gst_message_type_get_name(GST_MESSAGE_TYPE(msg)));
            // g_print("%s\n",
            // gst_message_type_get_name(GST_MESSAGE_TYPE(msg)));

            // g_print (GST_MESSAGE_TYPE (msg));

            break;
    }
    // g_print(
    //     "counter_eos is %d, counter_warning is %d, counter_error is %d, "
    //     "counter_element is %d, counter_total is %d \n",
    //     counter_eos, counter_warning, counter_error, counter_element,
    //     counter_total);
    return TRUE;
}

void MessageHandling::create_message_handler(GstElement *pipeline,
                                             bool g_run_forever,
                                             GMainLoop *loop) {
    /* add a message handler */
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    StreamData *stream_data = new StreamData{g_run_forever, loop};
    bus_watch_id = gst_bus_add_watch(bus, bus_call, stream_data);  // loop

    gst_object_unref(bus);
}