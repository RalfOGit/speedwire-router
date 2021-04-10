# speedwire-router
A C++ executable to route sma speedwire packets between subnets and to individual hosts on residing on different subnets.

The executable opens sockets on all available host interfaces. Each inbound SMA(TM) speedwire unicast and multicast packet on a given host interface is forwarded to each of the other host interfaces. A bounce detecter is implemented to prevent packets bouncing infinitely between subnets.

Ideally this executable is running a host connecting the two or more subnets you would like to route speedwire packets between. But you can also configure individual hosts on a different subnets. In the latter case inbound unicast and multicast packets are forwarded as unicast packets to the configured individual hosts.

The software comes as is. No warrantees whatsoever are given and no responsibility is assumed in case of failure. There is neither a GUI nor a configuration file. Configurations must be tweaked by modifying main.cpp. A number of obis definitions are given, some of them are commented out, since I do not need them.

The code is based on a Speedwire(TM) access library implementation https://github.com/RalfOGit/libspeedwire. The libspeedwire library implements a full parser for the sma header and the emeter datagram structure, including obis filtering. In addition, it implements some parsing functionality for inverter query and response datagrams. For convenience you may want to place the libspeedwire/ folder right next to the src/ and include/ folders of this repository.

The accompanied CMakeLists.txt assumes the following folder structure:

speedwire-router
    src
    include
    libspeedwire
        src
        include
        CMakeLists.txt
    ... build path ...
    CMakeLists.txt

The code has been tested against the following environment:

OS: CentOS 8(TM), IDE: VSCode (TM)
OS: Windows 10(TM), IDE: Visual Studio Community Edition 2019 (TM)
