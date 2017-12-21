smart-group-server
==================
## Introduction

This smart-group-server is based on Jonathan G4KLX's StarNetServer. It was designed expressly for QuadNet. The smart-group-server interact with QuadNet using IRC messages to provide additional information that will typically be display on the QuadNet web page at openquad.net. The smart-group-server may not function proplerly on other IRCDDB networks.

The smart-group-server requires a modern OS to compile and run. At least Debian 8 or Ubuntu 16.10, or equivilent. The command
```
g++ --version
```
must return at least Version 4.9. The latest Debian and Ubuntu will be far above ths. Unlike the StarNetServer, smart-group-server does not use wxWidgits. Modern C++ calls to the standard library (c++11 standard) are used instead of wxWidgets: std::string replaces wxString, std::future replaces wxThreads and standard std::map, std::list, std::queue and std::vector replace the older wx containers. The only external library used is libconfig++. The smart-group-server is significantly improved regarding resource utiliztion compared to the ancestral StarNetServer. The smart-group-server only creates resources for the channel you define in your configuration file. Also, there is no theoretical limit to the number of channels you can create. However there is a practical limit based on the underlying hardware.

The smart-group-server is installed as a systemd service. If you want to run this on a system without systemd, you are on your own. I am done dealing with init.d scripts in SysIVInit!

## Adminstrative Requirements

This Smart Group Server should have a unique IP address when it logs into QuadNet. That means you probably won't be able to run it from your home if you also have an ircddb gateway running from home. You probably shouldn't run it from your home anyway. The computer your Smart Group Server is running on should have reliable, 24/7 internet access and reliable, 24/7 power. It should also be properly protected from hackers. There are plenty of companies that provide virtual severs that easily fulfill these requirements for verly little money. (You don't need much horse-power for a typical Smart Group Server. For example, a $5/month server on Amazon Lightsail works fine.)

Also the Smart Group Server needs to have a unique callsign in QuadNet. Ideally, you should use a Club callsign, see the Configuring section below.

## Building

These instructions are for a Debian-based OS. Begin by downloading this git repository:
```
git clone git://github.com/n7tae/smart-group-server.git
```
Install the only needed development library:
```
sudo apt-get install libconfig++-dev
```
Change to the smart-group-server directory and type `make`. This should make the executable, `sgs` without errors or warnings. By default, you will have a group server that can link groups to X-Reflectors. To link to DCS reflectors (or for no linking) modify the `Makefile` before you make. If you change you mind about linking, do a `make clean` before you do another make.

## Configuring

Before you install the group server, you need to create a configuration file called `sgs.cfg`. There is an example configuration file: `example.cfg`. The smart-group-server supports an unlimited number of channels. However there will be a practical limit based on you hardware capability. Also remember that a unique port is created for each DExtra or DCS link on a running smart-group-server. At some point you system will simply run out of connections. Be sure you look and the "StarNet Groups" tab on the openquad.net web page to be sure you channel callsigns are not already in use! Each channel you define requires a band letter. Bands can be shared between channels. Choose any uppercase letter from 'A' to 'Z'. Each channel will have a group logon callsign and a group logoff callsign. The logon and logoff will differ only in the last letter of the callsign. PLEASE DON'T CHOOSE a channel callsign beginning in "REF", "XRF", "XLX", "DCS" or "CCS". While it is possible, it's really confusing for new-comers on QuadNet.

Your callsign parameter in the ircddb section of your configuration file is the callsign that will be used for logging into QuadNet. THIS NEEDS TO BE A UNIQUE CALLSIGN on QuadNet. Don't use your callsign if you are already using it for a repeater or a hot-spot. Ideally, you should use a Club callsign. Check with your club to see if you can use your club's callsign. If you don't have access to one, either apply to be a trustee for a new callsign from you club, or get together with three of your friends and start a club. All the information you need is at arrl.org or w5yi.org. It's not difficult to do, and once you file your application, you'll get your new Club Callsign very quickly.

## Installing and Uninstalling

To install and start the smart-group-server, type `sudo make install`. This will download the necessary Hosts files for linking to DExtra for DCS reflectors and put all the necessary files in /usr/local. See the Makefile for more information. To uninstall it, type `sudo make uninstall`.

73

Tom
n7tae (at) arrl (dot) net
