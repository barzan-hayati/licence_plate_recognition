#include "../include/my_libraries/nv_osd_manager.hpp"

#define NVDS_USER_EMBEDDING_VECTOR_META \
    (nvds_get_user_meta_type(           \
        const_cast<gchar *>("NVIDIA.NVINFER.EMBEDDING_VECTOR.USER_META")))
// #define ENABLE_DUMP_FILE
#ifdef ENABLE_DUMP_FILE
FILE *fp;
char fileObjNameString[1024];
#endif

// #define MEASURE_ENCODE_TIME
#ifdef MEASURE_ENCODE_TIME
#include <sys/time.h>
#define START_PROFILE           \
    {                           \
        struct timeval t1, t2;  \
        double elapsedTime = 0; \
        gettimeofday(&t1, NULL);

#define STOP_PROFILE(X)                                \
    gettimeofday(&t2, NULL);                           \
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;    \
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0; \
    printf("%s ElaspedTime=%f ms\n", X, elapsedTime);  \
    }

#else
#define START_PROFILE
#define STOP_PROFILE(X)
#endif

#define SET_GPU_ID(object, gpu_id) \
    g_object_set(G_OBJECT(object), "gpu-id", gpu_id, NULL);
#define GPU_ID 0
#define OSD_PROCESS_MODE \
    1  // use GPU to draw rectangles, keypoints and text on frame if
       // OSD_PROCESS_MODE set to 1
#define OSD_DISPLAY_TEXT 1
#define MAX_DISPLAY_LEN 64
#define MAX_TIME_STAMP_LEN 32
#define PGIE_CLASS_ID_PERSON 0
#define FACE_CLASS_ID 1
#define EMBEDDING_VECTOR_SIZE 512

gint msg2p_meta =
    1;  //"Type of message schema (0=Full, 1=minimal, 2=protobuf), default=0

gint NvOsdManager::frame_number = 0;
bool NvOsdManager::write_full_frame_to_disk = false;
bool NvOsdManager::write_cropped_objects_to_disk = false;

NvOsdManager::NvOsdManager() {
    const auto &config = ConfigManager::get_instance().get_config();
    write_full_frame_to_disk =
        config.at("write_full_frame_to_disk").get<bool>();
    write_cropped_objects_to_disk =
        config.at("write_cropped_objects_to_disk").get<bool>();
}

bool NvOsdManager::create_nv_osd() {
    /* Create OSD to draw on the converted RGBA buffer */
    nvosd = gst_element_factory_make("nvdsosd", "nv-onscreendisplay");
    /* Finally render the osd output */
    g_object_set(G_OBJECT(nvosd), "process-mode", OSD_PROCESS_MODE,
                 "display-text", OSD_DISPLAY_TEXT, NULL);
    SET_GPU_ID(nvosd, GPU_ID);

    if (!nvosd) {
        g_printerr("Unable to create NVOSD. Exiting.\n");
        return false;
    }
    return true;
}

// Attach probe to a pad in the pipeline
void NvOsdManager::attach_probe_to_sink_nvosd(
    NvDsObjEncCtxHandle obj_ctx_handle) {
    GstPad *sink_pad = gst_element_get_static_pad(nvosd, "sink");
    if (!sink_pad) {
        std::cerr << "Unable to get nvosd sink pad\n";
        return;
    }

    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
                      osd_sink_pad_buffer_probe, (gpointer)obj_ctx_handle,
                      NULL);
    gst_object_unref(sink_pad);
}

void NvOsdManager::save_full_frame(NvDsFrameMeta *frame_meta) {
    char fileFrameNameString[FILE_NAME_SIZE];
    const char *osd_string = "OSD";

    /* For Demonstration Purposes we are writing metadata to jpeg images of
     * the first 10 frames only.
     * The files generated have an 'OSD' prefix. */
    NvDsUserMetaList *usrMetaList = frame_meta->frame_user_meta_list;
    FILE *file;
    int stream_num = 0;
    while (usrMetaList != NULL) {
        NvDsUserMeta *usrMetaData = (NvDsUserMeta *)usrMetaList->data;
        if (usrMetaData->base_meta.meta_type == NVDS_CROP_IMAGE_META) {
            snprintf(fileFrameNameString, FILE_NAME_SIZE, "%s_frame_%d_%d.jpg",
                     osd_string, frame_number, stream_num++);
            NvDsObjEncOutParams *enc_jpeg_image =
                (NvDsObjEncOutParams *)usrMetaData->user_meta_data;
            /* Write to File */
            file = fopen(fileFrameNameString, "wb");
            fwrite(enc_jpeg_image->outBuffer, sizeof(uint8_t),
                   enc_jpeg_image->outLen, file);
            fclose(file);
        }
        usrMetaList = usrMetaList->next;
    }
}

