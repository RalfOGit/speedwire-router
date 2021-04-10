#ifndef __BOUNCEDETECTOR_HPP__
#define __BOUNCEDETECTOR_HPP__

#include <AddressConversion.hpp>
#include <SpeedwireHeader.hpp>
#include <SpeedwireEmeterProtocol.hpp>
#include <SpeedwireInverterProtocol.hpp>
#include <SpeedwirePacketSender.hpp>
#include <ObisData.hpp>


/**
 *  Speedwire packet bounce detector
 *  Multicast packets may bounce indefinetely back and forth between subnets, if they are 
 *  routed. This class holds a limited history of previously received packets and checks
 *  if they were received shortly before
 */
class BounceDetector {
protected:
    typedef struct {
        uint16_t src_susyid;    // source susy id
        uint32_t src_serial;    // source serial number
        uint32_t timer;         // creation timestamp (emeter only)
        uint16_t packet_id;     // packet id (inverter only
    } Fingerprint;

    std::array<Fingerprint, 8> history;
    int replace_index;

    bool insertFingerprint(Fingerprint& fingerprint, SpeedwireEmeterProtocol&   packet, struct sockaddr& src);
    bool insertFingerprint(Fingerprint& fingerprint, SpeedwireInverterProtocol& packet, struct sockaddr& src);

public:
    BounceDetector(void);
    template<class T> void receive(T& packet, struct sockaddr& src);
    template<class T> bool isBouncedPacket(T& packet, struct sockaddr& src);
};

#endif
