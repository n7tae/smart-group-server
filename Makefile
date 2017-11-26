# Copyright (c) 2017 by Thomas A. Early N7TAE

# if you change these locations, make sure the service.smartgroupserver script is updated!
BINDIR=/usr/local/bin
CFGDIR=/usr/local/etc

CPPFLAGS=-g -ggdb -W -Wall -I/usr/include -std=c++11 -DDATA_DIR=\"$(CFGDIR)\" -DDEXTRA_LINK
#CPPFLAGS=-g -ggdb -W -Wall -I/usr/include -std=c++11 -DDATA_DIR=\"$(CFGDIR)\"
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

# install and uninstall need root priviledges
install : smartgroupserver
	/usr/bin/wget ftp://dschost1.w6kd.com/DExtra_Hosts.txt
	/bin/mv DExtra_Hosts.txt $(CFGDIR)
	/usr/bin/wget ftp://dschost1.w6kd.com/DCS_Hosts.txt
	/bin/mv DCS_Hosts.txt $(CFGDIR)
	/bin/cp -f smartgroupserver $(BINDIR)
	/bin/cp -f smartgroupserver.cfg $(CFGDIR)
	/bin/cp -f service.smartgroupserver /etc/init.d/smartgroupserver
	/usr/sbin/update-rc.d smartgroupserver defaults
	/usr/sbin/update-rc.d smartgroupserver enable

uninstall :
	/usr/sbin/service smartgrouppserver stop
	/bin/rm -f /etc/init.d/smartgroupserver
	/usr/sbin/update-rc.d smartgroupserver remove
	/bin/rm -f $(BINDIR)/smartgroupserver
	/bin/rm -f $(CFGDIR)/smartgroupserver.cfg
	/bin/rm -f $(CFGDIR)/DExtra_Hosts.txt
	/bin/rm -f $(CFGDIR)/DCS_Hosts.txt
