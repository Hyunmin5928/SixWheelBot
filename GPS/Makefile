# — 파일 및 디렉토리 설정
SRCDIR    := src
INCDIR    := include
SRCS      := $(wildcard $(SRCDIR)/*.cpp)
OBJS      := $(patsubst $(SRCDIR)/%.cpp,$(SRCDIR)/%.o,$(SRCS))
TARGET    := gps_parse

# — 컴파일러 및 옵션
CXX       := g++
CXXFLAGS  := -Wall -std=c++17 -I$(INCDIR)

# — 기본 빌드
all: $(TARGET)

# — 링킹
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# — src/ 디렉토리 안 .cpp → .o
$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# — 정리
clean:
	rm -f $(OBJS) $(TARGET)


# # 파일 이름들
# SRCS = src/main_doGps.cpp src/GPS_parse.cpp src/logger.cpp
# OBJS = $(SRCS:.cpp=.o) \
# 		lib/* \
# TARGET = gps_parse

# # 컴파일러 및 옵션
# CXX = g++
# CXXFLAGS = -Wall -std=c++17

# # 기본 빌드 명령
# all: $(TARGET)

# $(TARGET): $(OBJS)
# 	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

# # 개별 .cpp -> .o 컴파일 규칙
# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# # 정리
# clean:
# 	rm -f $(OBJS) $(TARGET)