NvDsObjEncOutParams *NvOsdManager::get_full_frame(NvDsFrameMeta *frame_meta) {
    NvDsObjEncOutParams *enc_jpeg_image = NULL;
    NvDsUserMetaList *usrMetaList = frame_meta->frame_user_meta_list;
    while (usrMetaList != NULL) {
        NvDsUserMeta *usrMetaData = (NvDsUserMeta *)usrMetaList->data;
        if (usrMetaData->base_meta.meta_type == NVDS_CROP_IMAGE_META) {
            enc_jpeg_image = (NvDsObjEncOutParams *)usrMetaData->user_meta_data;
        }
        usrMetaList = usrMetaList->next;
    }
    return enc_jpeg_image;
}

void NvOsdManager::save_cropped_objects(NvDsFrameMeta *frame_meta,
                                        NvDsObjectMeta *obj_meta,
                                        guint num_rects) {
    const char *osd_string = "OSD";
    char fileObjNameString[FILE_NAME_SIZE];

    /* For Demonstration Purposes we are writing metadata to jpeg images of
     * faces or persons for the first 100 frames only.
     * The files generated have a 'OSD' prefix. */
    NvDsUserMetaList *usrMetaList = obj_meta->obj_user_meta_list;
    FILE *file;
    while (usrMetaList != NULL) {
        NvDsUserMeta *usrMetaData = (NvDsUserMeta *)usrMetaList->data;
        if (usrMetaData->base_meta.meta_type == NVDS_CROP_IMAGE_META) {
            NvDsObjEncOutParams *enc_jpeg_image =
                (NvDsObjEncOutParams *)usrMetaData->user_meta_data;

            snprintf(fileObjNameString, FILE_NAME_SIZE, "%s_%d_%d_%d_%s.jpg",
                     osd_string, frame_number, frame_meta->batch_id, num_rects,
                     obj_meta->obj_label);
            /* Write to File */
            file = fopen(fileObjNameString, "wb");
            fwrite(enc_jpeg_image->outBuffer, sizeof(uint8_t),
                   enc_jpeg_image->outLen, file);
            fclose(file);
            usrMetaList = NULL;
        } else {
            usrMetaList = usrMetaList->next;
        }
    }
}

NvDsObjEncOutParams *NvOsdManager::get_cropped_objects(
    NvDsObjectMeta *obj_meta) {
    NvDsObjEncOutParams *enc_jpeg_image = NULL;
    NvDsUserMetaList *usrMetaList = obj_meta->obj_user_meta_list;
    while (usrMetaList != NULL) {
        NvDsUserMeta *usrMetaData = (NvDsUserMeta *)usrMetaList->data;
        if (usrMetaData->base_meta.meta_type == NVDS_CROP_IMAGE_META) {
            enc_jpeg_image = (NvDsObjEncOutParams *)usrMetaData->user_meta_data;
            usrMetaList = NULL;
        } else {
            usrMetaList = usrMetaList->next;
        }
    }
    return enc_jpeg_image;
}

/* This is the buffer probe function that we have registered on the sink pad
 * of the OSD element. All the infer elements in the pipeline shall attach
 * their metadata to the GstBuffer, here we will iterate & process the metadata
 * forex: class ids to strings, counting of class_id objects etc. */
GstPadProbeReturn NvOsdManager::osd_sink_pad_buffer_probe(GstPad *pad,
                                                          GstPadProbeInfo *info,
                                                          gpointer u_data) {
    (void)pad;
    (void)u_data;
    GstBuffer *buf = (GstBuffer *)info->data;
    guint num_rects = 0;
    guint person_count = 0;
    NvDsObjectMeta *obj_meta = NULL;
    NvDsMetaList *l_frame = NULL;
    NvDsMetaList *l_obj = NULL;
    NvDsDisplayMeta *display_meta = NULL;

    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);

    for (l_frame = batch_meta->frame_meta_list; l_frame != NULL;
         l_frame = l_frame->next) {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);
        int offset = 0;
        if (write_full_frame_to_disk == true) save_full_frame(frame_meta);
        for (l_obj = frame_meta->obj_meta_list; l_obj != NULL;
             l_obj = l_obj->next) {
            obj_meta = (NvDsObjectMeta *)(l_obj->data);
            if (obj_meta->class_id == PGIE_CLASS_ID_PERSON) {
                person_count++;
                num_rects++;
            }
            if (write_cropped_objects_to_disk == true)
                save_cropped_objects(frame_meta, obj_meta, num_rects);
        }
        display_meta = nvds_acquire_display_meta_from_pool(batch_meta);
        NvOSD_TextParams *txt_params = &display_meta->text_params[0];
        display_meta->num_labels = 1;
        txt_params->display_text = (gchar *)g_malloc0(MAX_DISPLAY_LEN);
        offset = snprintf(txt_params->display_text, MAX_DISPLAY_LEN,
                          "Person = %d ", person_count);
        (void)offset;

        /* Now set the offsets where the string should appear */
        txt_params->x_offset = 10;
        txt_params->y_offset = 12;

        /* Font , font-color and font-size */
        txt_params->font_params.font_name = (gchar *)"Serif";
        txt_params->font_params.font_size = 10;
        txt_params->font_params.font_color.red = 1.0;
        txt_params->font_params.font_color.green = 1.0;
        txt_params->font_params.font_color.blue = 1.0;
        txt_params->font_params.font_color.alpha = 1.0;

        /* Text background color */
        txt_params->set_bg_clr = 1;
        txt_params->text_bg_clr.red = 0.0;
        txt_params->text_bg_clr.green = 0.0;
        txt_params->text_bg_clr.blue = 0.0;
        txt_params->text_bg_clr.alpha = 1.0;

        nvds_add_display_meta_to_frame(frame_meta, display_meta);
    }
    // g_print(
    //     "In OSD sink "
    //     "Frame Number = %d "
    //     "Person Count = %d\n",
    //     frame_number, person_count);

    // frame_number++;
    return GST_PAD_PROBE_OK;
}

