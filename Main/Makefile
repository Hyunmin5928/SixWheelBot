# 1. 컴파일러 설정
CXX      := g++
CC       := gcc

PKG_OCV_CFLAGS := $(shell pkg-config --cflags opencv4)
PKG_OCV_LIBS   := $(shell pkg-config --libs   opencv4)
SDK_ROOT      := LiDAR/YDLidar-SDK
SDK_INC_SRC   := $(SDK_ROOT)/src             # CYdLidar.h
SDK_INC_COMMON:= $(SDK_ROOT)/core/common     # ydlidar_help.h 등
SDK_LIB       := $(SDK_ROOT)/build/libydlidar_sdk.a


CXXFLAGS := -std=c++17 -Wall -pthread \
            -I$(SDK_ROOT)       \
            -I$(SDK_INC_SRC)    \
            -I$(SDK_INC_COMMON) \
            -I.                  \
            -I./Communication    \
            -I./GPS              \
            -I./GPS/lib          \
            -I./IMU             \
            -I./IMU/lib         \
            -I./LiDAR           \
            -I./Motor           \
            $(PKG_OCV_CFLAGS)   

LDFLAGS  := $(SDK_LIB) $(PKG_OCV_LIBS) -pthread -lwiringPi
#LIBS := $(SDK_LIB) $(PKG_OCV_LIBS) -lpthread -lwiringPi

# 2. 소스 파일 리스트
CPP_SRCS := main.cpp \
            Communication/comm_module.cpp \
            GPS/gps_module.cpp \
            GPS/lib/GPS_parse.cpp \
            LiDAR/Lidar.cpp \
            LiDAR/lidar_module.cpp \
            IMU/imu_module.cpp \
            IMU/lib/SerialPort.cpp \
            Motor/motor_module.cpp  \
            Motor/motorCtrl.cpp     \
            logger.cpp \
            Vision/vision_module.cpp
# C_SRCS   := logger.c

# 3. 오브젝트 파일 리스트
OBJ_CPP  := $(CPP_SRCS:.cpp=.o)
# OBJ_C    := $(C_SRCS:.c=.o)
OBJS     := $(OBJ_CPP) $(OBJ_C)

# 4. 최종 바이너리 이름
TARGET   := app   # 원하는 실행 파일 이름으로 변경 가능

# =================================
# 기본 타겟: all
.PHONY: all clean
all: $(TARGET)

# 5. 링크 명령: OBJS 뒤에 SDK_LIB 를 둡니다.
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(SDK_LIB)

# 6. 컴파일 규칙
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)