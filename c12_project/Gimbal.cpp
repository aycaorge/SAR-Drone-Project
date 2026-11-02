#include "Gimbal.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <algorithm>
#include <thread>
#include <chrono>

Gimbal::Gimbal(const std::string& ip, int port) : ip_(ip), port_(port), sock_fd_(-1) {
    sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd_ < 0) {
        std::cerr << "Soket olusturulamadi!\n";
        return;
    }

    // 2. Hedef adres yapilandirmasi
    std::memset(&dest_addr_, 0, sizeof(dest_addr_));
    dest_addr_.sin_family = AF_INET;
    dest_addr_.sin_port = htons(port_);
    inet_pton(AF_INET, ip_.c_str(), &dest_addr_.sin_addr);
}

Gimbal::~Gimbal() {
    if (sock_fd_ >= 0) {
        stop();
        close(sock_fd_);
    }
}

std::string Gimbal::calculate_checksum(const std::string& cmd) {
    int total = 0;
    for (char c : cmd) {
        total += static_cast<unsigned char>(c);
    }
    total &= 0xFF;

    std::stringstream ss;
    ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << total;
    return ss.str();
}

std::string Gimbal::speed_to_hex(int speed) {
    speed = std::clamp(speed, -99, 99);
    uint8_t byte_val = static_cast<uint8_t>(speed & 0xFF);

    std::stringstream ss;
    ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte_val);
    return ss.str();
}

void Gimbal::send_cmd(const std::string& cmd_body) {
    if (sock_fd_ < 0) return;

    std::string full_packet = cmd_body + calculate_checksum(cmd_body);
    
    // Giden paketi terminalde görelim:
    std::cout << "[Gimbal TX] " << full_packet << std::endl;

    sendto(sock_fd_, full_packet.c_str(), full_packet.length(), 0,
           reinterpret_cast<struct sockaddr*>(&dest_addr_), sizeof(dest_addr_));
}
void Gimbal::set_camera_mode(uint8_t mode) {
    std::vector<uint8_t> packet = {
        0x55, 0x66,  // Header
        0x01,        // Type
        0x01,        // Length
        0x0D,        // Cmd: Display Switch
        mode         // 0: RGB, 1: Thermal, 2: PIP
    };

    // Gimbal.cpp içindeki soket değişkeni adını kullanın (genelde sock_, sockfd_ veya socket_fd_)
    sendto(sock_fd_, 
           packet.data(), 
           packet.size(), 
           0, 
           (struct sockaddr*)&dest_addr_, 
           sizeof(dest_addr_));

    std::cout << "[Gimbal] Kamera modu degistirildi: " << static_cast<int>(mode) << std::endl;
}
void Gimbal::stop() {
    send_cmd("#TPUG2wGSP0000"); // Pitch durdur
    send_cmd("#TPUG2wGSY0000"); // Yaw durdur
    send_cmd("#TPUG2wPTZ00");   // Genel PTZ durdur
}

void Gimbal::recenter() {
    send_cmd("#TPUG2wPTZ05");
}

void Gimbal::yaw(int speed) {
    send_cmd("#TPUG2wGSY" + speed_to_hex(speed));
}

void Gimbal::pitch(int speed) {
    send_cmd("#TPUG2wGSP" + speed_to_hex(speed));
}
// --- ZOOM KOMUTLARI ---
// --- ZOOM KOMUTLARI (M: Camera Module) ---
// Alternatif Zoom Komutları (Hızlı/Yavaş parametreli)
// --- ZOOM KOMUTLARI (Hedef: M - Camera Module) ---
// --- ZOOM KOMUTLARI ---
// --- CAM TABANLI ZOOM KOMUTLARI ---
void Gimbal::zoom_in() {
    set_camera_mode(2);
}

void Gimbal::zoom_out() {
    send_cmd("#TPUG2wPTZ13");
}

void Gimbal::zoom_stop() {
    send_cmd("#TPUG2wPTZ13");
}

// --- CAM TABANLI FOCUS KOMUTLARI ---
void Gimbal::auto_focus() {
    send_cmd("#TPUG2wPTZ0F"); // Güvenli tek tık AF
}

void Gimbal::focus_far() {
    send_cmd("#TPUD2wREC01");
}

void Gimbal::focus_near() {
    send_cmd("#TPUD2wREC00");
}

void Gimbal::focus_stop() {
    send_cmd("#TPUG2wCAM0006");
}
// Gimbal.cpp içine test fonksiyonu:
void Gimbal::test_zoom_packets() {
    std::cout << "\n--- ZOOM PAKET TESTI BASLATISLIYOR ---\n";

    // 1. Varyasyon A: M modülü, 4w ve hız parametreli
    std::cout << "[Test 1] TPUM 4w ZOM (Hizli)...\n";
    send_cmd("#TPUM4wZOM00010005");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    send_cmd("#TPUM4wZOM00000000"); // Stop
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 2. Varyasyon B: G köprüsü üzerinden 4w
    std::cout << "[Test 2] TPUG 4w ZOM (Hizli)...\n";
    send_cmd("#TPUG4wZOM00010005");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    send_cmd("#TPUG4wZOM00000000"); // Stop
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 3. Varyasyon C: Kucuk harf tpUM 4w
    std::cout << "[Test 3] tpUM 4w ZOM...\n";
    send_cmd("#tpUM4wZOM00010005");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    send_cmd("#tpUM4wZOM00000000"); // Stop
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 4. Varyasyon D: MAG (Magnification) komutu
    std::cout << "[Test 4] TPUM 2w MAG...\n";
    send_cmd("#TPUM2wMAG0001");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    send_cmd("#TPUM2wMAG0000"); // Stop

    std::cout << "--- TEST TAMAMLANDI ---\n\n";
}