// Attach probe to a pad in the pipeline
void NvOsdManager::attach_probe_to_src_nvosd(
    NvDsObjEncCtxHandle obj_ctx_handle) {
    GstPad *src_pad = gst_element_get_static_pad(nvosd, "src");
    if (!src_pad) {
        std::cerr << "Unable to get nvosd src pad\n";
        return;
    }

    if (msg2p_meta == 0) {  // generate payload using eventMsgMeta
        gst_pad_add_probe(src_pad, GST_PAD_PROBE_TYPE_BUFFER,
                          osd_src_pad_buffer_metadata_probe, NULL, NULL);
    } else {  // generate payload using NVDS_CUSTOM_MSG_BLOB
        gst_pad_add_probe(src_pad, GST_PAD_PROBE_TYPE_BUFFER,
                          osd_src_pad_buffer_image_probe,
                          (gpointer)obj_ctx_handle, NULL);
    }
}

GstPadProbeReturn NvOsdManager::osd_src_pad_buffer_probe(GstPad *,
                                                         GstPadProbeInfo *,
                                                         gpointer) {
    return GST_PAD_PROBE_OK;
}

void NvOsdManager::meta_free_func(gpointer data, gpointer user_data) {
    (void)user_data;
    NvDsUserMeta *user_meta = (NvDsUserMeta *)data;
    NvDsEventMsgMeta *srcMeta = (NvDsEventMsgMeta *)user_meta->user_meta_data;

    g_free(srcMeta->ts);
    g_free(srcMeta->sensorStr);

    if (srcMeta->objSignature.size > 0) {
        g_free(srcMeta->objSignature.signature);
        srcMeta->objSignature.size = 0;
    }

    if (srcMeta->objectId) {
        g_free(srcMeta->objectId);
    }

    if (srcMeta->extMsgSize > 0) {
        if (srcMeta->objType == NVDS_OBJECT_TYPE_FACE) {
            NvDsFaceObject *obj = (NvDsFaceObject *)srcMeta->extMsg;
            if (obj->cap) g_free(obj->cap);
            if (obj->eyecolor) g_free(obj->eyecolor);
            if (obj->facialhair) g_free(obj->facialhair);
            if (obj->gender) g_free(obj->gender);
            if (obj->glasses) g_free(obj->glasses);
            if (obj->hair) g_free(obj->hair);
            if (obj->name) g_free(obj->name);
        } else if (srcMeta->objType == NVDS_OBJECT_TYPE_PERSON) {
            NvDsPersonObject *obj = (NvDsPersonObject *)srcMeta->extMsg;

            if (obj->gender) g_free(obj->gender);
            if (obj->cap) g_free(obj->cap);
            if (obj->hair) g_free(obj->hair);
            if (obj->apparel) g_free(obj->apparel);
        }
        g_free(srcMeta->extMsg);
        srcMeta->extMsgSize = 0;
    }
    g_free(user_meta->user_meta_data);
    user_meta->user_meta_data = NULL;
}

