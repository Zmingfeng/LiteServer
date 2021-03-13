CXX = g++
CFLAGHS = -std=c++14 -O2 -Wall -g

FILE  = server

OBJS = ../code/pools/*.cpp ../code/timer/*.cpp ../code/http/*.cpp ../code/epoll/*.cpp \
       ../code/webserver/*.cpp ../code/buffer/*.cpp ../code/main.cpp

TARGET = ../bin/$(FILE)


all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS)  -o  $(TARGET) -pthread -lmysqlclient
clean:
	rm -rf $(FILE)
