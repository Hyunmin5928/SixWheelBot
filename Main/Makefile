# 컴파일러 및 기본 설정
CXX := g++
CXXFLAGS := -std=c++17 -Wall -IMotor -ILiDAR -ILiDAR/lib -pthread 

# 소스 파일 목록
SRCS := \
    Motor/motor_module.cpp \
    Motor/motorCtrl.cpp \
    LiDAR/lidar_module.cpp \
    LiDAR/Lidar.cpp \
    logger.cpp \
    SafeQueue.hpp

LDFLAGS = -L/usr/lib/ -lwiringPi -LYDLiDAR/YDLidar-SDK/build -lydlidar_sdk
# 오브젝트 파일 목록
OBJS := $(SRCS:.cpp=.o)

# 최종 실행 파일 이름
TARGET := main_test

# 빌드 타겟
all: $(TARGET)

# 링크 단계
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# 개별 cpp -> o 파일로 빌드
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 정리 명령
clean:
	rm -f $(OBJS) $(TARGET)
# # Makefile

# # 1. 컴파일러 설정
# CXX      := g++
# CC       := gcc
# CXXFLAGS := -std=c++17 -Wall -pthread \
#              -I. \
#              -I./Communication \
#              -I./GPS \
#              -I./GPS/lib \
#              -I./IMU
# LDFLAGS  := -pthread

# # 2. 소스 파일 리스트
# CPP_SRCS := main.cpp \
#             Communication/comm_module.cpp \
#             GPS/gps_module.cpp \
#             GPS/lib/GPS_parse.cpp \
#             IMU/imu_module.cpp \
#             logger.cpp
# # C_SRCS   := logger.c

# # 3. 오브젝트 파일 리스트
# OBJ_CPP  := $(CPP_SRCS:.cpp=.o)
# OBJ_C    := $(C_SRCS:.c=.o)
# OBJS     := $(OBJ_CPP) $(OBJ_C)

# # 4. 최종 바이너리 이름
# TARGET   := app   # 원하는 실행 파일 이름으로 변경 가능

# # =================================
# # 기본 타겟: all
# all: $(TARGET)

# # 링크 단계
# $(TARGET): $(OBJS)
# 	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS)

# # C++ 컴파일 규칙
# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# # C 컴파일 규칙
# %.o: %.c
# 	$(CC) -Wall -c $< -o $@

# # 청소 타겟
# clean:
# 	rm -f $(OBJS) $(TARGET)

# .PHONY: all clean