gpointer NvOsdManager::meta_copy_func(gpointer data, gpointer user_data) {
    (void)user_data;
    NvDsUserMeta *user_meta = (NvDsUserMeta *)data;
    NvDsEventMsgMeta *srcMeta = (NvDsEventMsgMeta *)user_meta->user_meta_data;
    NvDsEventMsgMeta *dstMeta = NULL;

    dstMeta = (NvDsEventMsgMeta *)g_memdup2(srcMeta, sizeof(NvDsEventMsgMeta));

    if (srcMeta->ts) dstMeta->ts = g_strdup(srcMeta->ts);

    if (srcMeta->sensorStr) dstMeta->sensorStr = g_strdup(srcMeta->sensorStr);

    if (srcMeta->objSignature.size > 0) {
        dstMeta->objSignature.signature = (gdouble *)g_memdup2(
            srcMeta->objSignature.signature, srcMeta->objSignature.size);
        dstMeta->objSignature.size = srcMeta->objSignature.size;
    }

    if (srcMeta->objectId) {
        dstMeta->objectId = g_strdup(srcMeta->objectId);
    }

    if (srcMeta->extMsgSize > 0) {
        if (srcMeta->objType == NVDS_OBJECT_TYPE_FACE) {
            NvDsFaceObject *srcObj = (NvDsFaceObject *)srcMeta->extMsg;
            NvDsFaceObject *obj =
                (NvDsFaceObject *)g_malloc0(sizeof(NvDsFaceObject));
            if (srcObj->age) obj->age = srcObj->age;
            if (srcObj->cap) obj->cap = g_strdup(srcObj->cap);
            if (srcObj->eyecolor) obj->eyecolor = g_strdup(srcObj->eyecolor);
            if (srcObj->facialhair)
                obj->facialhair = g_strdup(srcObj->facialhair);
            if (srcObj->gender) obj->gender = g_strdup(srcObj->gender);
            if (srcObj->glasses) obj->glasses = g_strdup(srcObj->glasses);
            if (srcObj->hair) obj->hair = g_strdup(srcObj->hair);
            //   if (srcObj->mask)
            //     obj->mask = g_strdup (srcObj->mask);
            if (srcObj->name) obj->name = g_strdup(srcObj->name);

            dstMeta->extMsg = obj;
            dstMeta->extMsgSize = sizeof(NvDsFaceObject);
        } else if (srcMeta->objType == NVDS_OBJECT_TYPE_PERSON) {
            NvDsPersonObject *srcObj = (NvDsPersonObject *)srcMeta->extMsg;
            NvDsPersonObject *obj =
                (NvDsPersonObject *)g_malloc0(sizeof(NvDsPersonObject));

            obj->age = srcObj->age;

            if (srcObj->gender) obj->gender = g_strdup(srcObj->gender);
            if (srcObj->cap) obj->cap = g_strdup(srcObj->cap);
            if (srcObj->hair) obj->hair = g_strdup(srcObj->hair);
            if (srcObj->apparel) obj->apparel = g_strdup(srcObj->apparel);
            dstMeta->extMsg = obj;
            dstMeta->extMsgSize = sizeof(NvDsPersonObject);
        }
    }

    return dstMeta;
}

void NvOsdManager::generate_ts_rfc3339(char *buf, int buf_size) {
    time_t tloc;
    struct tm tm_log;
    struct timespec ts;
    char strmsec[6];  //.nnnZ\0

    clock_gettime(CLOCK_REALTIME, &ts);
    memcpy(&tloc, (void *)(&ts.tv_sec), sizeof(time_t));
    gmtime_r(&tloc, &tm_log);
    strftime(buf, buf_size, "%Y-%m-%dT%H:%M:%S", &tm_log);
    int ms = ts.tv_nsec / 1000000;
    g_snprintf(strmsec, sizeof(strmsec), ".%.3dZ", ms);
    strncat(buf, strmsec, buf_size);
}

void NvOsdManager::generate_face_meta(gpointer data) {
    NvDsFaceObjectExt *obj = (NvDsFaceObjectExt *)data;

    obj->age = 25;
    obj->cap = g_strdup("cap");
    obj->eyecolor = g_strdup("eyecolor");
    obj->facialhair = g_strdup("facialhair");
    obj->gender = g_strdup("gender");
    obj->glasses = g_strdup("glasses");
    obj->hair = g_strdup("hair");
    //   obj->mask = g_strdup ("mask");
    obj->name = g_strdup("name");
}

void NvOsdManager::generate_person_meta(gpointer data) {
    NvDsPersonObject *obj = (NvDsPersonObject *)data;
    obj->age = 45;
    obj->cap = g_strdup("none");
    obj->hair = g_strdup("black");
    obj->gender = g_strdup("male");
    obj->apparel = g_strdup("formal");
    //   obj->mask =  g_strdup ("formal");
}

void NvOsdManager::generate_event_msg_meta(gpointer data, gint class_id,
                                           NvDsObjectMeta *obj_params) {
    NvDsEventMsgMeta *meta = (NvDsEventMsgMeta *)data;
    meta->sensorId = 0;
    meta->placeId = 0;
    meta->moduleId = 0;
    meta->sensorStr = g_strdup("sensor-0");

    meta->ts = (gchar *)g_malloc0(MAX_TIME_STAMP_LEN + 1);
    meta->objectId = (gchar *)g_malloc0(MAX_LABEL_SIZE);

    strncpy(meta->objectId, obj_params->obj_label, MAX_LABEL_SIZE);

    generate_ts_rfc3339(meta->ts, MAX_TIME_STAMP_LEN);

    /*
     * This demonstrates how to attach custom objects.
     * Any custom object as per requirement can be generated and attached
     * like NvDsFaceObject / NvDsPersonObject. Then that object should
     * be handled in payload generator library (nvmsgconv.cpp) accordingly.
     */
    if (class_id == FACE_CLASS_ID) {
        meta->type = NVDS_EVENT_MOVING;
        meta->objType = NVDS_OBJECT_TYPE_FACE;
        meta->objClassId = FACE_CLASS_ID;

        NvDsFaceObject *obj =
            (NvDsFaceObject *)g_malloc0(sizeof(NvDsFaceObject));
        generate_face_meta(obj);

        meta->extMsg = obj;
        meta->extMsgSize = sizeof(NvDsFaceObject);
    } else if (class_id == PGIE_CLASS_ID_PERSON) {
        meta->type = NVDS_EVENT_ENTRY;
        meta->objType = NVDS_OBJECT_TYPE_PERSON;
        meta->objClassId = PGIE_CLASS_ID_PERSON;

        NvDsPersonObject *obj =
            (NvDsPersonObject *)g_malloc0(sizeof(NvDsPersonObject));
        generate_person_meta(obj);

        meta->extMsg = obj;
        meta->extMsgSize = sizeof(NvDsPersonObject);
    }
}

