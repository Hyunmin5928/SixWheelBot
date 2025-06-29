## Config 파일 구성 요소 정리

### SERVER 섹션 

1. 웹 서버 공인 IP 주소(tailscale 활용)
2. 통신 포트 번호 

### CLIENT 섹션

1. 배달 로봇 공인 IP 주소(tailscale 활용)
2. 통신 포트 번호

### NETWORK 섹션

1. 로컬 호스트 IP 주소
2. ALLOW IP 주소
3. 웹 서버 자체 통신 포트 번호
4. ACK 소켓 timeout 설정
5. ACK 소켓 재시도 횟수 설정
6. AI 추론 모델 서버 IP 주소
7. AI 서버 통신 포트 번호

### LOG 섹션

1. 로그 레벨 설정
2. 로그 파일 경로 작성

### DB 섹션

1. 웹 서버 DB 파일 경로

### MAP 섹션

1. 웹 서버 DB 파일 생성 위치 경로
