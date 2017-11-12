CXX = g++
TARGET = kaitei_SDT5
CXXFLAGS = -O3 -std=c++14 -mbmi2 -mbmi
LDFLAGS = -pthread -lstdc++fs
SRCS  = $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

$(TARGET) : $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

all : $(OBJS)

clean:
	rm -f $(TARGET) $(OBJS)
