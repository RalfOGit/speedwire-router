#include <SpeedwireDiscoveryProtocol.hpp>
#include <BounceDetector.hpp>
using namespace libspeedwire;

/**
 *  Speedwire packet bounce detector
 *  Multicast packets may bounce indefinetely back and forth between subnets, if they are
 *  routed. This class holds a limited history of previously received packets and checks
 *  if they were received shortly before.
 */

/**
 *  Constructor
 */
BounceDetector::BounceDetector(void) : history() {
    replace_index = 0;
}

/**
 *  Received packets are added to the history table, while replacing the oldest entry
 */
template<class T> void BounceDetector::receive(const T& speedwire_packet, const struct sockaddr& src) {
    // insert the fingerprint of the given speedwire packet at the replace_index position in the history
    if (setFingerprint(history[replace_index], speedwire_packet, src) == true) {
        if (++replace_index >= history.size()) {
            replace_index = 0;
        }
    }
}

/**
 *  Check if the given packets fingerprint can be found in the history table
 */
template<class T> bool BounceDetector::isBouncedPacket(const T& speedwire_packet, const struct sockaddr& src) const {
    Fingerprint packet;
    if (setFingerprint(packet, speedwire_packet, src) == true) {
        for (const auto& entry : history) {
            if (entry.packet_type == packet.packet_type) {
                switch (entry.packet_type) {
                case PacketType::EMETER:
                    if (entry.src_susyid == packet.src_susyid && entry.src_serial == packet.src_serial && entry.src_timer == packet.src_timer) {
                        return true;
                    }
                    break;
                case PacketType::INVERTER:
                    if (entry.src_susyid == packet.src_susyid && entry.src_serial == packet.src_serial && entry.src_packet_id == packet.src_packet_id) {
                        return true;
                    }
                    break;
                case PacketType::DISCOVERY_REQUEST:
                    if (SpeedwireTime::calculateAbsTimeDifference(entry.create_time, (uint32_t)LocalHost::getUnixEpochTimeInMs()) <= 1000) {
                        return true;
                    }
                    break;
                case PacketType::DISCOVERY_RESPONSE:
                    if (entry.src_ip_addr.s_addr == packet.src_ip_addr.s_addr &&
                        SpeedwireTime::calculateAbsTimeDifference(entry.create_time, (uint32_t)LocalHost::getUnixEpochTimeInMs()) <= 1000) {
                        return true;
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }
    return false;
}

/**
 *  Insert emeter fingerprint at the given history tableposition
 *  The fingerprint is derived from the source susyid, serial and timer values
 */
bool BounceDetector::setFingerprint(Fingerprint& fingerprint, const SpeedwireEmeterProtocol& emeter_packet, const struct sockaddr& src) const {
    // the fingerprint for emeter packets is defined by susyid, serialnumber and packet time
    fingerprint = Fingerprint(src, PacketType::EMETER, (uint32_t)LocalHost::getUnixEpochTimeInMs());
    fingerprint.src_susyid  = emeter_packet.getSusyID();
    fingerprint.src_serial  = emeter_packet.getSerialNumber();
    fingerprint.src_timer   = emeter_packet.getTime();
    return true;
}

/**
 *  Insert inverter fingerprint at the given history table position
 *  The fingerprint is derived from the source susyid, serial and packetid values
 */
bool BounceDetector::setFingerprint(Fingerprint& fingerprint, const SpeedwireInverterProtocol& inverter_packet, const struct sockaddr& src) const {
    // the fingerprint for emeter packets is defined by susyid, serialnumber and packet id
    fingerprint = Fingerprint(src, PacketType::INVERTER, (uint32_t)LocalHost::getUnixEpochTimeInMs());
    fingerprint.src_susyid    = inverter_packet.getSrcSusyID();
    fingerprint.src_serial    = inverter_packet.getSrcSerialNumber();
    fingerprint.src_packet_id = inverter_packet.getPacketID();
    return true;
}

/**
 *  Insert discovery fingerprint at the given history table position
 *  The fingerprint is derived from the 
 */
bool BounceDetector::setFingerprint(Fingerprint& fingerprint, const SpeedwireHeader& speedwire_packet, const struct sockaddr& src) const {
    // the fingerprint for discovery packets is defined by src ip, src packet type and src ip address
    fingerprint = Fingerprint(src, PacketType::UNKNOWN, (uint32_t)LocalHost::getUnixEpochTimeInMs());

    // check if it is a discovery packet (either request or response)
    if (speedwire_packet.isValidDiscoveryPacket()) {
        SpeedwireDiscoveryProtocol discovery_packet(speedwire_packet);

        if (discovery_packet.isMulticastRequestPacket()) {
            fingerprint.packet_type = PacketType::DISCOVERY_REQUEST;
        }
        else if (discovery_packet.isMulticastResponsePacket()) {
            fingerprint.packet_type = PacketType::DISCOVERY_RESPONSE;
            fingerprint.src_ip_addr.s_addr = discovery_packet.getIPv4Address();
        }
        return true;
    }
    return false;
}


// explicit template instantiations
template void BounceDetector::receive(const SpeedwireEmeterProtocol& packet, const struct sockaddr& src);
template void BounceDetector::receive(const SpeedwireInverterProtocol& packet, const struct sockaddr& src);
template void BounceDetector::receive(const SpeedwireHeader& packet, const struct sockaddr& src);
template bool BounceDetector::isBouncedPacket(const SpeedwireEmeterProtocol& packet, const struct sockaddr& src) const;
template bool BounceDetector::isBouncedPacket(const SpeedwireInverterProtocol& packet, const struct sockaddr& src) const;
template bool BounceDetector::isBouncedPacket(const SpeedwireHeader& packet, const struct sockaddr& src) const;
