# Makefile

# 컴파일러 설정
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -pthread -I.

# 빌드할 실행 파일 이름
TARGET   := main_gps

# 소스 파일 리스트
SRCS     := runGPS.cpp \
            GPS_parse.cpp

# 오브젝트 파일 리스트 (SRCS에서 .cpp→.o 변환)
OBJS     := $(SRCS:.cpp=.o)

# 기본 타겟: all
all: $(TARGET)

# 링크 단계
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

# C++ 컴파일 규칙
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 청소 타겟
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