void NvOsdManager::event_message_meta(
    NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta,
    NvDsObjectMeta *obj_meta, float *user_meta_data,
    std::vector<NvDsObjEncOutParams> encoded_images) {
    NvDsObjEncOutParams *face_frame = &encoded_images.front();
    NvDsObjEncOutParams *full_frame = &encoded_images.back();
    if (encoded_images.size() == 3) {
        NvDsObjEncOutParams *body_frame = &encoded_images[1];
        (void)body_frame;
    }

    gchar *face_encoded_data =
        g_base64_encode(face_frame->outBuffer, face_frame->outLen);
    gchar *full_frame_encoded_data =
        g_base64_encode(full_frame->outBuffer, full_frame->outLen);
    // gchar *combined = g_strconcat(face_encoded_data, ";",
    // full_frame_encoded_data, NULL);

    NvDsEventMsgMeta *msg_meta =
        (NvDsEventMsgMeta *)g_malloc0(sizeof(NvDsEventMsgMeta));
    msg_meta->bbox.top = obj_meta->rect_params.top;
    msg_meta->bbox.left = obj_meta->rect_params.left;
    msg_meta->bbox.width = obj_meta->rect_params.width;
    msg_meta->bbox.height = obj_meta->rect_params.height;
    msg_meta->frameId = frame_number;
    msg_meta->trackingId = obj_meta->object_id;
    msg_meta->confidence = obj_meta->confidence;
    msg_meta->embedding.embedding_vector = user_meta_data;
    msg_meta->embedding.embedding_length = EMBEDDING_VECTOR_SIZE;
    // msg_meta->otherAttrs = combined;
    msg_meta->otherAttrs =
        g_strdup_printf("{\"face_frame\":\"%s\",\"full_frame\":\"%s\"}",
                        face_encoded_data, full_frame_encoded_data);
    // msg_meta->otherAttrs = g_strdup_printf(
    // "[\"customMessage\":\"%s\"]",
    // "face_encoded_data");
    // msg_meta->otherAttrs = g_strdup("test123;test456");

    generate_event_msg_meta(msg_meta, obj_meta->class_id, obj_meta);

    NvDsUserMeta *user_event_meta =
        nvds_acquire_user_meta_from_pool(batch_meta);
    if (user_event_meta) {
        user_event_meta->user_meta_data = (void *)msg_meta;
        user_event_meta->base_meta.meta_type = NVDS_EVENT_MSG_META;
        user_event_meta->base_meta.copy_func = (NvDsMetaCopyFunc)meta_copy_func;
        user_event_meta->base_meta.release_func =
            (NvDsMetaReleaseFunc)meta_free_func;
        nvds_add_user_meta_to_frame(frame_meta, user_event_meta);
    } else {
        g_print("Error in attaching event meta to buffer\n");
    }
}

/* osd_sink_pad_buffer_probe  will extract metadata received on OSD sink pad
 * and update params for drawing rectangle, object information etc. */
