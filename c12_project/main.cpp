#include <iostream>
#include <string_view>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include "Gimbal.hpp"
#include <opencv2/opencv.hpp>
#include "Camera.hpp"
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

struct Config {
    std::string ip;
    int port;
    int max_speed;
    std::string rtsp_url; 
};
    
Config load_config(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Config dosyasi acilamadi!");
    }

    json data = json::parse(file);
    return {
        data["gimbal_ip"],
        data["udp_port"],
        data["max_speed"],
        data["rtsp_url"]
    };
}   

int main() {
    Config cfg = load_config("config.json");
    bool is_thermal = false;    
    std::string stream_rgb = "rtsp://192.168.144.108:554/stream=1";
    std::string stream_thermal = "rtsp://192.168.144.108:555/stream=2";
    
    std::cout << "Gimbal baglantisi kuruluyor: " << cfg.ip << ":" << cfg.port << std::endl;
    Gimbal gimbal(cfg.ip, cfg.port);
    Camera camera(cfg.rtsp_url);     // RGB
    Camera camera2(stream_thermal);  // Termal

    std::cout << "Merkeze aliniyor...\n";
    gimbal.recenter();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "Saga donuluyor (Yaw 30)...\n";
    gimbal.yaw(30);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    std::cout << "Durduruluyor...\n";
    gimbal.stop();
    
    if (!camera.is_opened()) {
        std::cerr << "Kamera acilamadi, cikiliyor..." << std::endl;
        return -1;
    }
    
    cv::namedWindow("PIP Goruntu", cv::WINDOW_AUTOSIZE);
    cv::Mat frame, frame2;

    std::cout << "\n--- Kontroller ---" << std::endl;
    std::cout << "[W/S]: Pitch | [A/D]: Yaw | [BOSLUK]: Durdur | [R]: Merkezle | [Q/ESC]: Cikis\n" << std::endl;
    
    double zoom_level = 1.0; // 1.0 = normal, 2.0 = 2x, 4.0 = 4x
    
    while (true) {
        // 1. Her iki kameradan da kareleri oku
        bool rgb_ok = camera.get_frame(frame);
        bool thermal_ok = camera2.get_frame(frame2); // EKSİK OLAN SATIR EKLENDİ

        // Ana RGB kamera boşsa döngüyü atla
        if (!rgb_ok || frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // 2. Dijital Zoom İşlemi (Uygulanmamış kısım tamamlandı)
        if (zoom_level > 1.0) {
            int crop_w = static_cast<int>(frame.cols / zoom_level);
            int crop_h = static_cast<int>(frame.rows / zoom_level);
            int crop_x = (frame.cols - crop_w) / 2;
            int crop_y = (frame.rows - crop_h) / 2;
            
            // Çerçeveyi kes ve eski boyutuna geri büyüt
            cv::Rect zoom_roi(crop_x, crop_y, crop_w, crop_h);
            cv::Mat cropped = frame(zoom_roi).clone();
            cv::resize(cropped, frame, frame.size());
        }

        // 3. PIP (Resim İçinde Resim) İşlemi
        cv::Mat combined_frame = frame.clone();

        // GÜVENLİK KONTROLÜ: Termal kamera kopsa bile program çökmesin
        if (thermal_ok && !frame2.empty()) {
            cv::Mat pip_frame;
            int pip_w = combined_frame.cols / 4;
            int pip_h = combined_frame.rows / 4;
            cv::resize(frame2, pip_frame, cv::Size(pip_w, pip_h));

            int offset_x = combined_frame.cols - pip_w - 10;
            int offset_y = combined_frame.rows - pip_h - 10;
            cv::Rect roi(offset_x, offset_y, pip_w, pip_h);

            pip_frame.copyTo(combined_frame(roi));
            cv::rectangle(combined_frame, roi, cv::Scalar(255, 255, 255), 2);
        }
	// [C] Screenshot (Sarı renk)
cv::putText(combined_frame, "[C] Screenshot", cv::Point(30, combined_frame.rows - 80), 
            cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);

// [V] Video Start (Yeşil renk)
cv::putText(combined_frame, "[V] Video Start", cv::Point(30, combined_frame.rows - 50), 
            cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);

// [B] Video End (Kırmızı renk)
cv::putText(combined_frame, "[B] Video End", cv::Point(30, combined_frame.rows - 20), 
            cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
        // --- GÖRÜNTÜYÜ EKRANA BASMA ---
        cv::imshow("PIP Goruntu", combined_frame);

        // 1 ms bekle ve klavye girdisini oku
        int key = cv::waitKey(1);
        if (key == 27 || key == 'q' || key == 'Q') { // ESC veya Q
            break;
        }

        switch (key) {
            case 'a': case 'A': gimbal.yaw(-30); break;
            case 'd': case 'D': gimbal.yaw(30);  break;
            case 'w': case 'W': gimbal.pitch(25); break;
            case 's': case 'S': gimbal.pitch(-25); break;
            case 'r': case 'R': gimbal.recenter(); break;
            case ' ':           gimbal.stop();    break;
            // --- ZOOM KONTROLLERİ ---
            case 'z': case 'Z':
                if (zoom_level < 4.0) {
                    zoom_level += 0.25;
                    std::cout << "[Zoom] " << zoom_level << "x\n";
                }
                break;
            case 'x': case 'X':
                if (zoom_level > 1.0) {
                    zoom_level -= 0.25;
                    if (zoom_level < 1.0) zoom_level = 1.0;
                    std::cout << "[Zoom] " << zoom_level << "x\n";
                }
                break;
                
            case 'o': case 'O': gimbal.zoom_in(); break;
            case 'p': case 'P': gimbal.zoom_out(); break;
            case 'l': case 'L': gimbal.zoom_stop(); break;

            // --- FOCUS KONTROLLERİ ---
            case 'f': case 'F':
                std::cout << "[Lens] Otomatik netleme (AF) yapiliyor...\n";
                gimbal.auto_focus();
                break;
            case 'v': case 'V':
                std::cout << "[Lens] Focus Far (Uzak)...\n";
                gimbal.focus_far();
                break;
            case 'b': case 'B':
                std::cout << "[Lens] Focus Near (Yakin)...\n";
                gimbal.focus_near();
                break;
            case 'c': case 'C': {
                auto now = std::time(nullptr);
                auto tm = *std::localtime(&now);
                std::ostringstream oss;
                oss << "/home/thermaljetson1/Desktop/Proje_Hersey/c12_project/captures/frame_" 
    		<< std::put_time(&tm, "%Y%m%d_%H%M%S") << ".jpg";
                cv::imwrite(oss.str(), combined_frame);
                std::cout << "[Kayit] Fotograf kaydedildi: " << oss.str() << "\n";
                break;
            }
            
            default: break;
        }
    }

    gimbal.stop();
    cv::destroyAllWindows();
    return 0;
}
