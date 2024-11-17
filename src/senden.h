#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <b15f/b15f.h>

void inputSender(B15F &drv) {
    std::string buffer;
    while (getline(std::cin, buffer)) {
        vector<uint8_t> block;
        for (char c : buffer) {
            block.push_back(c);
            if (block.size() == 64) {
                sendBlock(drv, block);
                block.clear();
            }
        }

        // Restdaten sind kleiner 64
        if (!block.empty()) {
            block.resize(64, 0); // Erweitern mit nullen für Block
            sendBlock(drv, block);
        }
    }
}

uint8_t berechnungSumme(const vector<uint8_t> &data) {
    uint8_t checksum = 0;
    for (uint8_t byte : data) {
        checksum ^= byte;
    }
    return checksum;
}

// Sendet das Datenpacket
void sendBlock(B15F &drv, const vector<uint8_t> &block) {
    lock_guard<mutex> lock(b15_mutex); // Protect B15F access
    uint8_t checksum = berechnungSumme(block);
    vector<uint8_t> packet = block;
    packet.push_back(checksum); // Append checksum

   for (size_t z = 0; z < packet.size(); i++) {
        uint8_t byte = packet.at(z);
        for (int i = 0; i < 8; i += 3) {            // Übertragung 3-Bizs
            uint8_t bitsToSend = (byte >> i) & 0x07; // Nimmt 3 bits
            if(z%2 == 0) drv.setRegister(&PORTA, bitsToSend| 0x08);    // Send 3 bits on lines 0-2
            if(z%2 == 1) drv.setRegister(&PORTA, bitsToSend| 0x00);
        }
    }

}