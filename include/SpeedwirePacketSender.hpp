#ifndef __SPEEDWIREPACKETSENDER_HPP__
#define __SPEEDWIREPACKETSENDER_HPP__

#ifdef _WIN32
#include <Winsock2.h>
#include <ws2ipdef.h>
#include <inaddr.h>
#include <in6addr.h>
#else
#include <netinet/in.h>
#include <net/if.h>
#endif
#include <LocalHost.hpp>
#include <SpeedwireHeader.hpp>
#include <SpeedwireEmeterProtocol.hpp>
#include <SpeedwireInverterProtocol.hpp>


/**
 *  Speedwire packet sender base class
 */
class SpeedwirePacketSender {
protected:
    const libspeedwire::LocalHost& local_host;
    std::string peer_ip;
    std::string local_interface_ip;
    bool is_ipv4;
    bool is_ipv6;
    struct in_addr  local_interface_in_addr;
    struct in6_addr local_interface_in6_addr;
    uint32_t local_interface_prefix_length;

public:
    SpeedwirePacketSender(const libspeedwire::LocalHost& localhost, const std::string& local_interface_ip, const std::string& peer_ip);
    virtual void send(libspeedwire::SpeedwireHeader& packet, const struct sockaddr& src) {};
};


/**
 *  Speedwire packet sender class for multicast packets
 */
class MulticastPacketSender : public SpeedwirePacketSender {
public:
    MulticastPacketSender(const libspeedwire::LocalHost& local_host, const std::string& local_interface, const std::string& peer_ip);
    virtual void send(libspeedwire::SpeedwireHeader& packet, const struct sockaddr& src);
};


/**
 *  Speedwire packet sender class for unicast packets
 */
class UnicastPacketSender : public SpeedwirePacketSender {
public:
    UnicastPacketSender(const libspeedwire::LocalHost& local_host, const std::string& local_interface, const std::string& peer_ip);
    virtual void send(libspeedwire::SpeedwireHeader& packet, const struct sockaddr& src);
};

#endif