GstPadProbeReturn NvOsdManager::osd_src_pad_buffer_metadata_probe(
    GstPad *pad, GstPadProbeInfo *info, gpointer u_data) {
    (void)pad;
    (void)u_data;
    GstBuffer *buf = (GstBuffer *)info->data;
    NvDsFrameMeta *frame_meta = NULL;
    NvOSD_TextParams *txt_params = NULL;
    (void)txt_params;
    guint face_count = 0;
    guint person_count = 0;
    NvDsMetaList *l_frame, *l_obj;

    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);
    if (!batch_meta) {
        // No batch meta attached.
        return GST_PAD_PROBE_OK;
    }

    for (l_frame = batch_meta->frame_meta_list; l_frame;
         l_frame = l_frame->next) {
        frame_meta = (NvDsFrameMeta *)l_frame->data;

        if (frame_meta == NULL) {
            // Ignore Null frame meta.
            continue;
        }

        for (l_obj = frame_meta->obj_meta_list; l_obj; l_obj = l_obj->next) {
            NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)l_obj->data;

            if (obj_meta == NULL) {
                // Ignore Null object.
                continue;
            }

            //   txt_params = &(obj_meta->text_params);
            //   if (txt_params->display_text)
            //     g_free (txt_params->display_text);

            //   txt_params->display_text = (char *)g_malloc0 (MAX_DISPLAY_LEN);

            //   g_snprintf (txt_params->display_text, MAX_DISPLAY_LEN, "%s ",
            //       pgie_classes_str[obj_meta->class_id]);

            //   if (obj_meta->class_id == FACE_CLASS_ID)
            //     face_count++;
            //   if (obj_meta->class_id == PGIE_CLASS_ID_PERSON)
            //     person_count++;

            //   /* Now set the offsets where the string should appear */
            //   txt_params->x_offset = obj_meta->rect_params.left;
            //   txt_params->y_offset = obj_meta->rect_params.top - 25;

            //   /* Font , font-color and font-size */
            //   txt_params->font_params.font_name = (char *) "Serif";
            //   txt_params->font_params.font_size = 10;
            //   txt_params->font_params.font_color.red = 1.0;
            //   txt_params->font_params.font_color.green = 1.0;
            //   txt_params->font_params.font_color.blue = 1.0;
            //   txt_params->font_params.font_color.alpha = 1.0;

            //   /* Text background color */
            //   txt_params->set_bg_clr = 1;
            //   txt_params->text_bg_clr.red = 0.0;
            //   txt_params->text_bg_clr.green = 0.0;
            //   txt_params->text_bg_clr.blue = 0.0;
            //   txt_params->text_bg_clr.alpha = 1.0;

            /*
             * Ideally NVDS_EVENT_MSG_META should be attached to buffer by the
             * component implementing detection / recognition logic.
             * Here it demonstrates how to use / attach that meta data.
             */

            std::vector<NvDsObjEncOutParams> encoded_images;
            NvDsObjEncOutParams *enc_jpeg_image = NULL;
            NvDsUserMetaList *usrMetaList = obj_meta->obj_user_meta_list;
            int num_encode = 0;
            bool is_meta_type_NVDS_CROP_IMAGE_META = false;
            while (usrMetaList != NULL) {
                NvDsUserMeta *user_meta = (NvDsUserMeta *)usrMetaList->data;
                if (user_meta->base_meta.meta_type == NVDS_CROP_IMAGE_META) {
                    enc_jpeg_image =
                        (NvDsObjEncOutParams *)user_meta->user_meta_data;
                    encoded_images.push_back(*enc_jpeg_image);
                    num_encode++;
                    // usrMetaList = NULL;
                    is_meta_type_NVDS_CROP_IMAGE_META = true;
                }
                // else {
                //     usrMetaList = usrMetaList->next;
                // }
                usrMetaList = usrMetaList->next;
            }

            // // Print results
            // for (const auto &item : encoded_images) {
            //     std::cout << " (size=" << item.outLen << ")\n";
            // }

            if (is_meta_type_NVDS_CROP_IMAGE_META == true) {
                enc_jpeg_image = get_full_frame(frame_meta);
                encoded_images.push_back(*enc_jpeg_image);
            }

            // Sort by size (ascending)
            std::sort(
                encoded_images.begin(), encoded_images.end(),
                [](const NvDsObjEncOutParams &a, const NvDsObjEncOutParams &b) {
                    return a.outLen < b.outLen;
                });

            NvDsUserMeta *user_meta = NULL;
            NvDsMetaList *l_user_meta = NULL;
            float *user_meta_data = NULL;
            bool is_meta_type_NVOSD_embedding_vector = false;
            for (l_user_meta = obj_meta->obj_user_meta_list;
                 l_user_meta != NULL; l_user_meta = l_user_meta->next) {
                user_meta = (NvDsUserMeta *)(l_user_meta->data);
                if (user_meta->base_meta.meta_type ==
                    NVDS_USER_EMBEDDING_VECTOR_META) {
                    is_meta_type_NVOSD_embedding_vector = true;
                    user_meta_data = (float *)user_meta->user_meta_data;
                }
            }

            if (is_meta_type_NVOSD_embedding_vector == true &&
                encoded_images.size() >= 2) {
                event_message_meta(batch_meta, frame_meta, obj_meta,
                                   user_meta_data, encoded_images);
            }
        }
    }
    g_print(
        "Frame Number = %d "
        "Face Count = %d Person Count = %d\n",
        frame_number, face_count, person_count);
    frame_number++;

    return GST_PAD_PROBE_OK;
}

gpointer NvOsdManager::meta_copy_func_custom(gpointer data,
                                             gpointer user_data) {
    (void)user_data;
    NvDsUserMeta *user_meta = (NvDsUserMeta *)data;
    NvDsCustomMsgInfo *srcMeta = (NvDsCustomMsgInfo *)user_meta->user_meta_data;
    NvDsCustomMsgInfo *dstMeta = NULL;

    dstMeta =
        (NvDsCustomMsgInfo *)g_memdup2(srcMeta, sizeof(NvDsCustomMsgInfo));

    if (srcMeta->message)
        dstMeta->message = (gpointer)g_strdup((const char *)srcMeta->message);
    dstMeta->size = srcMeta->size;

    return dstMeta;
}

