# Copyright (c) 2017 by Thomas A. Early N7TAE

# if you change these locations, make sure the sgs.service file is updated!
BINDIR=/usr/local/bin
CFGDIR=/usr/local/etc

# check for Fedora or something else
REDHAT=$(shell [ -f /etc/redhat-release ] && echo 1 || echo 0)
# see if sgs user already exists
SGSUSER=$(shell id -u sgs >/dev/null 2>&1 && echo 1 || echo 0 )

# choose this if you want debugging help
#CPPFLAGS=-ggdb -W -std=c++11 -DCFG_DIR=\"$(CFGDIR)\"
# or, you can choose this for a much smaller executable without debugging help
CPPFLAGS=-W -std=c++11 -DCFG_DIR=\"$(CFGDIR)\"

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

sgs :  $(OBJS)
	g++ $(CPPFLAGS) -o sgs $(OBJS) -lconfig++ -lssl -lcrypto -pthread

%.o : %.cpp
	g++ $(CPPFLAGS) -MMD -MD -c $< -o $@

sgs.crt sgs.key :
	openssl req -new -newkey rsa:4096 -days 36500 -nodes -x509 -subj "/CN=Smart Group Server" -keyout sgs.key  -out sgs.crt

.PHONY: clean

clean:
	$(RM) $(OBJS) $(DEPS) sgs

-include $(DEPS)

# install, uninstall and removehostfiles need root priviledges
newhostfiles :
	/usr/bin/wget http://www.pistar.uk/downloads/DExtra_Hosts.txt && sudo /bin/mv -f DExtra_Hosts.txt $(CFGDIR)
	/usr/bin/wget http://www.pistar.uk/downloads/DCS_Hosts.txt && sudo /bin/mv -f DCS_Hosts.txt $(CFGDIR)

install : sgs sgs.key sgs.crt sgs.cfg
	/bin/cp -f sgs $(BINDIR)
# Fedora defaults to perm 700 for home directories, so we cannot symlink sgs.cfg
	if [ $(REDHAT) = 1 ]; then /bin/cp -f $(shell pwd)/sgs.cfg $(CFGDIR); else /bin/ln -s $(shell pwd)/sgs.cfg $(CFGDIR); fi
	/bin/cp -f sgs.service /lib/systemd/system
	/bin/cp -f sgs.crt sgs.key $(CFGDIR)
	if [ $(SGSUSER) = 0 ]; then /usr/sbin/useradd -d /tmp -M -s /usr/sbin/nologin -r sgs; fi
	chown sgs:sgs $(CFGDIR)/sgs.key
	systemctl enable sgs.service
	systemctl daemon-reload
	systemctl start sgs.service

uninstall :
	systemctl stop sgs.service
	systemctl disable sgs.service
	/bin/rm -f /lib/systemd/system/sgs.service
	systemctl daemon-reload
	/bin/rm -f $(BINDIR)/sgs
# On Fedora, save current sgs.cfg in working directory before removing
	if [ $(REDHAT) = 1 ]; then /bin/cp -f $(CFGDIR)/sgs.cfg $(shell pwd)/sgs.cfg; fi
	/bin/rm -f $(CFGDIR)/sgs.cfg
	/bin/rm -f $(CFGDIR)/sgs.key
	/bin/rm -f $(CFGDIR)/sgs.crt
	/usr/sbin/userdel sgs

removehostfiles :
	/bin/rm -f $(CFGDIR)/DExtra_Hosts.txt
	/bin/rm -f $(CFGDIR)/DCS_Hosts.txt
