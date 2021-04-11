#include <LocalHost.hpp>
#include <AddressConversion.hpp>
#include <SpeedwireReceiveDispatcher.hpp>
#include <SpeedwirePacketReceiver.hpp>
#include <ObisData.hpp>
#include <Logger.hpp>

static Logger logger = Logger("EmeterPacketReceiver");


/**
 *  Speedwire packet receiver class for sma emeter packets
 */

/**
 *  Constructor, std::vector<SpeedwirePacketSender&> &senders, BounceDetector &bounceDetector, PacketPatcher &packetPatcher);
 */
EmeterPacketReceiver::EmeterPacketReceiver(LocalHost& host, std::vector<SpeedwirePacketSender*>& sender) 
  : EmeterPacketReceiverBase(host),
    senders(sender),
    bounceDetector(),
    packetPatcher() {
    protocolID = SpeedwireHeader::sma_emeter_protocol_id;
}

/**
 *  Receive method - can be called with arbitrary speedwire packets
 */
void EmeterPacketReceiver::receive(SpeedwireHeader& speedwire_packet, struct sockaddr& src) {
    uint32_t group      = speedwire_packet.getGroup();
    uint16_t length     = speedwire_packet.getLength();
    uint16_t protocolID = speedwire_packet.getProtocolID();
    int      offset     = speedwire_packet.getPayloadOffset();

    // check if it is an emeter packet
    if (speedwire_packet.isEmeterProtocolID() == true) {
        SpeedwireEmeterProtocol emeter_packet(speedwire_packet);
        uint16_t susyid = emeter_packet.getSusyID();
        uint32_t serial = emeter_packet.getSerialNumber();
        uint32_t timer  = emeter_packet.getTime();

        // perform some simple multicast bounce back prevention
        if (bounceDetector.isBouncedPacket(emeter_packet, src) == true) {
            logger.print(LogLevel::LOG_INFO_1, "received bounced emeter packet from %s susyid %u serial %lu time %lu\n", AddressConversion::toString(src).c_str(), susyid, serial, timer);
            return;
        }
        bounceDetector.receive(emeter_packet, src);
        logger.print(LogLevel::LOG_INFO_1, "received emeter packet from %s susyid %u serial %lu time %lu\n", AddressConversion::toString(src).c_str(), susyid, serial, timer);

        // patch packet if required
        packetPatcher.patch(speedwire_packet, src);

        // loop across obis data in the emeter packet
        //for (void* obis = emeter_packet.getFirstObisElement(); obis != NULL; obis = emeter_packet.getNextObisElement(obis)) {
        //    obis = emeter_packet.getNextObisElement(obis);
        //}

        // forward the emeter packet to all registered producers
        for (auto& sender : senders) {
            sender->send(speedwire_packet, src);
        }
    }
}


/**
 *  Constructor
 */
InverterPacketReceiver::InverterPacketReceiver(LocalHost& host, std::vector<SpeedwirePacketSender*> sender)
  : InverterPacketReceiverBase(host),
    senders(sender),
    bounceDetector(),
    packetPatcher() {
    protocolID = SpeedwireHeader::sma_inverter_protocol_id; 
}

/**
 *  Receive method - can be called with arbitrary speedwire packets
 */
void InverterPacketReceiver::receive(SpeedwireHeader& speedwire_packet, struct sockaddr& src) {
    uint32_t group      = speedwire_packet.getGroup();
    uint16_t length     = speedwire_packet.getLength();
    uint16_t protocolID = speedwire_packet.getProtocolID();
    int      offset     = speedwire_packet.getPayloadOffset();

    if (speedwire_packet.isInverterProtocolID() == true) {
        SpeedwireInverterProtocol inverter_packet(speedwire_packet);
        uint16_t susyid = inverter_packet.getSrcSusyID();
        uint32_t serial = inverter_packet.getSrcSerialNumber();
        uint32_t timer  = (uint32_t)LocalHost::getUnixEpochTimeInMs();

        // perform some simple multicast bounce back prevention
        if (bounceDetector.isBouncedPacket(inverter_packet, src) == true) {
            logger.print(LogLevel::LOG_INFO_1, "received bounced inverter packet from %s susyid %u serial %lu time %lu => DROPPED\n", AddressConversion::toString(src).c_str(), susyid, serial, timer);
            return;
        }
        bounceDetector.receive(inverter_packet, src);
        logger.print(LogLevel::LOG_INFO_1, "received inverter packet from %s susyid %u serial %lu time %lu => DROPPED\n", AddressConversion::toString(src).c_str(), susyid, serial, timer);

        // patch packet if required
        packetPatcher.patch(speedwire_packet, src);

        // forward the emeter packet to all registered producers
        for (auto& sender : senders) {
            sender->send(speedwire_packet, src);
        }
    }
}
