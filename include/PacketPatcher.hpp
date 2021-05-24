#ifndef __PACKETPATCHER_HPP__
#define __PACKETPATCHER_HPP__

#include <AddressConversion.hpp>
#include <SpeedwireHeader.hpp>
#include <SpeedwireEmeterProtocol.hpp>
#include <SpeedwireInverterProtocol.hpp>
#include <SpeedwirePacketSender.hpp>
#include <ObisData.hpp>


/**
 *  Speedwire packet patcher class
 *  In some cases it is useful to change the content of a speedwire packet before forwarding it to another network or host 
 * 
 */
class PacketPatcher {
protected:
    std::array<uint8_t, 12> maxNegativeActivePowerTotalBytes;

public:
    PacketPatcher(void);
    virtual void patch(libspeedwire::SpeedwireHeader& packet, struct sockaddr& src);
};

#endif

