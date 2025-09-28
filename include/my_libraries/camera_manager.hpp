#include <fstream>
#include <iostream>
#include <vector>

struct RtspCameraConfig {
    int camera_id;
    std::string address;
};

class CameraManager {
   private:
   public:
    std::vector<RtspCameraConfig> camera_list;
    CameraManager();
    void add_rtsp_camera(const std::string&, int);
    ~CameraManager();
};