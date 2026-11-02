#include "Camera.hpp"
#include <iostream>

Camera::Camera(const std::string& rtsp_url) {
    std::string pipeline = build_pipeline(rtsp_url);
    std::cout << "[Camera] GStreamer baslatiliyor:\n" << pipeline << std::endl;

    cap_.open(pipeline, cv::CAP_GSTREAMER);
    if (!cap_.isOpened()) {
        std::cerr << "[Camera] HATA: RTSP akisi acilamadi!\n";
        return;
    }

    // Arka plan iş parçacığını başlat
    is_running_ = true;
    capture_thread_ = std::thread(&Camera::capture_loop, this);
}

Camera::~Camera() {
    is_running_ = false;
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
    if (cap_.isOpened()) {
        cap_.release();
    }
}

bool Camera::is_opened() const {
    return cap_.isOpened();
}

void Camera::capture_loop() {
    cv::Mat temp_frame;
    while (is_running_) {
        if (!cap_.read(temp_frame) || temp_frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        // En son kareyi güvenli şekilde güncelle
        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            temp_frame.copyTo(latest_frame_);
            has_new_frame_ = true;
        }
    }
}

bool Camera::get_frame(cv::Mat& frame) {
    if (!has_new_frame_) {
        return false;
    }

    std::lock_guard<std::mutex> lock(frame_mutex_);
    if (latest_frame_.empty()) {
        return false;
    }

    latest_frame_.copyTo(frame);
    return true;
}

std::string Camera::build_pipeline(const std::string& rtsp_url) const {
    return "rtspsrc location=" + rtsp_url + " protocols=tcp latency=100 ! "
           "rtph265depay ! h265parse ! nvv4l2decoder ! "
           "nvvidconv ! video/x-raw, format=BGRx ! "
           "videoconvert ! video/x-raw, format=BGR ! appsink drop=1 sync=false";
}
bool Camera::switch_stream(const std::string& new_rtsp_url) {
    if (cap_.isOpened()) {
        cap_.release();
    }
    
    std::string pipeline = build_pipeline(new_rtsp_url);
    std::cout << "[Camera] Yeni akis baslatiliyor: " << new_rtsp_url << std::endl;
    
    cap_.open(pipeline, cv::CAP_GSTREAMER);
    if (!cap_.isOpened()) {
        std::cerr << "[Camera] HATA: " << new_rtsp_url << " acilamadi!\n";
        return false;
    }
    return true;
}
