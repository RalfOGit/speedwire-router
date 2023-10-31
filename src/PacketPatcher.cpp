#include <PacketPatcher.hpp>
using namespace libspeedwire;


/**
 *  Speedwire packet patcher base class
 *  In some cases it is useful to change the content of a speedwire packet before forwarding it to another network or host
 */

/**
 *  Constructor
 */
PacketPatcher::PacketPatcher(void) {
    // define an obis data element with the given negative active power total value and convert it to its byte representation
    ObisData temp = ObisData::NegativeActivePowerTotal;
    temp.measurementValues.addMeasurement(3480, 0);
    maxNegativeActivePowerTotalBytes = temp.toByteArray();
}

/**
 *  Patch the given speedwire packet in-place
 */
void PacketPatcher::patch(SpeedwireHeader& speedwire_packet, struct sockaddr& src) {
#if 1
    // example implementation for an emeter packet patch

    // check if it is a valid data2 packet
    if (speedwire_packet.isValidData2Packet()) {
        SpeedwireData2Packet data2_packet(speedwire_packet);
        uint16_t length     = data2_packet.getTagLength();
        uint16_t protocolID = data2_packet.getProtocolID();
        int      offset     = data2_packet.getPayloadOffset();

        // check if it is an emeter packet
        if (SpeedwireData2Packet::isEmeterProtocolID(protocolID) == true || SpeedwireData2Packet::isExtendedEmeterProtocolID(protocolID)) {
            SpeedwireEmeterProtocol emeter_packet(data2_packet);

            uint16_t susyid = emeter_packet.getSusyID();
            uint32_t serial = emeter_packet.getSerialNumber();
            uint32_t timer  = emeter_packet.getTime();

            // loop across all obis data in the emeter packet
            const void* obis = emeter_packet.getFirstObisElement();
            while (obis != NULL) {
                //emeter_packet.printObisElement(obis, stderr);
                const uint8_t  channel = emeter_packet.getObisChannel(obis);
                const uint8_t  index   = emeter_packet.getObisIndex(obis);
                const uint8_t  type    = emeter_packet.getObisType(obis);
                const uint8_t  tariff  = emeter_packet.getObisTariff(obis);
                const uint32_t value   = emeter_packet.getObisValue4(obis);

                // limit the negative active total power to some given value
                const ObisData& negActPowTot = ObisData::NegativeActivePowerTotal;
                if (channel == negActPowTot.channel && index == negActPowTot.index && type == negActPowTot.type && tariff == negActPowTot.tariff) {
                    uint32_t max_value = emeter_packet.getObisValue4(maxNegativeActivePowerTotalBytes.data());
                    if (value > max_value) {
                        fprintf(stdout, "limited max negative active power from %ul to %ul\n", value, max_value);
                        emeter_packet.setObisElement((void*)obis, maxNegativeActivePowerTotalBytes.data());
                    }
                }

                // proceed to next obis element
                obis = emeter_packet.getNextObisElement(obis);
            }
        }
    }
#endif
}
