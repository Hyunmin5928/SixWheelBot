#include "gps_module.h"
#include <chrono>
#include <thread>
#include <cmath>
#include <iostream>

static double haversine(double lat1, double lon1, double lat2, double lon2) {
    constexpr double R = 6371000.0;
    auto toRad = [](double deg){ return deg * M_PI / 180.0; };
    double dLat = toRad(lat2 - lat1);
    double dLon = toRad(lon2 - lon1);
    double a = std::sin(dLat/2)*std::sin(dLat/2)
             + std::cos(toRad(lat1))*std::cos(toRad(lat2))
             * std::sin(dLon/2)*std::sin(dLon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return R * c;
}

void navigation_thread(
    SafeQueue<std::vector<Waypoint>>& map_q,
    SafeQueue<int>&                  m_cmd_q
) {
    // 1) MAP 수신 
    std::vector<Waypoint> path;
    if (!map_q.ConsumeSync(path)) {
        std::cerr << "[nav] map data not received\n";
        return;
    }
    Logger::instance().info("gps", "[navigation_thread] GPS navigation thread start");
    size_t idx = 0;
    const double threshold = 1.0;  // m
    GPS gpsSensor;
    sGPS raw;

    // 2) 네비게이션 루프
    while (running) {
        // GPS 데이터 수신
        if (!gpsSensor.GetGPSdata(&raw)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        double lat = raw.latitude;
        double lon = raw.longitude;

        // 3) 목표 waypoint와 거리 계산
        if (idx < path.size()) {
            auto [wlat, wlon, dir] = path[idx];
            double dist = haversine(lat, lon, wlat, wlon);
            if (dist <= threshold) {
                // 도달: 방향 코드 전송
                m_cmd_q.Produce(std::move(dir));
                idx++;
            } else {
                // (선택) 전진 명령 보내기
                // m_cmd_q.Produce(FORWARD_CMD);
            }
        } else {
            // 경로 완료: PAUSE
            m_cmd_q.Produce(0);
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // 4) 종료 시 정지
    m_cmd_q.Produce(0);
    m_cmd_q.Finish();
}

void gps_reader_thread(
    // SafeQueue<GpsPos>& gps_q,
    SafeQueue<GpsPos>& gps_q
) {
    GPS gpsSensor;
    sGPS  raw;

    // 읽기 루프
    while (running) {
        // 하드웨어에서 유효한 GNGGA 메시지 하나 읽어들이면 true 반환
        Logger::instance().info("gps", "[gps_reader_thread] GPS reader thread start");
        if (gpsSensor.GetGPSdata(&raw)) {
            double lat = raw.latitude;
            double lon = raw.longitude;

            // 위치 큐에 푸시
            gps_q.Produce({lat, lon});

            // (선택) 로그 출력
            std::ostringstream oss;
            oss << "[gps_reader_thread] GPS: 위도= " << std::to_string(lat)
                << ", 경도=" << std::to_string(lon);
            Logger::instance().info("gps", oss.str());
        }
        // 너무 빡빡하게 읽지 않도록 잠깐 대기 (예: 200ms)
        std::this_thread::sleep_for(
            std::chrono::milliseconds(200)
        );
    }

    // 종료 신호 시 큐에도 끝 알리기
    gps_q.Finish();
}