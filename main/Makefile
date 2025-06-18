# Makefile

CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -Iinclude $(shell pkg-config --cflags opencv4)
LDFLAGS   := $(shell pkg-config --libs opencv4)

SRCDIR    := src
SOURCES   := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS   := $(SOURCES:.cpp=.o)
TARGET    := robot_threads

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# src/foo.cpp → src/foo.o
$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(TARGET)
