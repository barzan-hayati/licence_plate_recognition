#include "../include/my_libraries/pipeline_manager.hpp"
#define GPU_ID 0

double PipelineManager::tee_fps = 0.0;
double PipelineManager::video_converter_fps = 0.0;
double PipelineManager::osd_fps = 0.0;
guint64 PipelineManager::frame_count_osd_sink = 0;
guint64 PipelineManager::frame_count_fps_probe = 0;
guint64 PipelineManager::frame_count_buffer_probe = 0;
std::chrono::time_point<std::chrono::steady_clock>
    PipelineManager::last_time_osd_sink = std::chrono::steady_clock::now();
std::chrono::time_point<std::chrono::steady_clock>
    PipelineManager::last_time_fps_probe = std::chrono::steady_clock::now();
std::chrono::time_point<std::chrono::steady_clock>
    PipelineManager::last_time_buffer_probe = std::chrono::steady_clock::now();

PipelineManager::PipelineManager() { ; }

PipelineManager::PipelineManager(int num_sources, char** url_camera) {
    g_setenv("GST_DEBUG_DUMP_DOT_DIR", ".", TRUE);
    gst_init(&num_sources, &url_camera);
    g_run_forever = atoi("0");
    loop = g_main_loop_new(NULL, FALSE);
}

bool PipelineManager::create_pipeline() {
    g_mutex_init(&eos_lock);

    /* Create Pipeline element that will form a connection of other elements */
    pipeline = gst_pipeline_new("BodyDetectionPipeline");

    if (!pipeline) {
        g_printerr("pipeline could not be created. Exiting.");
        return false;
    }
    return true;
}

void PipelineManager::set_cuda_device() {
    cudaGetDevice(&current_device);
    cudaGetDeviceProperties(&prop, current_device);

    std::cout << "Device Number: " << prop.pciDeviceID << std::endl;
    std::cout << "Device name: " << prop.name << std::endl;
    std::cout << "Device Version: " << prop.major << "." << prop.minor
              << std::endl;
}

char* createName(const char* str, int num) {
    // Calculate the required length
    // Max digits in an int is about 10 (for 32-bit int), plus 1 for null
    // terminator
    int length =
        strlen(str) + 12;  // Extra space for the number and null terminator

    // Allocate memory for the new string
    char* result = new char[length];

    // Format the string
    snprintf(result, length, "%s%d", str, num);

    return result;
}

GstPadProbeReturn PipelineManager::osd_sink_fps(GstPad* pad,
                                                GstPadProbeInfo* info,
                                                gpointer user_data) {
    (void)pad;        // This explicitly marks it as unused
    (void)user_data;  // This explicitly marks it as unused

    GstBuffer* buf = (GstBuffer*)info->data;
    NvDsBatchMeta* batch_meta = gst_buffer_get_nvds_batch_meta(buf);

    frame_count_osd_sink += batch_meta->num_frames_in_batch;

    if (frame_count_osd_sink % 60 == 0) {
        std::chrono::time_point<std::chrono::steady_clock> now =
            std::chrono::steady_clock::now();
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - last_time_osd_sink)
                           .count();
        osd_fps = 60000.0 / ms;
        // g_print("FPS_osd_sink: %.2f\n", fps_osd);

        std::cout << "==================== FPS in OSD is "
                  << std::setprecision(4) << osd_fps
                  << " ====================" << std::endl;
        last_time_osd_sink = now;
    }
    return GST_PAD_PROBE_OK;
}

void PipelineManager::get_fps_osd() {
    GstElement* osd = gst_bin_get_by_name(
        GST_BIN(pipeline), "nv-onscreendisplay");  // Or "nvinfer", etc.
    GstPad* sink_pad = gst_element_get_static_pad(osd, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, osd_sink_fps, this,
                      NULL);
    gst_object_unref(sink_pad);
    gst_object_unref(osd);
}

GstPadProbeReturn PipelineManager::video_converter_src_fps(
    GstPad* pad, GstPadProbeInfo* info, gpointer user_data) {
    (void)pad;        // This explicitly marks it as unused
    (void)user_data;  // This explicitly marks it as unused
    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) {
        frame_count_fps_probe++;

        if (frame_count_fps_probe % 30 == 0) {  // Calculate FPS every 30 frames
            std::chrono::time_point<std::chrono::steady_clock>
                current_time_fps_probe = std::chrono::steady_clock::now();
            long long duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    current_time_fps_probe - last_time_fps_probe)
                    .count();
            video_converter_fps = 30000.0 / duration;
            // g_print("fps_probe FPS: %.2f\n", fps_probe);

            std::cout << "==================== FPS in video converter is "
                    << std::setprecision(4) << video_converter_fps
                    << " ====================" << std::endl;
            last_time_fps_probe = current_time_fps_probe;
        }
    }
    return GST_PAD_PROBE_OK;
}

