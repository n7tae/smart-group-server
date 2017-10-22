# Copyright (c) 2017 by Thomas A. Early N7TAE
BINDIR=/usr/local/bin
CFGDIR=/usr/local/etc
LOGDIR=/var/log

CPPFLAGS=-W -Wall -I/usr/include -std=c++11 -DDATA_DIR=\"$(CFGDIR)\" -DDEXTRA_LINK
LDFLAGS=-L/usr/lib -lconfig++

OBJS = AMBEData.o AnnouncementUnit.o APRSCollector.o APRSWriter.o APRSWriterThread.o AudioUnit.o CacheManager.o CallsignList.o CCITTChecksum.o ConnectData.o DDData.o DExtraProtocolHandler.o DPlusProtocolHandler.o DRATSServer.o DVTOOLFileReader.o G2ProtocolHandler.o GatewayCache.o HeaderData.o HeaderLogger.o HeardData.o IRCDDB.o PollData.o RemoteLinkData.o RemoteRepeaterData.o RemoteStarNetUser.o RemoteStarNetGroup.o RepeaterCache.o SlowDataEncoder.o SmartServerConfig.o StarNetHandler.o StatusData.o TCPReaderWriterClient.o TCPReaderWriterServer.o TextCollector.o TextData.o Timer.o UserCache.o UDPReaderWriter.o Utils.o

smartgroup : $(OBJS)

AMBEData.o : AMBEData.cpp AMBEData.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) AMBEData.cpp

AnnouncementUnit.o : AnnouncementUnit.cpp AnnouncementUnit.h DStarDefines.h HeaderData.h Utils.h
	g++ -c $(CPPFLAGS) AnnouncementUnit.cpp

APRSCollector.o : APRSCollector.cpp APRSCollector.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) APRSCollector.cpp

APRSWriter.o : APRSWriter.cpp APRSWriter.h Utils.h
	g++ -c $(CPPFLAGS) APRSWriter.cpp

APRSWriterThread.o : APRSWriterThread.cpp APRSWriterThread.h DStarDefines.h Utils.h Defs.h
	g++ -c $(CPPFLAGS) APRSWriterThread.cpp

AudioUnit.o : AudioUnit.cpp AudioUnit.h DStarDefines.h HeaderData.h Utils.h
	g++ -c $(CPPFLAGS) AudioUnit.cpp

CacheManager.o : CacheManager.cpp CacheManager.h DStarDefines.h
	g++ -c $(CPPFLAGS) CacheManager.cpp

CallsignList.o : CallsignList.cpp CallsignList.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) CallsignList.cpp

CCITTChecksum.o : CCITTChecksum.cpp CCITTChecksum.h Utils.h
	g++ -c $(CPPFLAGS) CCITTChecksum.cpp

ConnectData.o : ConnectData.cpp ConnectData.h DStarDefines.h Version.h Utils.h
	g++ -c $(CPPFLAGS) ConnectData.cpp

DDData.o : DDData.cpp DDData.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) DDData.cpp

DExtraProtocolHandler.o : DExtraProtocolHandler.cpp DExtraProtocolHandler.h Utils.h
	g++ -c $(CPPFLAGS) DExtraProtocolHandler.cpp

DPlusProtocolHandler.o : DPlusProtocolHandler.cpp DPlusProtocolHandler.h Utils.h
	g++ -c $(CPPFLAGS) DPlusProtocolHandler.cpp

DRATSServer.o : DRATSServer.cpp DRATSServer.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) DRATSServer.cpp

DVTOOLFileReader.o : DVTOOLFileReader.cpp DVTOOLFileReader.h DStarDefines.h
	g++ -c $(CPPFLAGS) DVTOOLFileReader.cpp

G2ProtocolHandler.o : G2ProtocolHandler.cpp G2ProtocolHandler.h UDPReaderWriter.h Utils.h HeaderData.h AMBEData.h
	g++ -c $(CPPFLAGS) G2ProtocolHandler.cpp

GatewayCache.o : GatewayCache.cpp GatewayCache.h
	g++ -c $(CPPFLAGS) GatewayCache.cpp

HeaderData.o : HeaderData.cpp HeaderData.h CCITTChecksum.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) HeaderData.cpp

HeaderLogger.o : HeaderLogger.cpp HeaderLogger.h Utils.h Defs.h
	g++ -c $(CPPFLAGS) HeaderLogger.cpp

HeardData.o : HeardData.cpp HeardData.h
	g++ -c $(CPPFLAGS) HeardData.cpp

IRCDDB.o : IRCDDB.cpp IRCDDB.h
	g++ -c $(CPPFLAGS) IRCDDB.cpp

PollData.o : PollData.cpp PollData.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) PollData.cpp

RemoteLinkData.o : RemoteLinkData.cpp RemoteLinkData.h
	g++ -c $(CPPFLAGS) RemoteLinkData.cpp

