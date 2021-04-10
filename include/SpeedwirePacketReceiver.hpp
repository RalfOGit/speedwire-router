#ifndef __SPEEDWIREPACKETRECEIVER_HPP__
#define __SPEEDWIREPACKETRECEIVER_HPP__

#include <LocalHost.hpp>
#include <SpeedwireHeader.hpp>
#include <SpeedwireEmeterProtocol.hpp>
#include <SpeedwireInverterProtocol.hpp>
#include <SpeedwirePacketSender.hpp>
#include <BounceDetector.hpp>
#include <PacketPatcher.hpp>
#include <ObisData.hpp>


/**
 *  Base class for speedwire packet receivers
 *  Each derived class is intended to receive speedwire packets belonging to a a single protocol as 
 *  defined by its protocolID setting.
 *  After processing the received speedwire packet all speedwire packet senders can be called to send
 *  the packet out to different networks and hosts
 */
class SpeedwirePacketReceiver {
public:
    typedef enum {
        IGNORED = 0,    // packet belongs to a different protocolID
        DROP    = 1,    // packet should be dropped
        FORWARD = 2     // packet should be forwarded
    } PacketStatus;

    uint16_t protocolID;
    BounceDetector bounceDetector;
    PacketPatcher  packetPatcher;

    SpeedwirePacketReceiver(LocalHost &host) : protocolID(0x0000), bounceDetector() {}
    virtual PacketStatus receive(SpeedwireHeader& packet, struct sockaddr& src) = 0;
};


/**
 *  Speedwire packet receiver class for sma emeter packets
 */
class EmeterPacketReceiver : public SpeedwirePacketReceiver {
public:
    EmeterPacketReceiver(LocalHost& host);
    virtual SpeedwirePacketReceiver::PacketStatus receive(SpeedwireHeader& packet, struct sockaddr& src);
};


/**
 *  Speedwire packet receiver class for sma inverter packets
 */
class InverterPacketReceiver : public SpeedwirePacketReceiver {
public:
    InverterPacketReceiver(LocalHost& host);
    virtual SpeedwirePacketReceiver::PacketStatus receive(SpeedwireHeader& packet, struct sockaddr& src);
};

#endif
