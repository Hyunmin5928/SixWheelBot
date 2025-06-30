#include "gps_module.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(deg) ((deg) * M_PI / 180.0)
#define RAD2DEG(rad) ((rad) * 180.0 / M_PI)

static double haversine(double lat1, double lon1, double lat2, double lon2) {
    constexpr double R = 6371000.0;
    auto toRad = [](double deg){ return deg * M_PI / 180.0; };
    double dLat = toRad(lat2 - lat1);
    double dLon = toRad(lon2 - lon1);
    double a = std::sin(dLat/2)*std::sin(dLat/2)
             + std::cos(toRad(lat1))*std::cos(toRad(lat2))
             * std::sin(dLon/2)*std::sin(dLon/2);
    if (a < 0.0) a = 0.0;
    else if (a > 1.0) a = 1.0;
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    double result = R * c;
    Logger::instance().info("gps", "[nav] Calc Result : " + std::to_string(result));
    return R * c;
}

double bearing(double lat1, double lon1, double lat2, double lon2) {
    double phi1 = DEG2RAD(lat1);
    double phi2 = DEG2RAD(lat2);
    double delta_lambda = DEG2RAD(lon2 - lon1);

    double y = std::sin(delta_lambda) * std::cos(phi2);
    double x = std::cos(phi1) * std::sin(phi2) -
                std::sin(phi1) * std::cos(phi2) * std::cos(delta_lambda);

    double theta = std::atan2(y, x);
    return fmod((RAD2DEG(theta) + 360.0), 360.0);  // 0~360도 범위
}

