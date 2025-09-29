#include <gst/gst.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "../include/my_libraries/camera_manager.hpp"
#include "../include/my_libraries/pipeline_manager.hpp"

namespace fs = std::filesystem;

gint *g_source_id_list = NULL;
gboolean *g_eos_list = NULL;
gboolean *check_finished_streams = NULL;
gboolean *g_source_enabled = NULL;
guint num_sources;  // number  of input cameras
GstElement **g_source_bin_list = NULL;

void allocate_memory_variables_cameras(const int MAX_NUM_SOURCES) {
    g_source_id_list = (gint *)g_malloc0(sizeof(gint) * MAX_NUM_SOURCES);
    g_eos_list = (gboolean *)g_malloc0(sizeof(gboolean) * MAX_NUM_SOURCES);
    check_finished_streams =
        (gboolean *)g_malloc0(sizeof(gboolean) * MAX_NUM_SOURCES);
    g_source_enabled =
        (gboolean *)g_malloc0(sizeof(gboolean) * MAX_NUM_SOURCES);
    g_source_bin_list =
        (GstElement **)g_malloc0(sizeof(GstElement *) * MAX_NUM_SOURCES);
}

int load_rtsp_address(CameraManager *camera_manager, fs::path file_path) {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " +
                                     file_path.string());
        }

        std::cout << "Contents of '" << file_path << "':\n";

        std::string line;
        int line_number = 1;
        while (std::getline(file, line)) {
            std::cout << line << '\n';
            camera_manager->add_rtsp_camera(line, line_number);
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 1) {
        std::cerr << "Usage: " << argv[0] << " <RTSP_URL>" << std::endl;
        return 1;
    }

    CameraManager *camera_manager = new CameraManager();
    // Path handling works across platforms
    fs::path data_dir = "../config";
    fs::path file_path = data_dir / "addresses.txt";
    load_rtsp_address(camera_manager, file_path);

    char **url_camera = new char *[camera_manager->camera_list.size() + 1];
    num_sources = camera_manager->camera_list.size();
    const int MAX_NUM_SOURCES = camera_manager->camera_list.size();

    allocate_memory_variables_cameras(MAX_NUM_SOURCES);
    url_camera[0] = argv[0];
    for (guint i = 0; i < num_sources; i++) {
        url_camera[i + 1] = &camera_manager->camera_list.at(i).address[0];
    }

    PipelineManager *pipeline_manager =
        new PipelineManager(num_sources, url_camera);
    if (!pipeline_manager->create_pipeline()) {
        g_printerr("pipeline could not be created. Exiting.");
        return 0;
    }
    pipeline_manager->create_pipeline_elements(num_sources, url_camera);

    return 0;
}