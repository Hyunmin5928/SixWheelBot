#include <opencv2/opencv.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>
#include <csignal>
#include <iostream>

void ignore_sigpipe() {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, nullptr);
}

int main() {
    ignore_sigpipe();

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket 생성 실패");
        return -1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind 실패");
        return -1;
    }

    listen(server_fd, 1);
    std::cout << "MJPEG 스트리밍 서버 시작: http://192.168.0.208:8080/" << std::endl;

    // 해상도 명시적으로 설정 (예: 640x480)
    cv::VideoCapture cap(0, cv::CAP_V4L2);
    cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V'));
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    if (!cap.isOpened()) {
        std::cerr << "카메라 열기 실패" << std::endl;
        return -1;
    }

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            perror("클라이언트 연결 실패");
            continue;
        }

        std::string header =
            "HTTP/1.0 200 OK\r\n"
            "Server: MJPEGStreamer\r\n"
            "Cache-Control: no-cache\r\n"
            "Pragma: no-cache\r\n"
            "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
        send(client_fd, header.c_str(), header.size(), 0);

        while (true) {
            cv::Mat yuyv_frame;
            cap >> yuyv_frame;
            std::cout << "[DEBUG] frame.empty(): " << yuyv_frame.empty() << std::endl;
            if (yuyv_frame.empty()) {
                std::cerr << "[ERROR] 카메라 프레임 읽기 실패" << std::endl;
                break;
            }

            // 반드시 YUYV → BGR 변환!
            cv::Mat bgr_frame;
            cv::cvtColor(yuyv_frame, bgr_frame, cv::COLOR_YUV2BGR_YUYV);

            std::vector<uchar> buf;
            bool jpeg_ok = cv::imencode(".jpg", bgr_frame, buf);
            std::cout << "[DEBUG] jpeg_ok: " << jpeg_ok << ", buf.size(): " << buf.size() << std::endl;
            if (!jpeg_ok) {
                std::cerr << "[ERROR] JPEG 인코딩 실패" << std::endl;
                break;
            }

            std::ostringstream oss;
            oss << "--frame\r\n"
                << "Content-Type: image/jpeg\r\n"
                << "Content-Length: " << buf.size() << "\r\n\r\n";

            if (send(client_fd, oss.str().c_str(), oss.str().size(), 0) < 0) break;
            if (send(client_fd, reinterpret_cast<char*>(buf.data()), buf.size(), 0) < 0) break;
            if (send(client_fd, "\r\n", 2, 0) < 0) break;

            usleep(100000); // 10fps
        }

        close(client_fd);
        std::cout << "클라이언트 연결 종료됨" << std::endl;
    }

    close(server_fd);
    return 0;
}

// g++ web_streaming.cpp -o web_streaming `pkg-config --cflags --libs opencv4`
