#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <bitset>
#include <mutex>
#include <b15f/b15f.h>

uint8_t berechnungSumme(const std::vector<uint8_t> &data) {
    uint8_t checksum = 0;
    for (uint8_t byte : data) {
        //checksum ^= byte;
        checksum = 0;
    }
    return checksum;
}

void sendBlock(const std::vector<uint8_t> &paket, size_t position) {

    B15F &drv = B15F::getInstance();
    drv.setRegister(&DDRA, 0x0F);

    //lock_guard<mutex> lock(b15_mutex); // Protect B15F access
    std::vector<uint8_t> block = paket;
    block.push_back(berechnungSumme(paket)); // Append checksum
    int counterclock = 0;

   for (size_t z = 0; z < block.size(); z++) {
        uint8_t maske = 0x00;
        uint8_t byte = block.at(z);
        if(z == position) maske = 0x04;
        for (int i = 6; i >= 0; i -= 3) {            // Übertragung 3-Bits
            uint8_t bitsToSend = (byte >> i) & 0x07 | maske; // Nimmt 3 bits
            if(counterclock%2 == 0) std::cout<< std::bitset<4>(bitsToSend| 0x08)<<"; ";
            if(counterclock%2 == 1) std::cout<< std::bitset<4>(bitsToSend| 0x00)<<"; ";
            if(counterclock%2 == 0) drv.setRegister(&PORTA, (bitsToSend| 0x08));    // Send 3 bits on lines 0-2
            if(counterclock%2 == 1) drv.setRegister(&PORTA, (bitsToSend| 0x00));
            drv.delay_ms(10);
            counterclock++;
        }
    }
}

// Sendet das Datenpacket
void sendBlock(const std::vector<uint8_t> &paket) {

    B15F &drv = B15F::getInstance();
    drv.setRegister(&DDRA, 0x0F);

    //lock_guard<mutex> lock(b15_mutex); // Protect B15F access
    std::vector<uint8_t> block = paket;
    block.push_back(berechnungSumme(paket)); // Append checksum
    int counterclock = 0;

   for (size_t z = 0; z < block.size(); z++) {
        uint8_t byte = block.at(z);
        std::cout<<"A";
        for (int i = 6; i >= 0; i -= 3) {            // Übertragung 3-Bits
            std::cout<<"B";
            uint8_t bitsToSend = (byte >> i) & 0x07; // Nimmt 3 bits
            if(counterclock%2 == 0) std::cout<< std::bitset<4>(bitsToSend| 0x08)<<"; ";
            if(counterclock%2 == 1) std::cout<< std::bitset<4>(bitsToSend| 0x00)<<"; ";
            if(counterclock%2 == 0) drv.setRegister(&PORTA, (bitsToSend| 0x08));    // Send 3 bits on lines 0-2
            if(counterclock%2 == 1) drv.setRegister(&PORTA, (bitsToSend| 0x00));
            drv.delay_ms(500);
            counterclock++;
        }
    }

}

void inputSender() {

    std::string buffer;
    while (getline(std::cin, buffer)) {
        std::vector<uint8_t> paket;
        for (char c : buffer) {
            std::cout<<std::bitset<8>(c)<<'\n';
            paket.push_back(c);
            if (paket.size() == 64) {
                sendBlock(paket);
                paket.clear();
            }
        }

        // Restdaten sind kleiner 64
        if (!paket.empty()) {
            size_t position = paket.size();
            paket.resize(64, 0); // Erweitern mit nullen für Block
            sendBlock(paket, position);
            break;
        }
    }
}