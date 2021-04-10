#include <BounceDetector.hpp>

/**
 *  Speedwire packet bounce detector
 *  Multicast packets may bounce indefinetely back and forth between subnets, if they are
 *  routed. This class holds a limited history of previously received packets and checks
 *  if they were received shortly before.
 */

/**
 *  Constructor
 */
BounceDetector::BounceDetector(void) {
    Fingerprint initial;
    initial.src_susyid = 0;
    initial.src_serial = 0;
    initial.timer      = 0;
    initial.packet_id  = 0;
    history.fill(initial);

    replace_index = 0;
}

/**
 *  Received packets are added to the history table, while replacing the oldest entry
 */
template<class T> void BounceDetector::receive(T& speedwire_packet, struct sockaddr& src) {
    // insert the fingerprint of the given speedwire packet at the replace_index position in the history
    if (insertFingerprint(history[replace_index], speedwire_packet, src) == true) {
        if (++replace_index >= history.size()) {
            replace_index = 0;
        }
    }
}

/**
 *  Check if the given packets fingerprint can be found in the history table
 */
template<class T> bool BounceDetector::isBouncedPacket(T& speedwire_packet, struct sockaddr& src) {
    Fingerprint packet;
    if (insertFingerprint(packet, speedwire_packet, src) == true) {
        for (const auto& entry : history) {
            if (entry.src_susyid == packet.src_susyid && entry.src_serial == packet.src_serial && 
                entry.timer      == packet.timer      && entry.packet_id  == packet.packet_id) {
                return true;
            }
        }
    }
    return false;
}

/**
 *  Insert emeter fingerprint at the given history tableposition
 *  The fingerprint is derived from the source susyid, serial and timer values
 */
bool BounceDetector::insertFingerprint(Fingerprint& fingerprint, SpeedwireEmeterProtocol& emeter_packet, struct sockaddr& src) {
    // the fingerprint for emeter packets is defined by susyid, serialnumber and creation time
    fingerprint.src_susyid = emeter_packet.getSusyID();
    fingerprint.src_serial = emeter_packet.getSerialNumber();
    fingerprint.timer      = emeter_packet.getTime();
    fingerprint.packet_id  = 0;
    return true;
}

/**
 *  Insert inverter fingerprint at the given history table position
 *  The fingerprint is derived from the source susyid, serial and packetid values
 */
bool BounceDetector::insertFingerprint(Fingerprint& fingerprint, SpeedwireInverterProtocol& inverter_packet, struct sockaddr& src) {
    fingerprint.src_susyid = inverter_packet.getSrcSusyID();
    fingerprint.src_serial = inverter_packet.getSrcSerialNumber();
    fingerprint.timer      = 0;
    fingerprint.packet_id  = inverter_packet.getPacketID();
    return true;
}

// explicit template instantiations
template void BounceDetector::receive(SpeedwireEmeterProtocol& packet, struct sockaddr& src);
template void BounceDetector::receive(SpeedwireInverterProtocol& packet, struct sockaddr& src);
template bool BounceDetector::isBouncedPacket(SpeedwireEmeterProtocol& packet, struct sockaddr& src);
template bool BounceDetector::isBouncedPacket(SpeedwireInverterProtocol& packet, struct sockaddr& src);
