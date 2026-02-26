#include "Computer/Computer.h"
#include "Transmission/Transmission.h"
#include "Computer/Driver/Layer/LinkLayerLow.h"
#include <thread>
#include <chrono>

void send_to_and_wait_receive(Computer& sender,
                              Computer& receiver,
                              const std::string& filename) {
    sender.getNetworkInterfaceCard().getDriver().getNetworkLayer().start();

    const MACAddress receiverMac = receiver.getNetworkInterfaceCard()
                                       .getDriver()
                                       .getMACAddress();

    sender.send_file_to(receiverMac, filename);

    const std::size_t receiverReceived = receiver.receivedFileCount();
    while (receiverReceived == receiver.receivedFileCount()) {
        // Attendre que le receveur recoive le message
        _mm_pause();
    }
}

int main() {
    using namespace std::chrono_literals;
    std::cout << "Starting tests" << std::endl;

    Computer computer1{1};
    Computer computer2{2};
    Computer computer3{3};

    Configuration global_config{"config_no_noise.txt"};
    TransmissionHub hub{global_config};

    hub.connect_computer(&computer1);
    hub.connect_computer(&computer2);
    hub.connect_computer(&computer3);
    hub.start();

    // Test 1: Computer 1 envoie un message a Computer 2 et Computer 2 envoie
    // ACK en timeout
    std::cout << "--------------- Test 1: ACK on timeout ---------------"
              << std::endl;
    send_to_and_wait_receive(computer1, computer2, "test.txt");
    std::this_thread::sleep_for(2s);

    // Test 2: Computer 2 envoie ACK a Computer 1 avec piggybacking
    std::cout << "--------------- Test 2: ACK on piggyback ---------------"
              << std::endl;
    send_to_and_wait_receive(computer1, computer2, "test.txt");
    std::this_thread::sleep_for(50ms);
    send_to_and_wait_receive(computer2, computer1, "test.txt");
    std::this_thread::sleep_for(2s);

    // Test 3: Computer 1 et 3 envoient un message a Computer 2 et Computer 2
    // send un ACK en timeout
    std::cout << "--------------- Test 3: threesome ---------------"
              << std::endl;
    send_to_and_wait_receive(computer1, computer2, "test.txt");
    send_to_and_wait_receive(computer3, computer2, "test.txt");
    std::this_thread::sleep_for(2s);

    // Test 4: Computer 2 envoie ACK a Computer 1 et 3 avec piggybacking
    std::cout << "--------------- Test 4: threesome piggyback ---------------"
              << std::endl;
    send_to_and_wait_receive(computer1, computer2, "test.txt");
    send_to_and_wait_receive(computer3, computer2, "test.txt");
    std::this_thread::sleep_for(50ms);
    send_to_and_wait_receive(computer2, computer1, "test.txt");
    send_to_and_wait_receive(computer2, computer3, "test.txt");
    std::this_thread::sleep_for(2s);
    return 0;
}