RemoteRepeaterData.o : RemoteRepeaterData.cpp RemoteRepeaterData.h
	g++ -c $(CPPFLAGS) RemoteRepeaterData.cpp

RemoteStarNetUser.o : RemoteStarNetUser.cpp RemoteStarNetUser.h
	g++ -c $(CPPFLAGS) RemoteStarNetUser.cpp

RemoteStarNetGroup.o : RemoteStarNetGroup.cpp RemoteStarNetGroup.h RemoteStarNetUser.h
	g++ -c $(CPPFLAGS) RemoteStarNetGroup.cpp

RepeaterCache.o : RepeaterCache.cpp RepeaterCache.h
	g++ -c $(CPPFLAGS) RepeaterCache.cpp

SlowDataEncoder.o : SlowDataEncoder.cpp SlowDataEncoder.h CCITTChecksum.h DStarDefines.h
	g++ -c $(CPPFLAGS) SlowDataEncoder.cpp

SmartServerConfig.o : SmartServerConfig.cpp SmartServerConfig.h Defs.h
	g++ -c $(CPPFLAGS) SmartServerConfig.cpp

StarNetHandler.o : StarNetHandler.cpp StarNetHandler.h SlowDataEncoder.h RepeaterHandler.h DExtraHandler.h DStarDefines.h DCSHandler.h Utils.h
	g++ -c $(CPPFLAGS) StarNetHandler.cpp

StatusData.o : StatusData.cpp StatusData.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) StatusData.cpp

TCPReaderWriterClient.o : TCPReaderWriterClient.cpp TCPReaderWriterClient.h UDPReaderWriter.h Utils.h
	g++ -c $(CPPFLAGS) TCPReaderWriterClient.cpp

TCPReaderWriterServer.o : TCPReaderWriterServer.cpp TCPReaderWriterServer.h Utils.h
	g++ -c $(CPPFLAGS) TCPReaderWriterServer.cpp

TextCollector.o : TextCollector.cpp TextCollector.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) TextCollector.cpp

TextData.0 : TextData.cpp TextData.h DStarDefines.h Utils.h
	g++ -c $(CPPFLAGS) TextData.cpp

Timer.o : Timer.cpp Timer.h
	g++ -c $(CPPFLAGS) Timer.cpp

UDPReaderWriter.o : UDPReaderWriter.cpp
	g++ -c $(CPPFLAGS) UDPReaderWriter.cpp

UserCache.o : UserCache.cpp UserCache.h
	g++ -c $(CPPFLAGS) UserCache.cpp

Utils.o : Utils.cpp Utils.h
	g++ -c $(CPPFLAGS) Utils.cpp

AnnouncementData.h : DVTOOLFileReader.h RepeaterCallback.h AMBEData.h Timer.h Defs.h
APRSCollector.h : Defs.h
APRSWriter.h : APRSWriterThread.h APRSCollector.h DStarDefines.h AMBEData.h Timer.h Defs.h
APRSWriterThread.h : TCPReaderWriterClient.h
AudioUnit.h : RepeaterCallback.h SlowDataEncoder.h AMBEData.h Timer.h Defs.h
CacheManager.h : RepeaterCache.h GatewayCache.h UserCache.h
CCSCallback.h : DStarDefines.h HeaderData.h AMBEData.h Defs.h
ConnectData.h : Defs.h
DDData.h : HeaderData.h
DExtraProtocolHandler.h : UDPReaderWriter.h DStarDefines.h ConnectData.h HeaderData.h AMBEData.h PollData.h
DPlusProtocolHandler.h : UDPReaderWriter.h DStarDefines.h ConnectData.h HeaderData.h AMBEData.h PollData.h
DRATSServer.h : TCPReaderWriterServer.h RepeaterCallback.h HeaderData.h AMBEData.h Defs.h
DVTOOLFileReader.h : HeaderData.h AMBEData.h
HeardData.h : DStarDefines.h HeaderData.h
HeaderLogger.h : HeaderData.h DDData.h
PollData.h : Defs.h
ReflectorCallback.h : DStarDefines.h HeaderData.h AMBEData.h Defs.h
RemoteLinkData.h : Defs.h
RemoteRepeaterData.h : RemoteLinkData.h
RepeaterCallback.h : DStarDefines.h HeaderData.h AMBEData.h Defs.h
SlowDataEncoder.h : HeaderData.h
StarNetHandler.h : RemoteStarNetGroup.h G2ProtocolHandler.h ReflectorCallback.h RepeaterCallback.h TextCollector.h CacheManager.h HeaderData.h AMBEData.h IRCDDB.h Timer.h
TCPReaderWriterServer.h : TCPReaderWriterClient.h
TextCollector.h : AMBEData.h Defs.h
TextData.h : Defs.h
