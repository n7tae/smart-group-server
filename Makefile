# Copyright (c) 2017 by Thomas A. Early N7TAE
BINDIR=/usr/local/bin
CFGDIR=/usr/local/etc
LOGDIR=/var/log

CPPFLAGS=-g -ggdb -W -Wall -I/usr/include -std=c++11 -DDATA_DIR=\"$(CFGDIR)\" -DDEXTRA_LINK
LDFLAGS=-L/usr/lib -lconfig++

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

smartgroupserver :  $(OBJS)
	g++ $(CPPFLAGS) -o smartgroupserver $(OBJS) -L/usr/lib -lconfig++ -pthread

%.o : %.cpp
	g++ $(CPPFLAGS) -MMD -MD -c $< -o $@

.PHONY: clean

clean:
	$(RM) $(OBJS) $(DEPS) smartgroupserver

-include $(DEPS)
