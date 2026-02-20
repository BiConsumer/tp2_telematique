#include <iostream>
#include <string>
#include <vector>

#include "Computer/Computer.h"
#include "Transmission/Transmission.h"
#include "Computer/Driver/Layer/LinkLayerLow.h"

struct Config {
    size_t NumberComputer = 2;
    size_t FirstNumber = 1;
    std::string GlobalConfigName = "";
    size_t CodeCorrecteur = 0; // 0 = Aucun, 1 = CRC, 2 = Hamming
};

Config parse_arguments(int argc, char* argv[]) {
    Config config;
    config.NumberComputer = 2;

    for (int i = 1; i < argc; ++i) {
        char* arg = argv[i];
        if (std::string(arg) == "-c") {
            if (i + 1 < argc) {
                std::cout << "Initialization avec " << argv[i + 1]
                          << " ordinateurs." << std::endl;
                int numberComputer = std::stoi(argv[i + 1]);
                if (numberComputer < 1) {
                    std::cout
                        << "Le nombre d'ordinateur doit etre superieur a 1."
                        << std::endl;
                } else {
                    config.NumberComputer = (size_t)numberComputer;
                }
            } else {
                std::cout << "Le parametre -c doit etre suivi du nombre "
                             "d'ordinateur a mettre en connexion."
                          << std::endl;
            }
        } else if (std::string(arg) == "-f") {
            if (i + 1 < argc) {
                std::cout << "Premier numero des ordinateurs : " << argv[i + 1]
                          << std::endl;
                int firstNumber = std::stoi(argv[i + 1]);
                if (firstNumber < 0) {
                    std::cout << "Le premier numero doit etre superieur a 0."
                              << std::endl;
                } else {
                    config.FirstNumber = (size_t)firstNumber;
                }
            } else {
                std::cout << "Le parametre -f doit etre suivi du numero du "
                             "premier ordinateur de la simulation."
                          << std::endl;
            }
        } else if (std::string(arg) == "-g") {
            if (i + 1 < argc) {
                std::cout << "Configuration globale a utiliser : "
                          << argv[i + 1] << std::endl;
                config.GlobalConfigName = std::string(argv[i + 1]);
            } else {
                std::cout
                    << "Le parametre -g doit etre suivi du nom de fichier de "
                       "configuration principal de la simulation."
                    << std::endl;
            }
        } else if (std::string(arg) == "-t") {
            if (i + 1 < argc) {
                int code = std::stoi(argv[i + 1]);
                if (code < 2 && code > 0) {
                    config.CodeCorrecteur = code;
                } else {
                    std::cout
                        << "Le parametre -t doit avoir un code plus grand que "
                           "0 et plus petit que 3 (1 = CRC; 2 = Hamming)."
                        << std::endl;
                }
            } else {
                std::cout
                    << "Le parametre -t doit etre suivi du numero du code "
                       "correcteur a utiliser (1 = CRC; 2 = Hamming)."
                    << std::endl;
            }
        }
    }

    return config;
}

void execute_correction_erreur_test(size_t codeCorrecteur) {
    const uint8_t* str_test = (uint8_t*)((void*)("Test encodage"));
    DynamicDataBuffer buffer = DynamicDataBuffer(13, str_test);
    if (codeCorrecteur == 1) {
        // Un encoder/decoder CRC ne fait que detecter des erreurs. Il ne les
        // corrige pas.
        CRCDataEncoderDecoder encoder;
        DynamicDataBuffer outBuffer = encoder.encode(buffer);
        outBuffer[1] = 'a';
        if (encoder.decode(outBuffer).first == false) {
            std::cout << "Test 1 : Buffer en erreur -> Test valide"
                      << std::endl;
        } else {
            std::cout << "Test 1 : Buffer en erreur -> Test invalide"
                      << std::endl;
        }

        outBuffer = encoder.encode(buffer);
        if (encoder.decode(outBuffer).first == true) {
            std::cout << "Test 2 : Buffer valide -> Test valide" << std::endl;
        } else {
            std::cout << "Test 2 : Buffer valide -> Test invalide" << std::endl;
        }
    } else if (codeCorrecteur == 2) {
        // Un encoder/decodeur de Hamming detecte et corrige les erreurs.
        HammingDataEncoderDecoder encoder;
        DynamicDataBuffer outBuffer = encoder.encode(buffer);
        outBuffer[1] = 'a';
        std::pair<bool, DynamicDataBuffer> decodedBuffer = encoder.decode(
            outBuffer);
        if (decodedBuffer.first == false) {
            std::cout << "Test 1 : Buffer en erreur -> Test invalide"
                      << std::endl;
        } else {
            if (decodedBuffer.second == buffer) {
                std::cout << "Test 1 : Buffer en erreur -> Test valide"
                          << std::endl;
            } else {
                std::cout << "Test 1 : Buffer en erreur -> Test invalide"
                          << std::endl;
            }
        }

        outBuffer = encoder.encode(buffer);
        decodedBuffer = encoder.decode(outBuffer);
        if (decodedBuffer.first == true) {
            if (decodedBuffer.second == buffer) {
                std::cout << "Test 2 : Buffer valide -> Test valide"
                          << std::endl;
            } else {
                std::cout << "Test 2 : Buffer valide -> Test invalide"
                          << std::endl;
            }
        } else {
            std::cout << "Test 2 : Buffer valide -> Test invalide" << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    Config config = parse_arguments(argc, argv);
    if (config.CodeCorrecteur > 0) {
        execute_correction_erreur_test(config.CodeCorrecteur);
        return 0;
    }

    Configuration globalConfig(config.GlobalConfigName);

    std::cout << "Demarrage du simulateur..." << std::endl;

    TransmissionHub hub(globalConfig);

    std::vector<Computer*> computers;
    // Create computer and connect it to Hub
    for (size_t i = 0; i < config.NumberComputer; ++i) {
        Computer* computer = new Computer(i + config.FirstNumber);
        hub.connect_computer(computer);
        computers.push_back(computer);
    }
    hub.start();

    for (auto computer : computers) {
        computer->start();
    }

    unsigned int numbercomputerFinished = 0;
    unsigned int numberFileReceived = 0;
    unsigned int numberFileSent = 0;
    do {
        numbercomputerFinished = 0;
        numberFileReceived = 0;
        numberFileSent = 0;
        for (const Computer* computer : computers) {
            if (computer->sendingTerminated()) {
                ++numbercomputerFinished;
            }
            numberFileSent += computer->sentFileCount();
            numberFileReceived += computer->receivedFileCount();
        }
    } while (numbercomputerFinished != computers.size() ||
             numberFileReceived != numberFileSent);

    // std::cout << "Press something to stop" << std::endl;
	std::cin.get();
    hub.stop();
    for (auto computer : computers) {
        delete computer;
    }
    computers.clear();
}
