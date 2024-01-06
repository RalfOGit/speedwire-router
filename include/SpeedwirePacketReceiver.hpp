#ifndef __SPEEDWIREPACKETRECEIVER_HPP__
#define __SPEEDWIREPACKETRECEIVER_HPP__

#include <LocalHost.hpp>
#include <SpeedwireHeader.hpp>
#include <SpeedwireReceiveDispatcher.hpp>
#include <SpeedwirePacketSender.hpp>
#include <BounceDetector.hpp>
#include <PacketPatcher.hpp>


/**
 *  Derived classes for speedwire packet receivers
 *  Each derived class is intended to receive speedwire packets belonging to a a single protocol as 
 *  defined by its protocolID setting.
 */

/**
 *  Speedwire packet receiver class for sma emeter packets
 */
class EmeterPacketReceiver : public libspeedwire::EmeterPacketReceiverBase {
protected:
    std::vector<SpeedwirePacketSender*>& senders;
    BounceDetector bounceDetector;
    PacketPatcher  packetPatcher;

public:
    EmeterPacketReceiver(libspeedwire::LocalHost& host, std::vector<SpeedwirePacketSender*>& senders);
    virtual void receive(libspeedwire::SpeedwireHeader& packet, struct sockaddr& src);
};


/**
 *  Speedwire packet receiver class for sma inverter packets
 */
class InverterPacketReceiver : public libspeedwire::InverterPacketReceiverBase {
protected:
    libspeedwire::LocalHost& localHost;
    std::vector<SpeedwirePacketSender*>& senders;
    BounceDetector bounceDetector;
    PacketPatcher  packetPatcher;

public:
    InverterPacketReceiver(libspeedwire::LocalHost& host, std::vector<SpeedwirePacketSender*>& senders);
    virtual void receive(libspeedwire::SpeedwireHeader& packet, struct sockaddr& src);
};


/**
 *  Speedwire packet receiver class for sma discovery packets
 */
class DiscoveryPacketReceiver : public libspeedwire::DiscoveryPacketReceiverBase {
protected:
    libspeedwire::LocalHost &localHost;
    std::vector<SpeedwirePacketSender*>& senders;
    BounceDetector bounceDetector;
    PacketPatcher  packetPatcher;

public:
    DiscoveryPacketReceiver(libspeedwire::LocalHost& host, std::vector<SpeedwirePacketSender*>& senders);
    virtual void receive(libspeedwire::SpeedwireHeader& packet, struct sockaddr& src);
};

#endif
