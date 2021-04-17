# speedwire-router
A C++ executable to route sma speedwire packets between subnets and to individual hosts residing on different subnets.

The executable opens sockets on all available host interfaces. Each inbound SMA(TM) speedwire unicast and multicast packet on a given host interface is forwarded to each of the other host interfaces. A bounce detecter is implemented to prevent packets bouncing infinitely between subnets.

This is interesting for different use cases
1. You have speedwire devices residing in two different subnets. A lot of speedwire communication is handled through multicast udp packets. Multicast packets will not pass subnet boundaries. Executing the speedwire-router executable on a host that is connected to both subnets will solve this problem. You can also extend this scheme to three or more subnets; just make sure the bounce detector has enough space for packet history.
2. You have individual speedwire devices residing in a different subnet or somewhere on the internet. This can be solved by running the speedwire-router executable in your local subnet (where the multicast traffic is originating from) and pre-registering the IP address(es) of the individual devices by calling discoverer.preRegisterDevice("YOUR.IP.ADDRESS.HERE") in main.cpp. Inbound unicast and multicast packets on any of the available host interfaces will be forwarded as unicast packets to the configured individual devices.

As an additional benefit you can modify or patch the packet contents before routing them. 

The software comes as is. No warrantees whatsoever are given and no responsibility is assumed in case of failure. There is neither a GUI nor a configuration file. Configurations must be tweaked by modifying main.cpp.

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

Please see the description in https://github.com/RalfOGit/libspeedwire on how to setup this folder structure.

The code has been tested against the following environment:

    OS: CentOS 8(TM), IDE: VSCode (TM)
    OS: Windows 10(TM), IDE: Visual Studio Community Edition 2019 (TM)
