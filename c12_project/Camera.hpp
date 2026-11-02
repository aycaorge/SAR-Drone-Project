
#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <opencv2/opencv.hpp>

class Camera {
public:
    explicit Camera(const std::string& rtsp_url);
    ~Camera();

    bool is_opened() const;
    bool get_frame(cv::Mat& frame);
    bool switch_stream(const std::string& new_rtsp_url);


private:
    cv::VideoCapture cap_;
    std::string build_pipeline(const std::string& rtsp_url) const;

    // Asenkron çalışma değişkenleri
    std::thread capture_thread_;
    std::mutex frame_mutex_;
    cv::Mat latest_frame_;
    std::atomic<bool> is_running_{false};
    std::atomic<bool> has_new_frame_{false};

    void capture_loop();
};
