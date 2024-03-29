#include <AddressConversion.hpp>
#include <LocalHost.hpp>
#include <SpeedwireDiscoveryProtocol.hpp>
#include <SpeedwireEncryptionProtocol.hpp>
#include <SpeedwireReceiveDispatcher.hpp>
#include <SpeedwirePacketReceiver.hpp>
#include <SpeedwireSocketFactory.hpp>
#include <ObisData.hpp>
#include <Logger.hpp>
using namespace libspeedwire;

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
    protocolID = SpeedwireData2Packet::sma_emeter_protocol_id;
}

/**
 *  Receive method - can be called with arbitrary speedwire packets
 */
void EmeterPacketReceiver::receive(SpeedwireHeader& speedwire_packet, struct sockaddr& src) {

    // check if it is a valid data2 packet
    if (speedwire_packet.isValidData2Packet()) {
        const SpeedwireData2Packet data2_packet(speedwire_packet);
        uint16_t length     = data2_packet.getTagLength();
        uint16_t protocolID = data2_packet.getProtocolID();
        int      offset     = data2_packet.getPayloadOffset();

        // check if it is an emeter packet
        if (SpeedwireData2Packet::isEmeterProtocolID(protocolID) == true || SpeedwireData2Packet::isExtendedEmeterProtocolID(protocolID)) {
            const SpeedwireEmeterProtocol emeter_packet(data2_packet);

            uint16_t susyid = emeter_packet.getSusyID();
            uint32_t serial = emeter_packet.getSerialNumber();
            uint32_t timer  = emeter_packet.getTime();

            // perform some simple multicast bounce back prevention
            if (bounceDetector.isBouncedPacket(emeter_packet, src) == true) {
                logger.print(LogLevel::LOG_INFO_1, "received bounced emeter packet from %s susyid %u serial %lu time %lu => DROPPED\n", AddressConversion::toString(src).c_str(), susyid, serial, timer);
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
}


/**
 *  Constructor
 */
InverterPacketReceiver::InverterPacketReceiver(LocalHost& host, std::vector<SpeedwirePacketSender*>& sender)
  : InverterPacketReceiverBase(host),
    localHost(host),
    senders(sender),
    bounceDetector(),
    packetPatcher() {
    protocolID = SpeedwireData2Packet::sma_inverter_protocol_id;
}

/**
 *  Receive method - can be called with arbitrary speedwire packets
 */
void InverterPacketReceiver::receive(SpeedwireHeader& speedwire_packet, struct sockaddr& src) {

    // check if it is a valid data2 packet
    if (speedwire_packet.isValidData2Packet()) {
        const SpeedwireData2Packet data2_packet(speedwire_packet);
        uint16_t length = data2_packet.getTagLength();
        uint16_t protocolID = data2_packet.getProtocolID();
        int      offset = data2_packet.getPayloadOffset();

        // check if it is an inverter packet
        if (data2_packet.isInverterProtocolID() == true) {
            const SpeedwireInverterProtocol inverter_packet(data2_packet);

            uint16_t susyid = inverter_packet.getSrcSusyID();
            uint32_t serial = inverter_packet.getSrcSerialNumber();
            uint32_t timer  = (uint32_t)LocalHost::getUnixEpochTimeInMs();

            // perform some simple multicast bounce back prevention
            if (bounceDetector.isBouncedPacket(inverter_packet, src) == true) {
                logger.print(LogLevel::LOG_INFO_1, "received bounced inverter packet from %s susyid %u serial %lu time %lu => DROPPED\n", AddressConversion::toString(src).c_str(), susyid, serial, timer);
                return;
            }
            bounceDetector.receive(inverter_packet, src);
            std::string reqresp = (((uint32_t)inverter_packet.getCommandID() & 0xff) == 0x00 ? "request" : "response");
            logger.print(LogLevel::LOG_INFO_1, "received inverter %s packet from %s susyid %u serial %lu time %lu\n", reqresp.c_str(), AddressConversion::toString(src).c_str(), susyid, serial, timer);
            std::string istr = inverter_packet.toString();
            logger.print(LogLevel::LOG_INFO_1, "%s\n", istr.c_str());

#if 0
            // check if it is a broadcast request packet from a node on a different subnet
            if (inverter_packet.getDstSusyID() == 0xffff && inverter_packet.getDstSerialNumber() == 0xffffffff && (inverter_packet.getCommandID() & 0xff) == 0x00) {
                struct in_addr src_addr; src_addr.s_addr = AddressConversion::toSockAddrIn(src).sin_addr.s_addr;
                bool is_sender_on_local_subnet = false;
                for (const auto& local_ip : localHost.getLocalIPv4Addresses()) {
                    if (AddressConversion::resideOnSameSubnet(src_addr, AddressConversion::toInAddress(local_ip), localHost.getInterfacePrefixLength(local_ip))) {
                        is_sender_on_local_subnet = true;
                    }
                }
                if (is_sender_on_local_subnet == false) {
                    const std::vector<std::string>& localIPs = localHost.getLocalIPv4Addresses();
                    for (const auto& if_addr : localIPs) {
                        SpeedwireSocket socket = SpeedwireSocketFactory::getInstance(localHost)->getSendSocket(SpeedwireSocketFactory::SocketType::MULTICAST, if_addr);
                        fprintf(stdout, "send broadcast request to %s (via interface %s)\n", AddressConversion::toString(socket.getSpeedwireMulticastIn4Address()).c_str(), socket.getLocalInterfaceAddress().c_str());
                        int nbytes = socket.sendto(speedwire_packet.getPacketPointer(), (unsigned long)speedwire_packet.getPacketSize(), socket.getSpeedwireMulticastIn4Address(), AddressConversion::toInAddress(if_addr));
                    }
                }
            }
#endif
            // patch packet if required
            packetPatcher.patch(speedwire_packet, src);

            // forward the emeter packet to all registered producers
            for (auto& sender : senders) {
                sender->send(speedwire_packet, src);
            }
        }
        // check if it is an encryption packet
        else if (data2_packet.isEncryptionProtocolID() == true) {
            const SpeedwireEncryptionProtocol encryption_packet(data2_packet);

            uint8_t  type   = encryption_packet.getPacketType();
            uint16_t susyid = encryption_packet.getSrcSusyID();
            uint32_t serial = encryption_packet.getSrcSerialNumber();
            uint32_t timer  = (uint32_t)LocalHost::getUnixEpochTimeInMs();

            // perform some simple multicast bounce back prevention
            if (bounceDetector.isBouncedPacket(encryption_packet, src) == true) {
                logger.print(LogLevel::LOG_INFO_1, "received bounced encryption packet from %s susyid %u serial %lu time %lu => DROPPED\n", AddressConversion::toString(src).c_str(), susyid, serial, timer);
                return;
            }
            bounceDetector.receive(encryption_packet, src);
            std::string reqresp = (type == 0x01 ? "request" : (type == 0x02 ? "response" : "unknown"));
            logger.print(LogLevel::LOG_INFO_1, "received encryption %s packet from %s susyid %u serial %lu time %lu\n", reqresp.c_str(), AddressConversion::toString(src).c_str(), susyid, serial, timer);
            std::string istr = encryption_packet.toString();
            logger.print(LogLevel::LOG_INFO_1, "%s\n", istr.c_str());

            // patch packet if required
            packetPatcher.patch(speedwire_packet, src);

            // forward the emeter packet to all registered producers
            for (auto& sender : senders) {
                sender->send(speedwire_packet, src);
            }
#if 0
            if (type == 0x01) {
                // respond with a fake encryption response packet
                uint8_t buffer[1024];
                SpeedwireHeader packet(buffer, (unsigned long)sizeof(buffer));
                packet.setDefaultHeader(0x00000001, 0x70, 0x6075);
                SpeedwireData2Packet data2(packet);
                SpeedwireEncryptionProtocol response(data2);

                response.setPacketType(0x02);
                response.setSrcSusyID(378);
                response.setSrcSerialNumber(3009850131);
                response.setDstSusyID(encryption_packet.getSrcSusyID());
                response.setDstSerialNumber(encryption_packet.getSrcSerialNumber());

                std::array<uint8_t, 16> src_seed = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
                response.setDataUint8Array16(0, src_seed);
                response.setDataUint8Array16(16, encryption_packet.getDataUint8Array16(0));

                response.setDataUint8(32, 0x01);        // 0x00 ungesichert, 0x01 gesichert, 0x02 unbekannt
                std::string str1 = "AXTYFV"; // "AXTYFV"; => Hohe Sicherheit mit RID; "2ATKKZYYYS6J6QHR" => Basissicherheit mit WPA-PSK
                std::string str2 = "003783009850131";
                std::string str3 = "2ATKKZYYYS6J6QHR";
                response.setString16(33, str3);
                response.setString16(49, str2);

                std::array<uint8_t, 16> one_seed = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
                std::array<uint8_t, 16> two_seed = { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
                response.setDataUint8Array16(65, one_seed);
                response.setDataUint8Array16(81, two_seed);
                std::string result = response.toString();
                logger.print(LogLevel::LOG_INFO_1, "assembled: %s\n", result.c_str());
                LocalHost::hexdump(buffer, 140);

                SpeedwireSocket &socket = SpeedwireSocketFactory::getInstance(localHost)->getSendSocket(SpeedwireSocketFactory::SocketType::UNICAST, "192.168.178.20");
                socket.sendto(packet.getPacketPointer(), packet.getPacketSize(), src);
            }
#endif
        }
    }
}


/**
 *  Constructor
 */
DiscoveryPacketReceiver::DiscoveryPacketReceiver(LocalHost& host, std::vector<SpeedwirePacketSender*>& sender)
    : DiscoveryPacketReceiverBase(host),
    localHost(host),
    senders(sender),
    bounceDetector(),
    packetPatcher() {
    protocolID = 0x0000;
}

/**
 *  Receive method - can be called with arbitrary speedwire packets
 */
void DiscoveryPacketReceiver::receive(SpeedwireHeader& speedwire_packet, struct sockaddr& src) {

    // check if it is a valid discovery packet
    if (speedwire_packet.isValidDiscoveryPacket()) {

        // check if it is a discovery request or a discovery response
        SpeedwireDiscoveryProtocol discovery_packet(speedwire_packet);
        bool is_discovery_request  = discovery_packet.isMulticastRequestPacket();
        bool is_discovery_response = discovery_packet.isMulticastResponsePacket();
        std::string reqresp = (is_discovery_request ? "request" : (is_discovery_response ? "response" : ""));

        // perform some simple multicast bounce back prevention
        uint32_t timer = (uint32_t)LocalHost::getUnixEpochTimeInMs();
        if (bounceDetector.isBouncedPacket(speedwire_packet, src) == true) {
            logger.print(LogLevel::LOG_INFO_1, "received bounced discovery %s packet from %s time %lu => DROPPED\n", reqresp.c_str(), AddressConversion::toString(src).c_str(), timer);
            return;
        }
        bounceDetector.receive(speedwire_packet, src);
        logger.print(LogLevel::LOG_INFO_1, "received discovery %s packet from %s time %lu\n", reqresp.c_str(), AddressConversion::toString(src).c_str(), timer);

        // forward the discovery request packet to all registered producers
        if (is_discovery_request) {
            for (auto& sender : senders) {
                sender->send(speedwire_packet, src);
            }
        }

        // for discovery responses, try to find the requester
        if (is_discovery_response) {
            const BounceDetector::History& history = bounceDetector.getHistory();
            for (const auto& entry : history) {
                if (entry.packet_type == BounceDetector::PacketType::DISCOVERY_REQUEST &&
                    SpeedwireTime::calculateAbsTimeDifference(entry.create_time, (uint32_t)LocalHost::getUnixEpochTimeInMs()) <= 1000) {
                    struct in_addr entry_addr = AddressConversion::toSockAddrIn(entry.src_ip).sin_addr;

                    // try to find the local interface to reach the requester
                    const std::vector<std::string> &if_addresses = localHost.getLocalIPv4Addresses();
                    for (const auto& local_interface_ip : if_addresses) {
                        uint32_t prefix = localHost.getInterfacePrefixLength(local_interface_ip);
                        if (AddressConversion::resideOnSameSubnet(entry_addr, AddressConversion::toInAddress(local_interface_ip), prefix)) {

                            // forward the discovery response as a unicast packet to the given unicast peer ip address
                            SpeedwireSocket socket = SpeedwireSocketFactory::getInstance(localHost)->getSendSocket(SpeedwireSocketFactory::SocketType::UNICAST, local_interface_ip);
                            logger.print(LogLevel::LOG_INFO_1, "forward discovery response packet to unicast host %s (via interface %s)\n",
                                AddressConversion::toString(entry.src_ip).c_str(), socket.getLocalInterfaceAddress().c_str());
                            int nbytes = socket.sendto(speedwire_packet.getPacketPointer(), speedwire_packet.getPacketSize(), entry.src_ip);
                            if (nbytes != speedwire_packet.getPacketSize()) {
                                logger.print(LogLevel::LOG_ERROR, "error transmitting unicast packet to %s\n", local_interface_ip.c_str());
                            }
                        }
                    }
                }
            }
        }
    }
}
