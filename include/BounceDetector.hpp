#ifndef __BOUNCEDETECTOR_HPP__
#define __BOUNCEDETECTOR_HPP__

#ifdef _WIN32
#include <Winsock2.h>
#include <ws2def.h>
#include <inaddr.h>
#include <in6addr.h>
#else
#include <netinet/in.h>
#include <net/if.h>
#endif
#include <array>
#include <SpeedwireHeader.hpp>
#include <SpeedwireEmeterProtocol.hpp>
#include <SpeedwireInverterProtocol.hpp>
#include <SpeedwirePacketSender.hpp>


/**
 *  Speedwire packet bounce detector
 *  Multicast packets may bounce indefinetely back and forth between subnets, if they are 
 *  routed. This class holds a limited history of previously received packets and checks
 *  if they were received shortly before
 */
class BounceDetector {
public:
    enum class PacketType : uint8_t {
        UNKNOWN = 0,
        EMETER = 1,
        INVERTER = 2,
        DISCOVERY_REQUEST = 3,
        DISCOVERY_RESPONSE = 4
    };

    class Fingerprint {
    public:
        // members present for all packet types
        struct sockaddr src_ip;         //!< source ip address
        PacketType      packet_type;    //!< packet type
        uint32_t        create_time;    //!< create time of this entry

        // members present for some packet types
        uint16_t        src_susyid;     //!< source susy id (emeter and inverter only)
        uint32_t        src_serial;     //!< source serial number (emeter and inverter only)
        uint32_t        src_timer;      //!< source timestamp (emeter only)
        uint16_t        src_packet_id;  //!< packet id (inverter only)
        struct in_addr  src_ip_addr;    //!< ip address (discovery response only)

        Fingerprint(void) :
            packet_type(PacketType::UNKNOWN), create_time(0), src_susyid(0), src_serial(0), src_timer(0), src_packet_id(0) {
            memset(&src_ip, 0, sizeof(src_ip));
            src_ip_addr.s_addr = 0;
        }
        Fingerprint(const struct sockaddr& srcip, const PacketType& packettype, uint32_t createtime) :
            src_ip(srcip), packet_type(packettype), create_time(createtime), src_susyid(0), src_serial(0), src_timer(0), src_packet_id(0) {
            src_ip_addr.s_addr = 0;
        }
    };

    typedef std::array<Fingerprint, 16> History;

protected:
    History history;
    int replace_index;

    bool setFingerprint(Fingerprint& fingerprint, const libspeedwire::SpeedwireEmeterProtocol&   packet, const struct sockaddr& src) const;
    bool setFingerprint(Fingerprint& fingerprint, const libspeedwire::SpeedwireInverterProtocol& packet, const struct sockaddr& src) const;
    bool setFingerprint(Fingerprint& fingerprint, const libspeedwire::SpeedwireHeader& speedwire_packet, const struct sockaddr& src) const;

public:
    BounceDetector(void);
    template<class T> void receive(const T& packet, const struct sockaddr& src);
    template<class T> bool isBouncedPacket(const T& packet, const struct sockaddr& src) const;
    const History& getHistory(void) const { return history; }
};

#endif
