# 파일 이름들
SRCS = main_doGps.cpp GPS_parse.cpp logger.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = gps_parse

# 컴파일러 및 옵션
CXX = g++
CXXFLAGS = -Wall -std=c++17

# 기본 빌드 명령
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

# 개별 .cpp -> .o 컴파일 규칙
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 정리
clean:
	rm -f $(OBJS) $(TARGET)
