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

// Function to receive a 64-byte block with 9-bit data format
std::vector<uint8_t> receiveBlock(B15F &drv) {
    lock_guard<mutex> lock(b15_mutex); // Protect B15F access
    std::vector<uint8_t> erhalteneDaten;
    uint8_t byte = 0;
    int bitPosition = 0;

    while (erhalteneDaten.size() < 65) { // 64 - Daten + Check Summe

       while (!(drv.getRegister(&PINA) & 0x08)) { // Wait for clock high
            drv.delay_ms(1);
        }

        // 3-Bit-Daten aus den Leitungen 0-2 lesen
        uint8_t receivedBits = 00000000;//drv.getRegister(&PINA) & 0x07;
        byte |= (receivedBits << (5 - bitPosition)); // Bits in das Byte integrieren
        bitPosition += 3;

        if (bitPosition >= 8) { // Volles Byte empfangen
            erhalteneDaten.push_back(byte);
            byte = 0;
            bitPosition = 0;
        }

        // Auf Clock-Signal warten (Low)
        while (drv.getRegister(&PINA) & 0x08) {
            drv.delay_ms(1);
        }
    }
   
    // Prüfsumme validieren
    uint8_t receivedChecksum = erhalteneDaten.back(); // Letztes Byte ist die Prüfsumme
    erhalteneDaten.pop_back(); // Prüfsummenbyte entfernen
    uint8_t calculatedChecksum = berechnungSummeZurueck(erhalteneDaten);

    if (calculatedChecksum != receivedChecksum) {
        std::cerr << "Prüfsummenfehler! Daten sind möglicherweise beschädigt." << std::endl;
        return {};
        //TODO Erneutes Senden anfordern
    }

    return erhalteneDaten; // Validierte Daten zurückgeben
}

void outputReceiver(B15F &drv) {

    while (true) {
        std::vector<uint8_t> block = receiveBlock(drv);
        if (!block.empty()) {
            std::string output(block.begin(), block.end());
            std::cout << output << flush; // Write to STD:OUT
        }
    }
}