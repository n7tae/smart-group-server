# Copyright (c) 2017 by Thomas A. Early N7TAE
BINDIR=/usr/local/bin
CFGDIR=/usr/local/etc
LOGDIR=/var/log

CPPFLAGS=-W -Wall -I/usr/include -std=c++11 -DDATA_DIR=$(CFGDIR) -DDEXTRA_LINK
LDFLAGS=-L/usr/lib -lconfig++

OBJS = AMBEData.o CCITTChecksum.o G2ProtocolHandler.o HeaderData.o SmartServerConfig.o RemoteStarNetUser.o RemoteStarNetGroup.o UDPReaderWriter.o Utilities.o Utils.o

smartgroup : $(OBJS)

AMBEData.o : AMBEData.cpp AMBEData.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) AMBEData.cpp

CCITTChecksum.o : CCITTChecksum.cpp CCITTChecksum.h Utils.h
	g++ -c $(CPPFLAGS) CCITTChecksum.cpp

G2ProtocolHandler.o : G2ProtocolHandler.cpp G2ProtocolHandler.h UDPReaderWriter.h Utils.h HeaderData.h AMBEData.h
	g++ -c $(CPPFLAGS) G2ProtocolHandler.cpp

HeaderData.o : HeaderData.cpp HeaderData.h CCITTChecksum.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) HeaderData.cpp

SmartServerConfig.o : SmartServerConfig.cpp SmartServerConfig.h Defs.h
	g++ -c $(CPPFLAGS) SmartServerConfig.cpp

RemoteStarNetUser.o : RemoteStarNetUser.cpp RemoteStarNetUser.h
	g++ -c $(CPPFLAGS) RemoteStarNetUser.cpp

RemoteStarNetGroup.o : RemoteStarNetGroup.cpp RemoteStarNetGroup.h RemoteStarNetUser.h
	g++ -c $(CPPFLAGS) RemoteStarNetGroup.cpp

UDPReaderWriter.o : UDPReaderWriter.cpp Utilities.h
	g++ -c $(CPPFLAGS) UDPReaderWriter.cpp

Utils.o : Utils.cpp Utils.h
	g++ -c $(CPPFLAGS) Utils.cpp

Utilities.o : Utilities.cpp Utilities.h
	g++ -c $(CPPFLAGS) Utilities.cpp