void navigation_thread(
    SafeQueue<std::vector<Waypoint>>& map_q,
    SafeQueue<float>&                 dir_queue,
    SafeQueue<bool>&                  m_stop_queue,
    SafeQueue<int>&                   cmd_queue
) {
    pthread_setname_np(pthread_self(), "[THREAD]GPS_NAVD");
    // 1) MAP 수신 
    std::vector<Waypoint> path;
    if (!map_q.ConsumeSync(path)) {
        std::cerr << "[nav] map data not received\n";
        return;
    }
    // Logger::instance().info("gps", "[navigation_thread] GPS navigation thread start");
    const double threshold = 1.0;  // m
    GPS gpsSensor;
    sGPS raw;

    bool   outward         = true;   // true: delivery, false: return
    bool   wait_for_return = false;  // 배달 후 return 명령 대기
    size_t idx      = 0;       // path 인덱스
    // 프로그램 시작 시 한 번만 모터 구동 허용
    m_stop_queue.Produce(false);
    float dir_val = 0.0;
    // 2) 네비게이션 루프
    while (running.load()) {
        if (!run_gps.load() || !gpsSensor.GetGPSdata(&raw)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        Logger::instance().info("gps", "[navigation_thread] Running True");

        // GPS 데이터 수신
        double lat = raw.latitude;
        double lon = raw.longitude;

        if(outward) { // 목적지로 가는 경우 (1. 출발지 -> 사용자 ) 
            if (wait_for_return) {
                int cmd;
                if (cmd_queue.ConsumeSync(cmd) && cmd == 2) {
                    Logger::instance().info("gps", "[GPS] Return command received, start return");
                    outward = false;
                    wait_for_return = false;
                    idx = path.size() - 1;
                    m_stop_queue.Produce(false);
                }
            } else {
                // 다음 waypoint 방향 전송
                if (idx < path.size()) {
                    auto [wlat, wlon, dir] = path[idx];
                    int code = dir;
                    dir_val = 0.0f;
                    switch (code) {
                        case 12: case 212: dir_val = -90.0f; break;
                        case 16: case 214: dir_val = -120.0f; break;
                        case 17: case 215: dir_val = -60.0f; break;
                        case 13: case 213: dir_val = 90.0f; break;
                        case 18: case 216: dir_val = 60.0f; break;
                        case 19: case 217: dir_val = 120.0f; break;
                        case 211: /* 횡단보도, 유지 */ break;
                        case 201: dir_val = 0.0f; break;
                        default: Logger::instance().warn("gps", "[GPS] Unknown dir code " + std::to_string(code)); break;
                    }
                    dir_queue.Produce(std::move(dir_val));
                    Logger::instance().info("gps", "[GPS] DIR command sent: " + std::to_string(dir_val));
                    m_stop_queue.Produce(false);
                }
                // waypoint 도착 체크
                if (idx < path.size()) {
                    auto [wlat, wlon, dir] = path[idx];
                    double dist = haversine(lat, lon, wlat, wlon);
                    if (dist <= threshold) {
                        idx++;
                        if (idx >= path.size()) {
                            // 최종 목적지 도착
                            m_stop_queue.Produce(true);
                            wait_for_return = true;
                            Logger::instance().info("gps", "[GPS] Delivery arrived, waiting for return");
                        }
                    }
                }
            }
        }
        // 3-3) 복귀 모드
        else {
            // 다음 waypoint 방향 전송 (역순)
            if (idx < path.size()) {
                auto [wlat, wlon, dir] = path[idx];
                int code = dir;
                dir_val = 0.0f;
                switch (code) {
                    case 12: case 212: dir_val = 90.0f; break;
                    case 16: case 214: dir_val = -120.0f; break;
                    case 17: case 215: dir_val = 60.0f; break;
                    case 13: case 213: dir_val = -90.0f; break;
                    case 18: case 216: dir_val = -60.0f; break;
                    case 19: case 217: dir_val = 120.0f; break;
                    case 211: /* 횡단보도 */ break;
                    case 201: dir_val = 0.0f; break;
                    default: Logger::instance().warn("gps", "[GPS] Unknown return dir code " + std::to_string(code)); break;
                }
                dir_queue.Produce(std::move(dir_val));
                Logger::instance().info("gps", "[GPS] Return DIR sent: " + std::to_string(dir_val));
                m_stop_queue.Produce(false);
            }
            // 복귀 waypoint 도착 체크
            if (idx < path.size()) {
                auto [wlat, wlon, dir] = path[idx];
                double dist = haversine(lat, lon, wlat, wlon);
                if (dist <= threshold) {
                    if (idx == 0) {
                        // 출발지 복귀 완료
                        m_stop_queue.Produce(true);
                        Logger::instance().info("gps", "[GPS] Return arrived, navigation complete");
                        break;
                    } else {
                        idx--;
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    dir_queue.Finish();
}

void gps_reader_thread(
    SafeQueue<GpsPos>& gps_q,
    SafeQueue<bool>& m_stop_queue
) {
    pthread_setname_np(pthread_self(), "[THREAD]GPS_READ");
    GPS gpsSensor;
    sGPS  raw;
    // 읽기 루프
    while (running.load()) {
        // Logger::instance().info("gps", "[gps_reader_thread] Running True");
        if (!run_gps.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        // 하드웨어에서 유효한 GNGGA 메시지 하나 읽어들이면 true 반환
        // Logger::instance().info("gps", "[gps_reader_thread] GPS reader thread start");
        if (gpsSensor.GetGPSdata(&raw)) {
            double lat = raw.latitude;
            double lon = raw.longitude;

            // 위치 큐에 푸시
            gps_q.Produce({lat, lon});

            // (선택) 로그 출력
            std::string msg =
                "[gps_reader_thread] GPS: 위도=" + std::to_string(lat) +
                ", 경도=" + std::to_string(lon);
            Logger::instance().info("gps", msg);
        }
        else {
            Logger::instance().error("gps", "[gps_reader_thread] Can't read GPS data");
            m_stop_queue.Produce(false);
        }

        // 너무 빡빡하게 읽지 않도록 잠깐 대기 (예: 200ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // 종료 신호 시 큐에도 끝 알리기
    gps_q.Finish();
}