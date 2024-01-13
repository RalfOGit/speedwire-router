#include <LocalHost.hpp>
#include <Logger.hpp>
#include <SpeedwireAuthentication.hpp>
#include <SpeedwireCommand.hpp>
#include <SpeedwireDiscovery.hpp>
#include <SpeedwireHeader.hpp>
#include <SpeedwireReceiveDispatcher.hpp>
#include <SpeedwirePacketReceiver.hpp>
#include <SpeedwirePacketSender.hpp>
#include <SpeedwireSocketFactory.hpp>
#include <SpeedwireSocket.hpp>
using namespace libspeedwire;

static Logger logger("main");

class LogListener : public ILogListener {
public:
    virtual void log_msg(const std::string& msg, const LogLevel &level) {
        fprintf(stdout, "%s", msg.c_str());
    }
    virtual void log_msg_w(const std::wstring& msg, const LogLevel &level) {
        fprintf(stdout, "%ls", msg.c_str());
    }
};


int main(int argc, char **argv) {

    // configure logger and logging levels
    ILogListener *log_listener = new LogListener();
    LogLevel log_level = LogLevel::LOG_ERROR | LogLevel::LOG_WARNING;
    log_level = log_level | LogLevel::LOG_INFO_0;
    log_level = log_level | LogLevel::LOG_INFO_1;
    //log_level = log_level | LogLevel::LOG_INFO_2;
    //log_level = log_level | LogLevel::LOG_INFO_3;
    Logger::setLogListener(log_listener, log_level);

    // discover sma devices on the local network
    
    LocalHost& localhost = LocalHost::getInstance();
    logger.print(LogLevel::LOG_INFO_0, "starting device discovery ...\n");
    SpeedwireDiscovery discoverer(localhost);
    discoverer.preRegisterDevice("192.168.182.18");
    int num_devices = discoverer.discoverDevices();
    logger.print(LogLevel::LOG_INFO_0, "... finished device discovery\n");
    if (num_devices == 0) {
        logger.print(LogLevel::LOG_WARNING, "... no speedwire device found\n");
    }
    std::vector<SpeedwireDevice> devices = discoverer.getDevices();

    // open socket(s) to receive sma emeter packets from any local interface
    SpeedwireSocketFactory *socket_factory = SpeedwireSocketFactory::getInstance(localhost);
    const std::vector<SpeedwireSocket> recv_sockets = socket_factory->getRecvSockets(SpeedwireSocketFactory::SocketType::ANYCAST, localhost.getLocalIPv4Addresses());

    // configure speedwire packet sender for multicast to each local interface
    std::vector<SpeedwirePacketSender*> multicast_packet_senders;
    std::vector<std::string> ipv4_addresses = localhost.getLocalIPv4Addresses();
    for (auto& addr : ipv4_addresses) {
        SpeedwireSocket& send_socket = socket_factory->getSendSocket(SpeedwireSocketFactory::SocketType::MULTICAST, addr);
        multicast_packet_senders.push_back(new MulticastPacketSender(localhost, addr, AddressConversion::toString(send_socket.getSpeedwireMulticastIn4Address())));
    }

    // configure speedwire packet sender for unicast to single speedwire devices not directly reachable by multicast
    for (auto& device : devices) {
        const std::string& peer_ip = device.deviceIpAddress;
        bool is_reachable_by_multicast = false;
        for (auto& addr : ipv4_addresses) {
            if (AddressConversion::resideOnSameSubnet(AddressConversion::toInAddress(peer_ip), AddressConversion::toInAddress(addr), localhost.getInterfacePrefixLength(addr)) == true) {
                is_reachable_by_multicast = true;
                break;
            }
        }
        if (is_reachable_by_multicast == false) {
            SpeedwireSocket& send_socket = socket_factory->getSendSocket(SpeedwireSocketFactory::SocketType::UNICAST, peer_ip);
            multicast_packet_senders.push_back(new UnicastPacketSender(localhost, device.interfaceIpAddress, peer_ip));
        }
    }

    // configure speedwire packet consumers
    EmeterPacketReceiver   emeter_packet_receiver(localhost, multicast_packet_senders);
    InverterPacketReceiver inverter_packet_receiver(localhost, multicast_packet_senders);
    DiscoveryPacketReceiver discovery_packet_receiver(localhost, multicast_packet_senders);

    // configure speedwire packet receive dispatcher
    SpeedwireReceiveDispatcher dispatcher(localhost);
    dispatcher.registerReceiver(emeter_packet_receiver);
    dispatcher.registerReceiver(inverter_packet_receiver);
    dispatcher.registerReceiver(discovery_packet_receiver);

#if 0
    SpeedwireAuthentication authenticator(localhost, discoverer.getDevices());
    authenticator.logoffAnyFromAny();
    authenticator.loginAnyToAny(true, "9999", 1000);
#endif

    //
    // main loop
    //
    const int poll_timeout_in_ms = 2000;
    while(true) {
        dispatcher.dispatch(recv_sockets, poll_timeout_in_ms);
    }

    return 0;
}
