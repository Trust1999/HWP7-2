#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <b15f/b15f.h>

// Function to receive a 64-byte block with 9-bit data format
std::vector<uint8_t> receiveBlock(B15F &drv) {
    //lock_guard<mutex> lock(b15_mutex); // Protect B15F access
    std::vector<uint8_t> erhalteneDaten;
    uint8_t byte = 0;
    int bitPosition = 0;

    while (erhalteneDaten.size() < 65) { // 64 - Daten + Check Summe

       while (!(drv.getRegister(&PINA) & 0x08)) { // Wait for clock high
            drv.delay_ms(1);
        }

        erhalteneDaten.push_back( drv.getRegister(&PINA) & 0x07); // Read 3 bits from lines 0-2
        //byte |= (receivedBits << bitPosition); // Add bits to the current byte
        ///bitPosition += 3;

        if (bitPosition >= 8) { // If a full byte has been received
            //receivedData.push_back(byte);
            byte = 0;
            bitPosition = 0;
        }

        while (drv.getRegister(&PINA) & 0x08) { // Wait for clock low
            drv.delay_ms(1);
        }
        erhalteneDaten.push_back( drv.getRegister(&PINA) & 0x07);
    }

    // Verify checksum
    uint8_t erhaltenePruefsumme = erhalteneDaten.back();
    erhalteneDaten.pop_back(); // Remove checksum from data
    //uint8_t calculatedChecksum = calculateChecksum(receivedData);

    /*if (calculatedChecksum != receivedChecksum) {
        cerr << "Checksum mismatch! Data corrupted." << endl;
        return {};
    }*/

    return erhalteneDaten;
}

void outputReceiver(B15F &drv) {

    while (true) {
        std::vector<uint8_t> block = receiveBlock(drv);
        if (!block.empty()) {
            std::string output(block.begin(), block.end());
            //std::cout << output << flush; // Write to STD:OUT
        }
    }
}