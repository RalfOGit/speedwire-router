#include <memory.h>
#include <LocalHost.hpp>
#include <AddressConversion.hpp>
#include <Logger.hpp>
#include <SpeedwirePacketSender.hpp>
#include <SpeedwireSocketFactory.hpp>
#include <SpeedwireSocket.hpp>

static Logger logger = Logger("SpeedwirePacketSender");


/**
 *  Speedwire packet sender base class
 */
SpeedwirePacketSender::SpeedwirePacketSender(const LocalHost& _localhost, const std::string& _local_interface_ip, const std::string& _peer_ip) :
    local_host(_localhost), local_interface_ip(_local_interface_ip), peer_ip(_peer_ip) {

    memset(&local_interface_in_addr,  0, sizeof(local_interface_in_addr));
    memset(&local_interface_in6_addr, 0, sizeof(local_interface_in6_addr));

    if (AddressConversion::isIpv4(local_interface_ip)) {
        local_interface_in_addr = AddressConversion::toInAddress(local_interface_ip);
        is_ipv4 = true;
        is_ipv6 = false;
    }
    else if (AddressConversion::isIpv6(local_interface_ip)) {
        local_interface_in6_addr = AddressConversion::toIn6Address(local_interface_ip);
        is_ipv4 = false;
        is_ipv6 = true;
    }
    else {
        logger.print(LogLevel::LOG_ERROR, "error invalid local interface ip address %s\n", local_interface_ip.c_str());
        is_ipv4 = false;
        is_ipv6 = false;
    }
    local_interface_prefix_length = local_host.getInterfacePrefixLength(local_interface_ip);
}


// ====================================================================================================

/**
 *  Speedwire packet sender class for multicast packets
 */
MulticastPacketSender::MulticastPacketSender(const LocalHost& localhost, const std::string& local_interface, const std::string& peer_ip) :
    SpeedwirePacketSender(localhost, local_interface, peer_ip) {
}

void MulticastPacketSender::send(SpeedwireHeader& packet, struct sockaddr& src) {
    bool forward = false;

    // if it is an IPv4 packet and the local interface is also IPv4
    if (src.sa_family == AF_INET && is_ipv4 == true) {
        // if the multicast packet was sent by a host on a different subnet
        forward = (AddressConversion::resideOnSameSubnet(AddressConversion::toSockAddrIn(src).sin_addr, local_interface_in_addr, local_interface_prefix_length) == false);
    }
    // if it is an IPv6 packet and the local interface is also IPv6
    else if (src.sa_family == AF_INET6 && is_ipv6 == true) {
        // if the multicast packet was sent by a host on a different subnet
        forward = (AddressConversion::resideOnSameSubnet(AddressConversion::toSockAddrIn6(src).sin6_addr, local_interface_in6_addr, local_interface_prefix_length) == false);
    }

    // forward the packet as a multicast packet
    if (forward == true) {
        //SpeedwireSocket socket = SpeedwireSocketFactory::getInstance(local_host)->getSendSocket(SpeedwireSocketFactory::MULTICAST, local_interface_ip);
        SpeedwireSocket socket = SpeedwireSocketFactory::getInstance(local_host)->getSendSocket(SpeedwireSocketFactory::UNICAST, local_interface_ip);
        //char loop = 0;
        //int result1 = setsockopt(socket.getSocketFd(), IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
        logger.print(LogLevel::LOG_INFO_1, "forward emeter packet to speedwire multicast address (via interface %s)\n", socket.getLocalInterfaceAddress().c_str());
        int nbytes = socket.send(packet.getPacketPointer(), packet.getPacketSize());
        if (nbytes != packet.getPacketSize()) {
            logger.print(LogLevel::LOG_ERROR, "error transmitting multicast packet to %s\n", local_interface_ip.c_str());
        }
    }
}


// ====================================================================================================

/**
 *  Speedwire packet sender class for multicast packets
 */
UnicastPacketSender::UnicastPacketSender(const LocalHost& localhost, const std::string& local_interface, const std::string& peer_ip) :
    SpeedwirePacketSender(localhost, local_interface, peer_ip) {
}

void UnicastPacketSender::send(SpeedwireHeader& packet, struct sockaddr& src) {

    // if it is an IPv4 packet
    if (src.sa_family == AF_INET) {

        // if the multicast/unicast packet was sent to a host on a different subnet
        struct sockaddr_in& src_in = AddressConversion::toSockAddrIn(src);
        struct in_addr peer = AddressConversion::toInAddress(peer_ip);

        if (AddressConversion::resideOnSameSubnet(src_in.sin_addr, peer, local_interface_prefix_length) == false) {
            // forward the packet as a unicast packet to the given unicast peer ip address
            SpeedwireSocket socket = SpeedwireSocketFactory::getInstance(local_host)->getSendSocket(SpeedwireSocketFactory::UNICAST, local_interface_ip);
            sockaddr_in sockaddr;
            sockaddr.sin_family = AF_INET;
            sockaddr.sin_addr = peer;
            sockaddr.sin_port = htons(9522);
            logger.print(LogLevel::LOG_INFO_1, "forward emeter packet to unicast host %s (via interface %s)\n", peer_ip.c_str(), socket.getLocalInterfaceAddress().c_str());
            int nbytes = socket.sendto(packet.getPacketPointer(), packet.getPacketSize(), sockaddr);
            if (nbytes != packet.getPacketSize()) {
                logger.print(LogLevel::LOG_ERROR, "error transmitting unicast packet to %s\n", local_interface_ip.c_str());
            }
        }
    }
    // if it is an IPv6 packet
    else if (src.sa_family == AF_INET6) {

        // if the multicast/unicast packet was sent to a host on a different subnet
        struct sockaddr_in6& src_in = AddressConversion::toSockAddrIn6(src);
        struct in6_addr peer = AddressConversion::toIn6Address(peer_ip);

        if (AddressConversion::resideOnSameSubnet(src_in.sin6_addr, peer, local_interface_prefix_length) == false) {
            // forward the packet as a unicast packet to the given unicast peer ip address
            SpeedwireSocket socket = SpeedwireSocketFactory::getInstance(local_host)->getSendSocket(SpeedwireSocketFactory::UNICAST, local_interface_ip);
            sockaddr_in6 sockaddr;
            sockaddr.sin6_family = AF_INET6;
            sockaddr.sin6_addr = peer;
            sockaddr.sin6_port = htons(9522);
            logger.print(LogLevel::LOG_INFO_1, "forward emeter packet to unicast host %s (via interface %s)\n", peer_ip.c_str(), socket.getLocalInterfaceAddress().c_str());
            int nbytes = socket.sendto(packet.getPacketPointer(), packet.getPacketSize(), sockaddr);
            if (nbytes != packet.getPacketSize()) {
                logger.print(LogLevel::LOG_ERROR, "error transmitting unicast packet to %s\n", local_interface_ip.c_str());
            }
        }
    }
}