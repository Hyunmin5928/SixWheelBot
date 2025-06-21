CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2

INCLUDES = \
  -ISixWheelBot/Main/LiDAR/YDLidar/YDLidar-SDK/src \
  -ISixWheelBot/Main/Motor \
  -ISixWheelBot/Main/LiDAR \

SRC = \
  Motor/motorCtrl.cpp \
  LiDAR/Lidar.cpp

OBJ = $(SRC:.cpp=.o)

TARGET = SixWheelBotApp

LDFLAGS = -lwiringPi -LYDLiDAR/YDLidar-SDK/build -lydlidar_sdk

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)


# CXX = g++
# CXXFLAGS = -std=c++17 -Wall -O2

# INCLUDES = \
#   -ISixWheelBot/Motor \
#   -ISixWheelBot/Lidar \
#   # -IYDLiDAR/YDLidar-SDK/src \
#   # -IYDLiDAR/YDLidar-SDK/src/core \
#   # -IYDLiDAR/YDLidar-SDK/src/core/common

# SRC = Motor/motorCtrl.cpp \
#       # YDLiDAR/YDLidar-SDK/examples/myung-ryun-lidar.cpp

# OBJ = $(SRC:.cpp=.o)

# TARGET = motorCtrl_

# LDFLAGS = -lwiringPi -LYDLiDAR/YDLidar-SDK/build -lydlidar_sdk

# all: $(TARGET)

# $(TARGET): $(OBJ)
# 	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# clean:
# 	rm -f $(OBJ) $(TARGET)
