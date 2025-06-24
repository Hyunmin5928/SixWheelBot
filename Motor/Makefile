# Makefile

CXX = g++
CXXFLAGS = -Wall -std=c++17 -pthread \
             -I. \
			-I../Main/LiDAR \
			-I../Main/LiDAR/YDLidar-SDK \
			-I../Main/LiDAR/YDLidar-SDK/src \
			-I../Main/LiDAR/YDLidar-SDK/core/common \
			-I../Main/LiDAR/YDLidar-SDK/core/base \
			-I../Main/LiDAR/YDLidar-SDK/core
LDFLAGS = -lwiringPi -lpthread
TARGET = motorCtrlTest
SRC = motorCtrl.cpp
HDR = motorCtrl.h

all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
