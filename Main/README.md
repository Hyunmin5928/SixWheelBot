## 배달 로봇 통합 모듈

### 설명

* 전체 모듈 `SafeQueue.hpp`를 활용하여 멀티스레드화

### 빌드 방법

1. 현재 경로에서 `make`
2. `./delivery` 실행

## 모듈 별 설명

### main.cpp 및 SafeQueue.hpp

1. 각 모듈 통합을 위한 SafeQueue 방식의 멀티 스레드 제어 모듈
2. main.cpp을 통해 스레드 생성 및 관리 진행

### logger 라이브러리

1. 해당 라이브러리 파일을 활용해 손쉽게 로그 파일 생성 및 작성을 용이하게 함

### Arduino

1. 모터드라이버 & IMU(9축 자이로 센서) & 서보 모터 제어용
2. 모터 동작 방식 및 회피 로직 구현

### Communication

1. UDP 방식 통신 코드
2. 수신 및 송신 확인(ACK) 구현
3. map data 수신 후 queue에 저장하여 공유
4. GPS 센서를 활용하여 실시간 현재 위치 웹 서버에 전송

### GPS

1. GPS 센서를 활용하여 현재 위치 정보 추출
2. map data queue를 활용하여 도보 경로를 따라갈 수 있도록 방향을 설정
3. 배달 -> 복귀로직 구현

### LiDAR

1. LiDAR_Server에서 전송해주는 센서 데이터 수집 모듈
2. 라즈베리파이와 UDP 통신

### Vision

1. OpenCV + AI를 활용한 인도 검출 및 진행 방향 조절 모듈

### Motor

1. GPS, LiDAR, 카메라에서 보내주는 정보를 통합하여 모터 동작을 결정
2. Arduino와 Serial 통신 구축