# Makefile

# 1. 컴파일러 설정
CXX      := g++
CC       := gcc
CXXFLAGS := -std=c++17 -Wall -pthread \
             -I. \
             -I./Communication \
             -I./GPS \
             -I./GPS/lib \
             -I./Gyro
LDFLAGS  := -pthread

# 2. 소스 파일 리스트
CPP_SRCS := main.cpp \
            Communication/comm_module.cpp \
            GPS/gps_module.cpp \
            GPS/lib/GPS_parse.cpp \
            Gyro/gyro_module.cpp \
            logger.cpp
# C_SRCS   := logger.c

# 3. 오브젝트 파일 리스트
OBJ_CPP  := $(CPP_SRCS:.cpp=.o)
OBJ_C    := $(C_SRCS:.c=.o)
OBJS     := $(OBJ_CPP) $(OBJ_C)

# 4. 최종 바이너리 이름
TARGET   := app   # 원하는 실행 파일 이름으로 변경 가능

# =================================
# 기본 타겟: all
all: $(TARGET)

# 링크 단계
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS)

# C++ 컴파일 규칙
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# C 컴파일 규칙
%.o: %.c
	$(CC) -Wall -c $< -o $@

# 청소 타겟
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
