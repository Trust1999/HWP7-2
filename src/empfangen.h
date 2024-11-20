#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <b15f/b15f.h>

// CRC-8 Implementierung (wie im Sender)
uint8_t berechnungSummeZurueck(const std::vector<uint8_t> &data) {
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

std::vector<char> umwandeln(std::vector<uint8_t> &ed){
    std::vector<uint8_t> wert;
    uint8_t speicherByte;
    
    
    size_t outputSize = ed.size() / 3;

    for (size_t i = 0; i < outputSize; ++i) {
        uint8_t speicherByte = 0;

        if(ed.at(i*3) == 0b0000 1100 || ed.at(i*3) == 0b0000 0100){
            break;
        }
        speicherByte |= (ed.at(i * 3) >> 3);         // Nimm 5 höchstwertige Bits aus dem ersten Byte
        speicherByte |= (ed.at(i * 3 + 1) >> 3);     // Nimm 5 höchstwertige Bits aus dem zweiten Byte
        speicherByte |= (ed(i * 3 + 2));   // Nimm 5 niederwertige Bits aus dem dritten Byte
        wert[i] = static_cast<char>(speicherByte); // In char umwandeln und speichern
    }

    return wert;
}

// Function to receive a 64-byte block with 9-bit data format
std::vector<uint8_t> erhaltenesPacket(B15F &drv) {
    //lock_guard<mutex> lock(b15_mutex); // Protect B15F access
    std::vector<uint8_t> erhalteneDaten
    size_t counterClock = 0;

    while (true) { // 64 - Daten + Check Summe

       while (!(drv.getRegister(&PINA) & 0x80)) {} // warte 1-clock

       erhalteneDaten.push_back( drv.getRegister(&PINA) & 0x70); // Lese 3 bits vom Pin 4-6
       
       while (drv.getRegister(&PINA) & 0x80) {} // warte 0-Clock
        
       erhalteneDaten.push_back( drv.getRegister(&PINA) & 0x70);
    }

    std::vector<char> zeichen = umwandeln(erhalteneDaten);
    if(berechnungSummeZurueck() == ){
        zeichen.clear();
    }
    melden();
    return zeichen;
}

void outputReceiver(B15F &drv) {

    while (true) {
        std::vector<uint8_t> block = erhaltenesPacket(drv);
        if (!block.empty()) {
            std::string output(block.begin(), block.end());
            //std::cout << output << flush; // schreiben zu STD:OUT
        }
    }
}