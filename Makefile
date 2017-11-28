# Copyright (c) 2017 by Thomas A. Early N7TAE

# if you change these locations, make sure the service.smartgroupserver script is updated!
BINDIR=/usr/local/bin
CFGDIR=/usr/local/etc

CPPFLAGS=-g -ggdb -W -Wall -I/usr/include -std=c++11 -DCFG_DIR=\"$(CFGDIR)\" -DDEXTRA_LINK
#CPPFLAGS=-g -ggdb -W -Wall -I/usr/include -std=c++11 -DDATA_DIR=\"$(CFGDIR)\"
LDFLAGS=-L/usr/lib -lconfig++

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

sgs :  $(OBJS)
	g++ $(CPPFLAGS) -o sgs $(OBJS) -L/usr/lib -lconfig++ -pthread

%.o : %.cpp
	g++ $(CPPFLAGS) -MMD -MD -c $< -o $@

.PHONY: clean

clean:
	$(RM) $(OBJS) $(DEPS) smartgroupserver

-include $(DEPS)

# install and uninstall need root priviledges
install : sgs
	/usr/bin/wget ftp://dschost1.w6kd.com/DExtra_Hosts.txt
	/bin/mv DExtra_Hosts.txt $(CFGDIR)
	/usr/bin/wget ftp://dschost1.w6kd.com/DCS_Hosts.txt
	/bin/mv DCS_Hosts.txt $(CFGDIR)
	/bin/cp -f sgs $(BINDIR)
	/bin/cp -f sgs.cfg $(CFGDIR)
	/bin/cp -f sgs.service /lib/systemd/system
	systemctl enable sgs.service
	systemctl daemon-reload
	systemctl start sgs.service

uninstall :
	systemctl stop sgs.service
	systemctl disable sgs.service
	/bin/rm -f /lib/systemd/system/sgs.service
	systemctl daemon-reload
	/bin/rm -f $(BINDIR)/sgs
	/bin/rm -f $(CFGDIR)/sgs.cfg
	/bin/rm -f $(CFGDIR)/DExtra_Hosts.txt
	/bin/rm -f $(CFGDIR)/DCS_Hosts.txt