void NvOsdManager::meta_free_func_custom(gpointer data, gpointer user_data) {
    (void)user_data;
    NvDsUserMeta *user_meta = (NvDsUserMeta *)data;
    NvDsCustomMsgInfo *srcMeta = (NvDsCustomMsgInfo *)user_meta->user_meta_data;

    if (srcMeta->message) g_free(srcMeta->message);
    srcMeta->size = 0;

    g_free(user_meta->user_meta_data);
}

void NvOsdManager::event_message_custom_meta(
    NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta,
    NvDsObjectMeta *obj_meta, float *user_meta_data,
    std::vector<NvDsObjEncOutParams> encoded_images, guint source_id) {
    gchar *ts = (gchar *)g_malloc0(MAX_TIME_STAMP_LEN + 1);
    gchar *width, *height, *top, *left, *object_id, *confidence,
        *embedding_length, *json_embedding_vector, *src_id;
    gchar *message_data;
    NvDsObjEncOutParams *face_frame = &encoded_images.front();
    NvDsObjEncOutParams *full_frame = &encoded_images.back();
    if (encoded_images.size() == 3) {
        NvDsObjEncOutParams *body_frame = &encoded_images[1];
        (void)body_frame;
    }

    START_PROFILE;
    gchar *face_encoded_data =
        g_base64_encode(face_frame->outBuffer, face_frame->outLen);
    gchar *full_frame_encoded_data =
        g_base64_encode(full_frame->outBuffer, full_frame->outLen);
    // gchar *combined = g_strconcat(face_encoded_data, ";",
    // full_frame_encoded_data, NULL);

    // encoded_data = g_base64_encode(enc_jpeg_image->outBuffer,
    //                                 enc_jpeg_image->outLen);
    generate_ts_rfc3339(ts, MAX_TIME_STAMP_LEN);
    confidence = g_strdup_printf("%f", obj_meta->confidence);
    object_id = g_strdup_printf("%lu", obj_meta->object_id);
    src_id = g_strdup_printf("%d", source_id);
    top = g_strdup_printf("%f", obj_meta->rect_params.top);
    left = g_strdup_printf("%f", obj_meta->rect_params.left);
    width = g_strdup_printf("%f", obj_meta->rect_params.width);
    height = g_strdup_printf("%f", obj_meta->rect_params.height);
    embedding_length = g_strdup_printf("%d", EMBEDDING_VECTOR_SIZE);

    // Create a nlohmann::json object
    nlohmann::json embedding_vector_json;
    embedding_vector_json["embedding_vector"] = std::vector<float>(
        user_meta_data, user_meta_data + EMBEDDING_VECTOR_SIZE);
    std::string json_str_embedding_vector = embedding_vector_json.dump(4);
    json_embedding_vector = g_strdup(json_str_embedding_vector.c_str());

    /* Image message fields are separated by ";".
     * Specific Format:
     * "image;image_format;image_widthximage_height;time;encoded
     * data;" For Example:
     * "image;jpg;640x480;2023-07-31T10:20:13;xxxxxxxxxxx"
     */

    message_data =
        g_strconcat("image;jpg;",                  // fixed prefix
                    ";", ts,                       // timestamp
                    ";", face_encoded_data,        // face image
                    ";", full_frame_encoded_data,  // full frame image
                    ";", confidence, ";", src_id, ";", object_id, ";", top, ";",
                    left, ";", width, ";", height, ";", embedding_length, ";",
                    json_embedding_vector,  // embedding JSON
                    NULL);
    // message_data =
    //     g_strconcat("image;jpg;", width, "x", height, ";", ts,
    //                 ";", face_encoded_data, ";", NULL);
    STOP_PROFILE("Base64 Encode Time ");
    NvDsCustomMsgInfo *msg_custom_meta =
        (NvDsCustomMsgInfo *)g_malloc0(sizeof(NvDsCustomMsgInfo));
    msg_custom_meta->size = strlen(message_data);
    msg_custom_meta->message = g_strdup(message_data);
    NvDsUserMeta *user_event_meta_custom =
        nvds_acquire_user_meta_from_pool(batch_meta);
    if (user_event_meta_custom) {
        user_event_meta_custom->user_meta_data = (void *)msg_custom_meta;
        user_event_meta_custom->base_meta.meta_type = NVDS_CUSTOM_MSG_BLOB;
        user_event_meta_custom->base_meta.copy_func =
            (NvDsMetaCopyFunc)meta_copy_func_custom;
        user_event_meta_custom->base_meta.release_func =
            (NvDsMetaReleaseFunc)meta_free_func_custom;
        nvds_add_user_meta_to_frame(frame_meta, user_event_meta_custom);
        std::cout << "*** send custom message for source id = " << source_id
                  << " and object_id = " << obj_meta->object_id << " at " << ts
                  << " ***" << std::endl;
    } else {
        g_print(
            "Error in attaching event meta custom to "
            "buffer\n");
        // std::quick_exit(0);
    }

#ifdef ENABLE_DUMP_FILE
    gsize size = 0;
    snprintf(fileObjNameString, 1024, "%s_%d_%d_%s.jpg", ts, frame_number,
             frame_meta->batch_id, obj_meta->obj_label);
    guchar *decoded_data = g_base64_decode(face_encoded_data, &size);
    fp = fopen(fileObjNameString, "wb");
    if (fp) {
        fwrite(decoded_data, size, 1, fp);
        fclose(fp);
    } else {
        g_printerr("Could not open file!\n");
    }
    g_free(face_encoded_data);

    gsize size = 0;
    snprintf(fileObjNameString, 1024, "%s_%d_%d_%s.jpg", ts, frame_number,
             frame_meta->batch_id, obj_meta->obj_label);
    guchar *decoded_data = g_base64_decode(full_frame_encoded_data, &size);
    fp = fopen(fileObjNameString, "wb");
    if (fp) {
        fwrite(decoded_data, size, 1, fp);
        fclose(fp);
    } else {
        g_printerr("Could not open file!\n");
    }
    g_free(full_frame_encoded_data);
#endif

    g_free(ts);
    // g_free(message_data);  // after sending/processing
    g_free(width);
    g_free(height);
    g_free(top);
    g_free(left);
    g_free(object_id);
    g_free(src_id);
    g_free(confidence);
    g_free(embedding_length);
    g_free(json_embedding_vector);
    g_free(face_encoded_data);
    g_free(full_frame_encoded_data);
}

