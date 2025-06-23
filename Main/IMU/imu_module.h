// imu_module.h
#pragma once

#include <thread>
#include <atomic>
#include <string>
#include "SafeQueue.hpp"
#include "SerialPort.h"   // 기존 GPS/lib 에 있던 시리얼 포트 래퍼

/// IMU 에서 읽어들인 Roll/Pitch/Yaw 값을 담는 구조체
struct ImuData {
    double roll;
    double pitch;
    double yaw;
};

class ImuModule {
public:
    /// 생성자: 시리얼 포트 경로, 보드레이트, 데이터를 푸시할 큐를 받음
    ImuModule(const std::string& port, unsigned int baud, SafeQueue<ImuData>& queue);

    ~ImuModule();

    /// 스레드를 띄워 start/stop 명령 대기 → IMU 데이터 읽기 시작
    void start();

    /// stop 명령 전송 → IMU 데이터 읽기 중지
    void stop();

private:
    /// 내부 워커 스레드 함수
    void run();

    SerialPort     serial_;       // 시리얼 통신 객체
    SafeQueue<ImuData>& queue_;   // 읽은 데이터를 넣을 큐
    std::thread    worker_;       // 백그라운드 스레드
    std::atomic<bool> running_{false};    // start/stop 플래그
    std::atomic<bool> threadAlive_{true}; // 스레드 전체 생명주기 플래그
};
