# Copyright (c) 2017 by Thomas A. Early N7TAE
BINDIR=/usr/local/bin
CFGDIR=/usr/local/etc
LOGDIR=/var/log

CPPFLAGS=-W -Wall -I/usr/include -std=c++11 -DDATA_DIR=$(CFGDIR) -DDEXTRA_LINK
LDFLAGS=-L/usr/lib -lconfig++

OBJS = AMBEData.o CacheManager.o CCITTChecksum.o G2ProtocolHandler.o GatewayCache.o HeaderData.o SmartServerConfig.o RemoteStarNetUser.o RemoteStarNetGroup.o RepeaterCache.o TextCollector.o UserCache.o UDPReaderWriter.o Utilities.o Utils.o

smartgroup : $(OBJS)

AMBEData.o : AMBEData.cpp AMBEData.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) AMBEData.cpp

CacheManager.o : CacheManager.cpp CacheManager.h DStarDefines.h
	g++ -c $(CPPFLAGS) CacheManager.cpp

CCITTChecksum.o : CCITTChecksum.cpp CCITTChecksum.h Utils.h
	g++ -c $(CPPFLAGS) CCITTChecksum.cpp

G2ProtocolHandler.o : G2ProtocolHandler.cpp G2ProtocolHandler.h UDPReaderWriter.h Utils.h HeaderData.h AMBEData.h
	g++ -c $(CPPFLAGS) G2ProtocolHandler.cpp

GatewayCache.o : GatewayCache.cpp GatewayCache.h
	g++ -c $(CPPFLAGS) GatewayCache.cpp

HeaderData.o : HeaderData.cpp HeaderData.h CCITTChecksum.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) HeaderData.cpp

SmartServerConfig.o : SmartServerConfig.cpp SmartServerConfig.h Defs.h
	g++ -c $(CPPFLAGS) SmartServerConfig.cpp

RemoteStarNetUser.o : RemoteStarNetUser.cpp RemoteStarNetUser.h
	g++ -c $(CPPFLAGS) RemoteStarNetUser.cpp

RemoteStarNetGroup.o : RemoteStarNetGroup.cpp RemoteStarNetGroup.h RemoteStarNetUser.h
	g++ -c $(CPPFLAGS) RemoteStarNetGroup.cpp

RepeaterCache.o : RepeaterCache.cpp RepeaterCache.h
	g++ -c $(CPPFLAGS) RepeaterCache.cpp

TextCollector.o : TextCollector.cpp TextCollector.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) TextCollector.cpp

UDPReaderWriter.o : UDPReaderWriter.cpp Utilities.h
	g++ -c $(CPPFLAGS) UDPReaderWriter.cpp

UserCache.o : UserCache.cpp UserCache.h
	g++ -c $(CPPFLAGS) UserCache.cpp

Utils.o : Utils.cpp Utils.h
	g++ -c $(CPPFLAGS) Utils.cpp

Utilities.o : Utilities.cpp Utilities.h
	g++ -c $(CPPFLAGS) Utilities.cpp

CacheManager.h : RepeaterCache.h GatewayCache.h UserCache.h
ReflectorCallback.h : DStarDefines.h HeaderData.h AMBEData.h Defs.h
RepeaterCallback.h : DStarDefines.h HeaderData.h AMBEData.h Defs.h
TextCollector.h : AMBEData.h Defs.h
