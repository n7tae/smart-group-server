# Copyright (c) 2017 by Thomas A. Early N7TAE
BINDIR=/usr/local/bin
CFGDIR=/usr/local/etc
LOGDIR=/var/log

CPPFLAGS=-W -Wall -I/usr/include -std=c++11
LDFLAGS=-L/usr/lib -lconfig++

OBJS = SmartServerConfig.o

smartgroup : $(OBJS)

SmartServerConfig.o : SmartServerConfig.cpp SmartServerConfig.h Defs.h
	g++ -c $(CPPFLAGS) SmartServerConfig.cpp