void PipelineManager::get_fps_video_converter() {
    // 2. Add pad probe to get FPS
    GstElement* element = gst_bin_get_by_name(
        GST_BIN(pipeline), "nvvideo-converter");  // or any processing element
    GstPad* pad = gst_element_get_static_pad(element, "src");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, video_converter_src_fps,
                      NULL, NULL);
    gst_object_unref(pad);
    gst_object_unref(element);
}


bool PipelineManager::playing_pipeline(int num_sources, char** url_camera) {
    /* Set the pipeline to "playing" state */

    g_print("Now playing... \n");
    for (int i = 0; i < num_sources; i++) {
        g_print("%s, \n", url_camera[i + 1]);
    }

    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipeline),
                                      GST_DEBUG_GRAPH_SHOW_ALL,
                                      sink_manager->output_sink.c_str());
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    GstStateChangeReturn ret =
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set pipeline to playing.\n");
        gst_object_unref(pipeline);
        return false;
    }

    // Wait for state change to complete
    // GstState state;
    // GstState pending;
    // GstStateChangeReturn state_ret = gst_element_get_state(pipeline, &state,
    // &pending, 5 * GST_SECOND); if (state_ret == GST_STATE_CHANGE_SUCCESS) {
    //     g_print("***************Pipeline is in PLAYING state\n");
    // } else {
    //     g_printerr("***************Failed to change pipeline state\n");
    // }
    return true;
}

bool PipelineManager::check_playing_pipeline() {
    // Verify pipeline state (add this immediately after starting)
    GstState state;
    GstStateChangeReturn ret =
        gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to start pipeline!\n");
        return false;
    } else {
        // g_print("Pipeline state: %d (1=NULL, 2=READY, 3=PAUSED,
        // 4=PLAYING)\n",
        //         state);
        return true;
    }
}

bool PipelineManager::setup_pipeline() {
    /* Set up the pipeline */
    /* add all elements into the pipeline */
    // this is the running branch of the if statement for none-jetson platforms
    // (without a transform_jetson plugin before the sink plugin) custom_plugin
    // is dsexample pluging
    if (sink_manager->display_output < 3) {
        gst_bin_add_many(
            GST_BIN(pipeline), primary_nv_infer_manager->primary_detector,
            tiler_manager->tiler, queue_array[2].queue,
            nv_video_convert_manager->nvvidconv, nv_osd_manager->nvosd, 
            sink_manager->sink, NULL);
        if (!gst_element_link_many(
                streammux_manager->streammux,
                nv_video_convert_manager->nvvidconv, 
                primary_nv_infer_manager->primary_detector,
                tiler_manager->tiler, nv_osd_manager->nvosd, sink_manager->sink, NULL)) {
            g_printerr("Could not link elements!.\n");
            return false;
        }
    } else {
        gst_bin_add_many(
            GST_BIN(pipeline), primary_nv_infer_manager->primary_detector,
            tiler_manager->tiler, queue_array[2].queue,
            nv_video_convert_manager->nvvidconv, nv_osd_manager->nvosd,
            sink_manager->nvvidconv_postosd, sink_manager->caps,
            sink_manager->encoder, sink_manager->rtppay, sink_manager->sink,
            NULL);
        if (!gst_element_link_many(
                streammux_manager->streammux,
                nv_video_convert_manager->nvvidconv,
                primary_nv_infer_manager->primary_detector,
                tiler_manager->tiler, nv_osd_manager->nvosd, 
                sink_manager->nvvidconv_postosd,
                sink_manager->caps, sink_manager->encoder,
                sink_manager->rtppay, sink_manager->sink, NULL)) {
            g_printerr("Could not link elements!.\n");
            return false;
        }
    }
    return true;
}

gboolean PipelineManager::check_pipeline_state(gpointer user_data) {
    GstElement* pipeline = (GstElement*)user_data;
    GstState state;
    gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
    // g_print("Pipeline state (periodic check): %d\n", state);
    return G_SOURCE_CONTINUE;  // Keep timer active
}

gboolean PipelineManager::event_thread_func(gpointer arg) {
    DataPointer* data = static_cast<DataPointer*>(arg);
    // show which source camera. called every 4o ms.
    // if (value==true){
    // gst_element_set_state (pipeline, GST_STATE_PAUSED);
    // gst_element_set_state (pipeline, GST_STATE_PLAYING);
    // IMPORTANT:

    guint show_source = -1;  // which source to show: should be an integer
                             // between the range (0, num_sources-1)
    // to show the selected source number only.
    //  choose show_source=-1 to show the results for all the videos at once
    g_object_set(G_OBJECT(data->tiler_manager->tiler), "show-source",
                 show_source, NULL);
    return true;
}

