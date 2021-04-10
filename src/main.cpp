#ifdef _WIN32
#include <Winsock2.h>
#include <Ws2tcpip.h>
#define poll(a, b, c)  WSAPoll((a), (b), (c))
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#endif

#include <LocalHost.hpp>
#include <AddressConversion.hpp>
#include <Logger.hpp>
#include <Measurement.hpp>
#include <SpeedwireSocketFactory.hpp>
#include <SpeedwireSocketSimple.hpp>
#include <SpeedwireByteEncoding.hpp>
#include <SpeedwireHeader.hpp>
#include <SpeedwireEmeterProtocol.hpp>
#include <SpeedwireDiscovery.hpp>
#include <SpeedwirePacketReceiver.hpp>
#include <SpeedwirePacketSender.hpp>
#include <ObisData.hpp>
#include <ObisFilter.hpp>
#include <DataProcessor.hpp>


static int poll_sockets(const std::vector<SpeedwireSocket>& sockets, struct pollfd* const fds, const int poll_timeout_in_ms, std::vector<SpeedwirePacketReceiver*> &packet_receivers, std::vector<SpeedwirePacketSender*>& packet_senders);

class LogListener : public ILogListener {
public:
    virtual ~LogListener() {}

    virtual void log_msg(const std::string& msg, const LogLevel &level) {
        fprintf(stdout, "%s", msg.c_str());
    }

    virtual void log_msg_w(const std::wstring& msg, const LogLevel &level) {
        fprintf(stdout, "%ls", msg.c_str());
    }
};

static Logger logger("main");


int main(int argc, char **argv) {

    // configure logger and logging levels
    ILogListener *log_listener = new LogListener();
    LogLevel log_level = LogLevel::LOG_ERROR | LogLevel::LOG_WARNING;
    log_level = log_level | LogLevel::LOG_INFO_0;
    log_level = log_level | LogLevel::LOG_INFO_1;
    log_level = log_level | LogLevel::LOG_INFO_2;
    log_level = log_level | LogLevel::LOG_INFO_3;
    Logger::setLogListener(log_listener, log_level);

    // discover sma devices on the local network
    LocalHost localhost;
    SpeedwireDiscovery discoverer(localhost);
    discoverer.preRegisterDevice("192.168.182.18");
    discoverer.discoverDevices();
    std::vector<SpeedwireInfo> devices = discoverer.getDevices();

    //// define measurement filters for sma emeter packet filtering
    //ObisFilter filter;
    //filter.addFilter(ObisData::getAllPredefined());

    //// configure processing chain
    //ObisPrintoutConsumer obis_printer;
    //filter.addConsumer(&obis_printer);

    // open socket(s) to receive sma emeter packets from any local interface
    SpeedwireSocketFactory *socket_factory = SpeedwireSocketFactory::getInstance(localhost);
    const std::vector<SpeedwireSocket> recv_sockets = socket_factory->getRecvSockets(SpeedwireSocketFactory::ANYCAST, localhost.getLocalIPv4Addresses());

    // allocate pollfd struct(s) for the socket(s)
    struct pollfd *const fds = (struct pollfd *const) malloc(sizeof(struct pollfd) * recv_sockets.size());

    // configure speedwire packet consumers (as virtual functions are used, we need to work with pointers here)
    EmeterPacketReceiver   emeter_packet_receiver(localhost);
    InverterPacketReceiver inverter_packet_receiver(localhost);
    std::vector<SpeedwirePacketReceiver*> packet_receivers;
    packet_receivers.push_back(&emeter_packet_receiver);
    packet_receivers.push_back(&inverter_packet_receiver);

    // configure speedwire packet sender for multicast to each local interface
    std::vector<SpeedwirePacketSender*> packet_senders;
    std::vector<std::string> ipv4_addresses = localhost.getLocalIPv4Addresses();
    for (auto& addr : ipv4_addresses) {
        SpeedwireSocket& send_socket = socket_factory->getSendSocket(SpeedwireSocketFactory::MULTICAST, addr);
        packet_senders.push_back(new MulticastPacketSender(localhost, addr, AddressConversion::toString(send_socket.getSpeedwireMulticastIn4Address())));
    }

    // configure speedwire packet sender for unicast to single speedwire devices not directly reachable by multicast
    for (auto& device : devices) {
        const std::string& peer_ip = device.peer_ip_address;
        bool is_reachable_by_multicast = false;
        for (auto& addr : ipv4_addresses) {
            if (AddressConversion::resideOnSameSubnet(AddressConversion::toInAddress(peer_ip), AddressConversion::toInAddress(addr), localhost.getInterfacePrefixLength(addr)) == true) {
                is_reachable_by_multicast = true;
                break;
            }
        }
        if (is_reachable_by_multicast == false) {
            SpeedwireSocket& send_socket = socket_factory->getSendSocket(SpeedwireSocketFactory::UNICAST, peer_ip);
            packet_senders.push_back(new UnicastPacketSender(localhost, device.interface_ip_address, peer_ip));
        }
    }

    //
    // main loop
    //
    while(true) {
        const unsigned long poll_timeout_in_ms = 2000;

        // poll sockets for inbound speedwire packets
        int npackets = poll_sockets(recv_sockets, fds, poll_timeout_in_ms, packet_receivers, packet_senders);
    }
    free(fds);

    return 0;
}


/**
 +  poll all configured sockets for emeter udp packets and pass emeter data to the obis filter 
 */
static int poll_sockets(const std::vector<SpeedwireSocket> &sockets, struct pollfd* const fds, const int poll_timeout_in_ms, std::vector<SpeedwirePacketReceiver*> &packet_receivers, std::vector<SpeedwirePacketSender*>& packet_senders) {
    int npackets = 0;
    unsigned char udp_packet[2048];

    // prepare the pollfd structure
    for (int j = 0; j < sockets.size(); ++j) {
        fds[j].fd      = sockets[j].getSocketFd();
        fds[j].events  = POLLIN;
        fds[j].revents = 0;
    }

    // wait for a packet on the configured socket
    if (poll(fds, (unsigned)sockets.size(), poll_timeout_in_ms) < 0) {
        perror("poll failure");
        return -1;
    }

    // determine if the socket received a packet
    for (int j = 0; j < sockets.size(); ++j) {
        auto& socket = sockets[j];

        if ((fds[j].revents & POLLIN) != 0) {
            int nbytes = -1;

            // read packet data
            struct sockaddr src;
            if (socket.isIpv4()) {
                nbytes = socket.recvfrom(udp_packet, sizeof(udp_packet), *(sockaddr_in*)&src);
            }
            else if (socket.isIpv6()) {
                nbytes = socket.recvfrom(udp_packet, sizeof(udp_packet), *(sockaddr_in6*)&src);
            }

            // check if it is an sma speedwire packet
            SpeedwireHeader speedwire_packet(udp_packet, nbytes);
            bool valid = speedwire_packet.checkHeader();
            if (valid) {
                // if so, pass it to all registered packet consumers
                for (auto& receiver : packet_receivers) {
                    SpeedwirePacketReceiver::PacketStatus status = receiver->receive(speedwire_packet, src);
                    if (status == SpeedwirePacketReceiver::PacketStatus::FORWARD) {
                        for (auto& sender : packet_senders) {
                            sender->send(speedwire_packet, src);
                        }
                    }
                }
            }
        }
    }
    return npackets;
}
