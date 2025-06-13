CXX = g++
CXXFLAGS = -std=c++17 -O2 -I./include
LDFLAGS = -pthread

TARGET = udp_client
SRCS = udp_client.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)
