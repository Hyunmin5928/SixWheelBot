# Makefile

CXX = g++
CXXFLAGS = -Wall -std=c++17
LDFLAGS = -lwiringPi -lpthread
TARGET = motorCtrlTest
SRC = motorCtrl.cpp
HDR = motorCtrl.h

all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
