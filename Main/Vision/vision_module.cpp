#include "vision_module.h"


// ---- 외부 전역 & 상수 ------------------------------------------------
// extern std::atomic<bool> running;       // 메인에서 true/false 로 전체 종료 제어
// extern int               SERVER_PORT;   // 기본 9999, 필요 시 변경
// static constexpr const char kServerIp[] = "192.168.0.39"; // ★ 고정 IP

// ---------------------------------------------------------------------
// sendall : len 바이트를 보낼 때까지 반복 send ------------------------
static void sendall(int sock, const void* buf, size_t len)
{
    const char* p = static_cast<const char*>(buf);   // 현재 전송 포인터
    while (len) {
        ssize_t n = ::send(sock, p, len, 0);         // 남은 바이트 전송
        if (n <= 0) throw std::runtime_error("sendall error"); // 에러 시 예외
        p   += n;                                    // 포인터 이동
        len -= n;                                    // 남은 길이 감소
    }
}

// recvall : len 바이트를 다 받을 때까지 반복 recv ---------------------
static void recvall(int sock, void* buf, size_t len)
{
    char* p = static_cast<char*>(buf);               // 수신 버퍼 포인터
    while (len) {
        ssize_t n = ::recv(sock, p, len, 0);         // 최대 len 바이트 수신
        if (n <= 0) throw std::runtime_error("recv error"); // 연결 종료/에러
        p   += n;                                    // 포인터 이동
        len -= n;                                    // 남은 길이 감소
    }
}

// ---------------------------------------------------------------------
// 비전 스레드 : dir_queue 로 angle 값을 푸시 ---------------------------
void vision_thread(SafeQueue<float>& dir_queue)
{
    cv::VideoCapture cap(0);                         // 0번 카메라 열기
    if (!cap.isOpened()) {                           // 실패 시 즉시 종료
        std::cerr << "[vision] camera open failed\n";
        return;
    }

    const std::vector<int> enc_params = {cv::IMWRITE_JPEG_QUALITY, 90}; // JPEG 품질 90%
    const std::chrono::milliseconds retry_delay(200); // 오류 발생 시 200 ms 후 재시도
    const std::chrono::milliseconds loop_delay(50);   // 루프 최소 간격 50 ms (≈20 FPS)

    while (running.load()) {                          // 전체 시스템이 실행 중인 동안 반복
        auto loop_start = std::chrono::steady_clock::now(); // 루프 시작 시각

        // 1) 프레임 캡처 ------------------------------------------------
        cv::Mat frame; cap >> frame;                 // 한 프레임 가져오기
        if (frame.empty()) {                         // 캡처 실패 시 경고 후 재시도
            std::cerr << "[vision] empty frame captured\n";
            std::this_thread::sleep_for(retry_delay);
            continue;
        }

        // 2) JPEG 인코딩 ----------------------------------------------
        std::vector<uchar> jpeg_buf;                 // 결과 버퍼
        if (!cv::imencode(".jpg", frame, jpeg_buf, enc_params)) {
            std::cerr << "[vision] JPEG encode failed\n";
            continue;                                // 실패 시 다음 루프
        }

        // 3) TCP 소켓 생성 & 서버 연결 --------------------------------
        int sock = ::socket(AF_INET, SOCK_STREAM, 0); // IPv4/TCP
        if (sock < 0) {
            std::cerr << "[vision] socket() failed: " << strerror(errno) << '\n';
            std::this_thread::sleep_for(retry_delay);
            continue;
        }

        struct timeval tv{5, 0};                     // 5초 send/recv 타임아웃
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        sockaddr_in srv{};                           // 서버 주소 구조체
        srv.sin_family = AF_INET;
        srv.sin_port   = htons(static_cast<uint16_t>(AI_SERVER_PORT)); // 포트 네트워크 바이트오더
        if (::inet_pton(AF_INET, AI_SERVER, &srv.sin_addr) != 1) {  // 고정 IP 사용
            std::cerr << "[vision] invalid server IP constant\n";
            ::close(sock);
            std::this_thread::sleep_for(retry_delay);
            continue;
        }

        if (::connect(sock, reinterpret_cast<sockaddr*>(&srv), sizeof(srv)) < 0) {
            std::cerr << "[vision] connect() failed: " << strerror(errno) << '\n';
            ::close(sock);
            std::this_thread::sleep_for(retry_delay);
            continue;
        }

        try {
            // 4) [4바이트 길이][JPEG] 전송 -----------------------------
            uint32_t img_len_net = htonl(static_cast<uint32_t>(jpeg_buf.size())); // 길이 네트워크 오더
            sendall(sock, &img_len_net, 4);            // 길이 먼저
            sendall(sock, jpeg_buf.data(), jpeg_buf.size()); // JPEG 본문 전송

            // 5) [4바이트 길이][angle] 수신 ---------------------------
            uint32_t resp_len_net; recvall(sock, &resp_len_net, 4);   // 길이 수신
            uint32_t resp_len = ntohl(resp_len_net);                  // 호스트 바이트 오더 변환
            if (resp_len == 0 || resp_len > 64)                       // 길이 sanity 체크
                throw std::runtime_error("invalid payload length");

            std::string payload(resp_len, '\0');
            recvall(sock, payload.data(), resp_len);                  // angle 문자열 수신

            float angle = std::stof(payload);        // 문자열 → float 변환
                // ★ 다른 모듈로 전달
            dir_queue.Produce(std::move(angle));
        }
        catch (const std::exception& e) {            // send/recv/parse 실패 시 처리
            std::cerr << "[vision] transfer error: " << e.what() << '\n';
        }

        ::close(sock);                               // 소켓 닫기

        // 6) 루프 속도 조절 ------------------------------------------
        auto elapsed = std::chrono::steady_clock::now() - loop_start;
        if (elapsed < loop_delay)
            std::this_thread::sleep_for(loop_delay - elapsed);        // 20 FPS 유지
    }
}
