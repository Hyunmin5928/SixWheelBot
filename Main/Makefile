# Makefile

CXX        := g++
CXXFLAGS   := -std=c++17 -Wall -pthread -I. $(shell pkg-config --cflags opencv4)
LDFLAGS    := $(shell pkg-config --libs opencv4) -lwiringPi

# 소스 디렉터리 목록
SRCDIRS    := . Communication GPS LiDAR Motor Vision

# 각 디렉터리의 .cpp 파일을 모두 수집
SOURCES    := $(wildcard *.cpp) \
              $(wildcard Communication/*.cpp) \
              $(wildcard GPS/*.cpp) 
            #   $(wildcard LiDAR/*.cpp) \
              $(wildcard Motor/*.cpp) \
              $(wildcard Vision/*.cpp)

# .cpp → .o
OBJECTS    := $(SOURCES:.cpp=.o)

# 최종 실행 파일 이름
TARGET     := main

.PHONY: all clean

all: $(TARGET)

# 링크
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# 컴파일 규칙: foo.cpp → foo.o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