bool PipelineManager::create_pipeline_elements(int num_sources,
                                               char** url_camera) {
    streammux_manager->create_streammux(num_sources);
    set_cuda_device();
    gst_bin_add(GST_BIN(pipeline), streammux_manager->streammux);
    // for each source generate a pad for the source, generate another pad
    // for streammux, then connect the source pad to the pad of streammux

    for (guint i = 0; i < (guint)num_sources; i++) {
        GstElement* source_bin;
        //        GstElement *source_bin = create_uridecode_bin (i,
        //        const_cast<char*>(first_video.c_str()));
        g_print("Trying to create uridecode_bin for %s  \n",
                url_camera[i + 1]);

        source_bin = SourceBin::create_uridecode_bin(
            i, url_camera[i + 1], streammux_manager->streammux, prop);
        if (!source_bin) {
            g_printerr("Failed to create source bin for %s. Exiting.\n",
                        url_camera[i + 1]);
            return false;
        }
        // g_source_bin_list[i] = source_bin;
        gst_bin_add(GST_BIN(pipeline), source_bin);
    }

    tiler_manager->create_tiler(num_sources,
                                streammux_manager->MUXER_OUTPUT_WIDTH,
                                streammux_manager->MUXER_OUTPUT_HEIGHT);
    nv_video_convert_manager->create_nv_video_convert();
    nv_osd_manager->create_nv_osd();

    /* Add queue elements between every two elements */
    const char* base = "queue";
    for (int i = 0; i < 5; i++) {
        char* name = createName(base, i);
        queue_array[i] = QueueManager(name);
    }

    nv_ds_logger_manager->create_nv_ds_logger();
    sink_manager->create_sink(prop, rtsp_streaming_manager->host,
                              rtsp_streaming_manager->updsink_port_num);
    sink_manager->create_fake_sink();

    // Create Context for Object Encoding.
    // Creates and initializes an object encoder context.
    // This context manages resources such as GPU memory, encoders, and
    // parameters (resolution, quality, scaling, etc.) needed for encoding
    // objects into images. create this once per pipeline.
    NvDsObjEncCtxHandle obj_ctx_handle = nvds_obj_enc_create_context(GPU_ID);
    if (!obj_ctx_handle) {
        g_print("Unable to create context\n");
        return -1;
    }


    primary_nv_infer_manager->create_primary_nv_infer(num_sources);

    message_handling->create_message_handler(pipeline, g_run_forever, loop);
    setup_pipeline();

    get_fps_video_converter();
    get_fps_osd();

    /* Add probe to get informed of the meta data generated, we add probe to
     * the source pad of PGIE's next queue element, since by that time, PGIE's
     * buffer would have had got tensor metadata. */

    new_mux_str = g_getenv("USE_NEW_NVSTREAMMUX");
    use_new_mux = !g_strcmp0(new_mux_str, "yes");


    auto start = std::chrono::system_clock::now();
    status_playing = playing_pipeline(num_sources, url_camera);
    if (status_playing == false) {
        return -1;
    }

    status_playing = check_playing_pipeline();
    if (status_playing == false) {
        return -1;
    }

    rtsp_streaming_manager->start_rtsp_streaming();

    /* Wait till pipeline encounters an error or EOS */
    g_print("Running... \n");
    // event executed every 40 ms for selecting show_source

    DataPointer* pointer_data = new DataPointer{tiler_manager};
    g_timeout_add(40, event_thread_func, pointer_data);  // NULL
    g_timeout_add_seconds(1, check_pipeline_state,
                          pipeline);  // Check every 5 seconds

    message_handling->pipeline_is_run = true;

    g_main_loop_run(loop);

    auto end = std::chrono::system_clock::now();
    std::cout << " Overall running time = "
              << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                       start)
                     .count()
              << "us" << std::endl;
    /* Out of the main loop, clean up nicely */
    g_print("Returned, stopping playback \n");

    nvds_obj_enc_destroy_context(obj_ctx_handle);
    /* Release the request pads from the tee, and unref them */

    gst_element_set_state(pipeline, GST_STATE_NULL);
    g_print("Deleting pipeline \n");
    gst_object_unref(GST_OBJECT(pipeline));
    rtsp_streaming_manager->destroy_sink_bin();
    // g_source_remove (bus_watch_id);
    message_handling->source_remove();
    g_main_loop_unref(loop);
    gst_deinit();
    // g_free (g_source_bin_list);
    //    g_free (uri);
    g_mutex_clear(&eos_lock);

    return true;
}