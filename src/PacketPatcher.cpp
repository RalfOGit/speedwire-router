#include <PacketPatcher.hpp>


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
    temp.measurementValue->value = 3480;
    maxNegativeActivePowerTotalBytes = temp.toByteArray();
}

/**
 *  Patch the given speedwire packet in-place
 */
void PacketPatcher::patch(SpeedwireHeader& speedwire_packet, struct sockaddr& src) {
#if 1
    // example implementation for an emeter packet patch
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

        // loop across all obis data in the emeter packet
        void* obis = emeter_packet.getFirstObisElement();
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
                    emeter_packet.setObisElement(obis, maxNegativeActivePowerTotalBytes.data());
                }
            }

            // proceed to next obis element
            obis = emeter_packet.getNextObisElement(obis);
        }
    }
#endif
}
