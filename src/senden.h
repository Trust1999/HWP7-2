#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <bitset>
#include <mutex>
#include <b15f/b15f.h>

bool meldung(){
    while(true){
        return false;
    }
    return false;
}

uint8_t berechnungSumme(const std::vector<uint8_t> &data) {
    uint8_t crc = 0; // Initial CRC-Wert
    uint8_t polynomial = 0x07; // CRC-8 Polynom

    for (uint8_t byte : data) {
        crc ^= byte; // XOR mit dem aktuellen Byte
        for (int i = 0; i < 8; i++) { // Für jedes Bit im Byte
            if (crc & 0x80) { // Wenn das höchste Bit gesetzt ist
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void sendBlock(const std::vector<uint8_t> &paket, size_t position) {
    B15F &drv = B15F::getInstance();
    drv.setRegister(&DDRA, 0x0F);
    paket.push_back(berechnungSumme(paket));

    do{
        //lock_guard<mutex> lock(b15_mutex); // Protect B15F access
        //std::vector<uint8_t> block = paket;
        size_t counterClock = 0;

        for (size_t z = 0; z < paket.size(); z++) {
                uint8_t maske = 0x00;
                uint8_t byte = packet.at(z);

                if(z == position) maske = 0x04;

                for (int i = 6; i >= 0; i -= 3) {            // Übertragung 3-Bits
                    uint8_t bitsToSend = (byte >> i) & 0x07 | maske; // Nimmt 3 bits
                    
                    std::cout<< std::bitset<4>(bitsToSend| 0x0(counterClock%2 * 8))<<"; ";
                    drv.setRegister(&PORTA, (bitsToSend| 0x0(counterClock%2 * 8)));    // Send 3 bits on lines 0-2
        
                    drv.delay_ms(10);
                    counterClock++;
                }
            }
        drv.setRegister(&PORTA, (0b00000111| 0x0(counterClock%2 * 8)));
    }while(meldung())
}

// Sendet das Datenpacket
void sendBlock(const std::vector<uint8_t> &paket) {

    B15F &drv = B15F::getInstance();
    drv.setRegister(&DDRA, 0x0F);
    //lock_guard<mutex> lock(b15_mutex); // Protect B15F access
    paket.push_back(berechnungSumme(paket)); // Append checksum
    
    do{
        int counterClock = 0;
        drv.setRegister(&PORTA, (0b00000111| 0x0(counterClock%2 * 8))); //Start-Signal

        for (uint8_t byte: paket) {
            //uint8_t byte = block.at(z);
            for (int i = 6; i >= 0; i -= 3) {            // Übertragung 3-Bits
                uint8_t bitsToSend = (byte >> i) & 0x07; // Nimmt 3 bits
              
                std::cout<< std::bitset<4>(bitsToSend| 0x0(counterClock%2 * 8))<<"; ";
                drv.setRegister(&PORTA, (bitsToSend| 0x0(counterClock%2 * 8)));    // Send 3 bits on lines 0-2
        
                drv.delay_ms(10);
                counterClock++;
            }
        }
        drv.setRegister(&PORTA, (0b00000111| 0x0(counterClock%2 * 8))); //End-Signal
    }while(meldung())
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


/*
Code zum Testen der Prüfsumme [Gibt alles richtig aus, außer bei Zahl 3 Part 6 nicht (0100 statt 0000)]

#include <iostream>
#include <vector>
#include <bitset>

// CRC-8 Implementierung
uint8_t berechnungSumme(const std::vector<uint8_t> &data) {
    uint8_t crc = 0; // Initial CRC-Wert
    uint8_t polynomial = 0x07; // CRC-8 Polynom

    for (uint8_t byte : data) {
        crc ^= byte; // XOR mit dem aktuellen Byte
        for (int i = 0; i < 8; i++) { // Für jedes Bit im Byte
            if (crc & 0x80) { // Wenn das höchste Bit gesetzt ist
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// Mock-Funktion für sendBlock
void sendBlock(const std::vector<uint8_t> &paket, size_t position) {
    std::vector<uint8_t> block = paket;
    block.push_back(berechnungSumme(paket)); // Append CRC checksum
    int counterclock = 0;

    std::cout << "Datenblock mit CRC: ";
    for (uint8_t byte : block) {
        std::cout << std::bitset<8>(byte) << " ";
    }
    std::cout << "\n";

    for (size_t z = 0; z < block.size(); z++) {
        uint8_t maske = (z == position) ? 0x04 : 0x00;
        uint8_t byte = block.at(z);
        std::cout << "Verarbeite Byte: " << std::bitset<8>(byte) << "\n";

        for (int i = 6; i >= 0; i -= 3) {
        uint8_t rawBits = (byte >> i) & 0x07; // Extrahiere 3 Bits
        uint8_t bitsToSend = rawBits;

        // Wende die Maske nur beim ersten Block an (i == 6)
        if (i == 6 && z == position) {
            bitsToSend |= maske;
        }

        std::cout << "Byte: " << std::bitset<8>(byte)
                << " Pos: " << i
                << " RawBits: " << std::bitset<4>(rawBits)
                << " Maske: " << std::bitset<4>(maske)
                << " Bits gesendet: " << std::bitset<4>(bitsToSend) << "\n";
        }
    }
}

// Einfacher Testmodus
void testCRC() {
    std::vector<uint8_t> testData = {0x01, 0x02, 0x03, 0x04};
    uint8_t crc = berechnungSumme(testData);

    std::cout << "Testdaten: ";
    for (uint8_t byte : testData) {
        std::cout << std::bitset<8>(byte) << " ";
    }
    std::cout << "\nBerechneter CRC-8: " << std::bitset<8>(crc) << "\n";

    sendBlock(testData, 2);
}

int main() {
    testCRC();
    return 0;
}
*/