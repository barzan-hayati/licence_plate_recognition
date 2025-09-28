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
    //    struct Item {
    //     std::string name;
    //     int size;
    //     };
    GstElement *nvosd = NULL;
    static bool write_full_frame_to_disk, write_cropped_objects_to_disk;
    NvOsdManager();
    bool create_nv_osd();
    ~NvOsdManager();
    static gint frame_number;
    void attach_probe_to_sink_nvosd(NvDsObjEncCtxHandle);
    static GstPadProbeReturn osd_sink_pad_buffer_probe(GstPad *,
                                                       GstPadProbeInfo *,
                                                       gpointer);
    void attach_probe_to_src_nvosd(NvDsObjEncCtxHandle);
    static GstPadProbeReturn osd_src_pad_buffer_probe(GstPad *,
                                                      GstPadProbeInfo *,
                                                      gpointer);
    static void save_full_frame(NvDsFrameMeta *);
    static void save_cropped_objects(NvDsFrameMeta *, NvDsObjectMeta *, guint);
    static GstPadProbeReturn osd_src_pad_buffer_metadata_probe(
        GstPad *, GstPadProbeInfo *, gpointer);
    static GstPadProbeReturn osd_src_pad_buffer_image_probe(GstPad *,
                                                            GstPadProbeInfo *,
                                                            gpointer);
    static void generate_event_msg_meta(gpointer, gint, NvDsObjectMeta *);
    static gpointer meta_copy_func(gpointer, gpointer);
    static void meta_free_func(gpointer, gpointer);
    static void generate_ts_rfc3339(char *, int);
    static gpointer meta_copy_func_custom(gpointer, gpointer);
    static void meta_free_func_custom(gpointer, gpointer);
    static void generate_face_meta(gpointer);
    static void generate_person_meta(gpointer);
    static void event_message_meta(NvDsBatchMeta *, NvDsFrameMeta *,
                                   NvDsObjectMeta *, float *,
                                   std::vector<NvDsObjEncOutParams>);
    static void event_message_custom_meta(NvDsBatchMeta *, NvDsFrameMeta *,
                                          NvDsObjectMeta *, float *,
                                          std::vector<NvDsObjEncOutParams>,
                                          guint);
    static NvDsObjEncOutParams *get_full_frame(NvDsFrameMeta *);
    static NvDsObjEncOutParams *get_cropped_objects(NvDsObjectMeta *);
};