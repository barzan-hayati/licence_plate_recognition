#include "../include/my_libraries/camera_manager.hpp"

CameraManager::CameraManager() { ; }

void CameraManager::add_rtsp_camera(const std::string& rtsp_address,
                                    int camera_Id) {
    RtspCameraConfig current_camera;
    current_camera.address = rtsp_address;
    current_camera.camera_id = camera_Id;
    camera_list.push_back(current_camera);
}