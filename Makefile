# Copyright (c) 2017 by Thomas A. Early N7TAE

# if you change these locations, make sure the sgs.service file is updated!
BINDIR=/usr/local/bin
CFGDIR=/usr/local/etc

# choose this if you want debugging help
#CPPFLAGS=-g -ggdb -W -Wall -std=c++11 -DCFG_DIR=\"$(CFGDIR)\"
# or, you can choose this for a much smaller executable without debugging help
CPPFLAGS=-W -Wall -std=c++11 -DCFG_DIR=\"$(CFGDIR)\"

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

sgs :  $(OBJS)
	g++ $(CPPFLAGS) -o sgs $(OBJS) -lconfig++ -pthread

%.o : %.cpp
	g++ $(CPPFLAGS) -MMD -MD -c $< -o $@

.PHONY: clean

clean:
	$(RM) $(OBJS) $(DEPS) sgs

-include $(DEPS)

# newhostfiles, install, uninstall and removehostfiles need root priviledges
newhostfiles :
	/bin/rm -f $(CFGDIR)/DExtra_Hosts.txt
	/bin/rm -f $(CFGDIR)/DCS_Hosts.txt
	/usr/bin/wget ftp://dschost1.w6kd.com/DExtra_Hosts.txt
	/usr/bin/wget ftp://dschost1.w6kd.com/DCS_Hosts.txt
	/bin/mv DExtra_Hosts.txt $(CFGDIR)
	/bin/mv DCS_Hosts.txt $(CFGDIR)

install : sgs
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

removehostfiles :
	/bin/rm -f $(CFGDIR)/DExtra_Hosts.txt
	/bin/rm -f $(CFGDIR)/DCS_Hosts.txt
