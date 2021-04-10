#ifndef __SPEEDWIREPACKETSENDER_HPP__
#define __SPEEDWIREPACKETSENDER_HPP__

#include <LocalHost.hpp>
#include <SpeedwireHeader.hpp>
#include <SpeedwireEmeterProtocol.hpp>
#include <SpeedwireInverterProtocol.hpp>


/**
 *  Speedwire packet sender base class
 */
class SpeedwirePacketSender {
protected:
    const LocalHost& local_host;
    std::string peer_ip;
    std::string local_interface_ip;
    bool is_ipv4;
    bool is_ipv6;
    struct in_addr  local_interface_in_addr;
    struct in6_addr local_interface_in6_addr;
    uint32_t local_interface_prefix_length;

public:
    SpeedwirePacketSender(const LocalHost& localhost, const std::string& local_interface_ip, const std::string& peer_ip);
    virtual void send(SpeedwireHeader& packet, struct sockaddr& src) {};
};


/**
 *  Speedwire packet sender class for multicast packets
 */
class MulticastPacketSender : public SpeedwirePacketSender {
public:
    MulticastPacketSender(const LocalHost& local_host, const std::string& local_interface, const std::string& peer_ip);
    virtual void send(SpeedwireHeader& packet, struct sockaddr& src);
};


/**
 *  Speedwire packet sender class for unicast packets
 */
class UnicastPacketSender : public SpeedwirePacketSender {
public:
    UnicastPacketSender(const LocalHost& local_host, const std::string& local_interface, const std::string& peer_ip);
    virtual void send(SpeedwireHeader& packet, struct sockaddr& src);
};

#endif
