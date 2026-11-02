#pragma once

#include <string>
#include <arpa/inet.h>


class Gimbal{  

	public:
		Gimbal(const std::string& ip, int port);
		~Gimbal();
		void stop();
    		void recenter();
    		void yaw(int speed);
    		void pitch(int speed);
    		void set_camera_mode(uint8_t mode);
		void zoom_in();
    void zoom_out();
    void zoom_stop();

    void auto_focus();
    void focus_far();
    void focus_near();
    void focus_stop();
    void test_zoom_packets();
    	private:
	    std::string ip_;
	    int port_;
	    int sock_fd_;
	    struct sockaddr_in dest_addr_;

	    void send_cmd(const std::string& cmd_body);
	    std::string calculate_checksum(const std::string& cmd);
	    std::string speed_to_hex(int speed);
	    };
	    		
		
