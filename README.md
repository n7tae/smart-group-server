smart-group-server
==================
##Introduction

This smart-group-server is based on Jonathan G4KLX's StarNetServer. It was designed expressly for QuadNet. The smart-group-server interact with QuadNet using IRC messages to provide additional information that will typically be display on the QuadNet web page at openquad.net. The smart-group-server may not function proplerly on other IRCDDB networks.

The smart-group-server requires a modern OS to compile and run. At least Debian 8 or Ubuntu 16.10, or equivilent. Unlike the StarNetServer, smart-group-server does not use wxWidgits. Modern C++ calls to the standard library (c++11 standard) are used instead of wxWidgets: std::string replaces wxString, std::future replaces wxThreads and standard std::map, std::list, std::queue and std::vector replace the wx containers. The only external library used is libconfig++. In addtion, the smart-group-server is installed as a systemd service. If you want to run this on a system without systemd, you are on your own. I am done dealing with init.d scripts in SysIVInit!

##Building

These instructions are for a Debian-based OS. Begin by downloading this git repository:
```
git clone git:/github.com/n7tae/smart-group-server.git
```
Install the only needed development library:
```
sudo apt-get install libconfig++-dev
```
Change to the smart-group-server directory and type `make`. This should make the executable, `sgs` without errors or warnings. By default, you will have a groups that can link to X-Reflectors. To link to DCS reflectors (or for no linking) modify the `Makefile` before you make. If you change you mind about linking, do a `make clean` before you do another make.

##Configuring
Before you install the group server, you need to create a configuration file called `sgs.cfg`. There is an example configuration file: `example.cfg`. Currently smart-group-server supports up to 15 channels. Besure you look and the "StarNet Groups" tab on the openquad.net web page to be sure you channel callsigns are not already in use! Each channel you define requires a unique band letter. Choose any uppercase letter from 'A' to 'Z'. Each channel will have a group logon callsign and a group logoff callsign. The logon and logoff will differ only in the last letter of the callsign. PLEASE DON'T CHOOSE a channel callsign beginning in "REF", "XRF", "XLX", "DCS" or "CCS". That is really confusing for new-comers.

##Installing and Uninstalling
To install and start the smart-group-server, type `sudo make install`. This will download the necessary Hosts files for linking to DExtra for DCS reflectors and put all the necessary files in /usr/local. See the Makefile for more information. To uninstall it, type `sudo make uninstall`.

73

Tom
n7tae (at) arrl (dot) net