GstPadProbeReturn NvOsdManager::osd_src_pad_buffer_image_probe(
    GstPad *pad, GstPadProbeInfo *info, gpointer u_data) {
    (void)pad;
    (void)u_data;
    GstBuffer *buf = (GstBuffer *)info->data;
    NvDsFrameMeta *frame_meta = NULL;
    NvDsMetaList *l_frame, *l_obj;

    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);
    if (!batch_meta) {
        // No batch meta attached.
        return GST_PAD_PROBE_OK;
    }

    for (l_frame = batch_meta->frame_meta_list; l_frame;
         l_frame = l_frame->next) {
        frame_meta = (NvDsFrameMeta *)l_frame->data;

        if (frame_meta == NULL) {
            // Ignore Null frame meta.
            continue;
        }

        for (l_obj = frame_meta->obj_meta_list; l_obj; l_obj = l_obj->next) {
            NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)l_obj->data;

            if (obj_meta == NULL) {
                // Ignore Null object.
                continue;
            }

            //&& !(frame_number % frame_interval)
            /* Frequency of images to be send will be based on use case.
             * Here images is being sent for first object every
             * frame_interval(default=30).
             */
            std::vector<NvDsObjEncOutParams> encoded_images;
            NvDsObjEncOutParams *enc_jpeg_image = NULL;
            int num_encode = 0;
            bool is_meta_type_NVDS_CROP_IMAGE_META = false;
            NvDsUserMetaList *usrMetaList = obj_meta->obj_user_meta_list;
            while (usrMetaList != NULL) {
                NvDsUserMeta *usrMetaData = (NvDsUserMeta *)usrMetaList->data;
                if (usrMetaData->base_meta.meta_type == NVDS_CROP_IMAGE_META) {
                    enc_jpeg_image =
                        (NvDsObjEncOutParams *)usrMetaData->user_meta_data;
                    encoded_images.push_back(*enc_jpeg_image);
                    num_encode++;
                    is_meta_type_NVDS_CROP_IMAGE_META = true;
                    // usrMetaList = NULL;
                }
                // else {
                //     usrMetaList = usrMetaList->next;
                // }
                usrMetaList = usrMetaList->next;
            }

            // // Print results
            // for (const auto &item : encoded_images) {
            //     std::cout << " (size=" << item.outLen << ")\n";
            // }

            if (is_meta_type_NVDS_CROP_IMAGE_META == true) {
                enc_jpeg_image = get_full_frame(frame_meta);
                encoded_images.push_back(*enc_jpeg_image);
            }

            // Sort by size (ascending)
            std::sort(
                encoded_images.begin(), encoded_images.end(),
                [](const NvDsObjEncOutParams &a, const NvDsObjEncOutParams &b) {
                    return a.outLen < b.outLen;
                });

            NvDsUserMeta *user_meta = NULL;
            NvDsMetaList *l_user_meta = NULL;
            float *user_meta_data = NULL;
            bool is_meta_type_NVOSD_embedding_vector = false;
            for (l_user_meta = obj_meta->obj_user_meta_list;
                 l_user_meta != NULL; l_user_meta = l_user_meta->next) {
                user_meta = (NvDsUserMeta *)(l_user_meta->data);
                if (user_meta->base_meta.meta_type ==
                    NVDS_USER_EMBEDDING_VECTOR_META) {
                    is_meta_type_NVOSD_embedding_vector = true;
                    user_meta_data = (float *)user_meta->user_meta_data;
                }
            }
            if (is_meta_type_NVOSD_embedding_vector == true &&
                encoded_images.size() >= 2) {
                event_message_custom_meta(batch_meta, frame_meta, obj_meta,
                                          user_meta_data, encoded_images,
                                          frame_meta->source_id);
            }
        }
    }
    frame_number++;

    return GST_PAD_PROBE_OK;
}