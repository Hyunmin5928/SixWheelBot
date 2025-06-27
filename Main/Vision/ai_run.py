#1. ai 모델 실행 로직
#1-1. 모델 로드 -> 스레드 종료 시 모델 종료
#31-2. input.img를 ai 모델을 통해 추론하는 로직
#2. vision_module.cpp과 통신 하는 로직
#2-1. 스레드 생성 시 넘어오는 명령어(start) -> 모델 로드
#2-2. 스레드가 생성한 img 파일 경로를 참고하여 ai 모델에 넣어주는 로직
#2-3. output.img 생성하고 vision_module.cpp에 전달하는 로직


#ip랑 port는 localhost = 127.0.0.1
#msg = "start", "input.img", "output.img", "stop"

