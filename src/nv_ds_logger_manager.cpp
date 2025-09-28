#include "../include/my_libraries/nv_ds_logger_manager.hpp"

NvDsLoggerManager::NvDsLoggerManager() {}

bool NvDsLoggerManager::create_nv_ds_logger() {
    /* Use nvdslogger for perf measurement. */
    nvdslogger = gst_element_factory_make("nvdslogger", "nvdslogger");
    if (!nvdslogger) {
        g_printerr("Unable to create nvdslogger.Exiting.");
        return false;
    }
    return true;
}