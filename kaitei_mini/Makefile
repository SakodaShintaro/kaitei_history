CXX = g++
TARGET = kaitei_mini
CXXFLAGS = -O3 -std=c++14
LDFLAGS = -pthread
SRCS  = $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

$(TARGET) : $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

all : $(OBJS)

clean:
	rm -f $(TARGET) $(OBJS)
