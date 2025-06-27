# SixWheelBot
라즈베리파이5를 이용한 배달로봇 프로젝트

모듈 별 개별 디렉토리에 작업

## 📁 SixWheelBot 디렉토리 구조

SixWheelBot/
├── AI_Server/              # AI 모델 서버 통신 모듈
├── Config/                 # 설정 파일
├── Main/                   # 배달 로봇 통합 모듈
|   ├── Arduino/            # 아두이노 
│   ├── Communication/      # 통신 모듈
│   ├── GPS/                # GPS 모듈, Tmap 연동 모듈
│   │   └── lib/            # GPS 모듈관련 라이브러리 파일
│   ├── LiDAR/              # LiDAR 장애물 감지 및 회피 모듈
│   │   └── YDLidar-SDK/    # YDLiDAR SDK 라이브러리 파일
│   ├── Motor/              # Arduino 기반 Motor 제어 모듈
│   └── Vision/             # 이미지 추출 및 AI 서버 전송 모듈
└── Web/                    # 웹 서버 및 배달 로봇 통신 모듈
│   ├── web_project         # Node.js + react 기반 웹 서버
└───└── serer.py            # 웹 서버 <> 배달 로봇 통신 모듈
