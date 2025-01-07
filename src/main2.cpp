#include <iostream>
#include <vector>
#include <bitset>

bool ende;
std::string output;

std::vector<uint8_t> umwandeln(const std::vector<uint8_t>& ed) {
   std::vector<uint8_t> paket;

    for (size_t i = 0; i < ed.size(); i += 3) {
        uint8_t speicherByte = 0;

        if (ed.at(i) & 0x04) {
            speicherByte |= (ed.at(i) << 6);
            speicherByte |= (ed.at(i + 1) << 3);
            speicherByte |= ed.at(i + 2);

            if (speicherByte == 0b00100100) {  // End condition
                ende = true;
                std::cerr<<"ende";
                return paket;  // Early exit if end pattern is found
            }
        }
        else {
            speicherByte |= (ed.at(i) << 6);  // Take the 5 most significant bits from the first byte
            speicherByte |= (ed.at(i + 1) << 3);  // Take 5 more from the second byte
            speicherByte |= ed.at(i + 2);  // Take the last 5 bits from the third byte
        }

        paket.push_back(speicherByte);  // Add the processed byte to the result
    }

    return paket;
}

uint8_t auslesen() {
    //uint8_t byte = (drv.getRegister(&PINA) & 0xF0) >> 4;
    uint8_t byte = 0;
    return byte;
}

uint8_t lesen() {
    uint8_t zwischenspeicher = auslesen();
    size_t i = 0;
    while (zwischenspeicher == auslesen() && i <= 10) {
        // drv.delay_ms(1);
        i++;
    }
    std::cerr<<"E "<<auslesen()<<" ";
    return (auslesen() & 0x07);
}

uint8_t gerdehtesByte(uint8_t byte) {
    unsigned char gedreht = 0;
    for (int i = 0; i < 8; ++i) {
        gedreht <<= 1;
        gedreht |= (byte & 1);
        byte >>= 1;
    }
    return gedreht >> 4;
}

void schreiben(uint8_t byte, uint8_t cC) {
    uint8_t senden = gerdehtesByte(byte | (cC % 2 * 8));
    //drv.setRegister(&PORTA, senden);
    std::cerr<<"S "<<senden<<" ";
}

uint8_t berechnungSumme(const std::vector<uint8_t>& data) {
    uint8_t crc = 0;
    uint8_t polynomial = 0x07;
    for (uint8_t byte : data) {
        crc ^= byte;
        for (int i = 0; i < 8; i++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;
            }
            else {
                crc <<= 1;
            }
        }
  }
  return crc;
}

bool pruefen(std::vector<uint8_t>& blockDaten) {
    uint8_t crc = blockDaten.back();
    blockDaten.pop_back();
    if (ende) {
        schreiben(0x0D, 0);
        blockDaten.clear();
    } 
    else {
        if (berechnungSumme(blockDaten) == crc) {
            schreiben(0x0C, 0);
            output.append(blockDaten.begin(), blockDaten.end());
        }
        else {
            schreiben(0x0D, 0);
            ende = false;
            blockDaten.clear();
        }
    }

    uint8_t test = 0x00;
    //schreib Test
    while(true){
        if(test == 0x0D){
                std::cerr<<"Falsches CRC";
                return true;
            }
            else if(test == 0x0C){
                std::cerr<<"Richtiges CRC";
                return false;
            }
        test = auslesen();
    }
    return true;
}

void weiterLesen() {
    std::vector<uint8_t> erhalteneDaten;
    bool erneutSenden = false;
    for (size_t i = 0; i <= 195; i++) {
        schreiben(0x00, i);
        erhalteneDaten.push_back(lesen());
    }
    std::vector<uint8_t> blockDaten = umwandeln(erhalteneDaten);
    erneutSenden = pruefen(blockDaten);
}

void sendBlock(const std::vector<uint8_t>& paket, size_t position) {
    std::vector<uint8_t> block = paket;
    std::vector<uint8_t> erhalteneDaten;
    bool erneutSenden = true;
    do {
        size_t counterClock = 0;
        for (size_t z = 0; z < block.size(); z++) {
            uint8_t maske = 0x00;
            uint8_t byte = block.at(z);
            if (z == position) maske = 0x04;
            for (int i = 6; i >= 0; i -= 3) {
                 uint8_t bitsToSend = (byte >> i) & 0x07 | maske;
                schreiben(bitsToSend, counterClock);
                erhalteneDaten.push_back(lesen());
                std::cerr << "E" << counterClock << " ";
                counterClock++;
            }
        }
        std::vector<uint8_t> blockDaten = umwandeln(erhalteneDaten);
        erneutSenden = pruefen(blockDaten);
        schreiben(0x00, 0);
    } while (erneutSenden);
    schreiben(0x00, 0);
}

void sendBlock(const std::vector<uint8_t>& paket) {
    std::vector<uint8_t> block = paket;
    std::vector<uint8_t> erhalteneDaten;
    bool erneutSenden = true;
    do {
        size_t counterClock = 0;
        for (uint8_t byte : block) {
            for (int i = 6; i >= 0; i -= 3) {
                uint8_t bitsToSend = (byte >> i) & 0x07;
                schreiben(bitsToSend, counterClock);
                erhalteneDaten.push_back(lesen());
                std::cerr << counterClock << " ";
                counterClock++;
            }
        }
        std::vector<uint8_t> blockDaten = umwandeln(erhalteneDaten);
        erneutSenden = pruefen(blockDaten);
        schreiben(0x00, 0);
    } while (erneutSenden);
}

uint8_t berechnungSumme(const std::vector<uint8_t>& data) {
    uint8_t crc = 0;    // Initial CRC-Wert
    uint8_t polynomial = 0x07;  // CRC-8 Polynom

    for (uint8_t byte : data) {
        crc ^= byte;    // XOR mit dem aktuellen Byte
        for (int i = 0; i < 8; i++) {   // Für jedes Bit im Byte
            if (crc & 0x80) {   // Wenn das höchste Bit gesetzt ist
                crc = (crc << 1) ^ polynomial;
            } 
            else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void startSenden() {
    std::vector<uint8_t> buffer(4096);
    std::vector<uint8_t> paket;
    while (std::cin.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || std::cin.gcount() > 0) {
        size_t bytesRead = std::cin.gcount();
        if (buffer.size() < bytesRead) {
            buffer.reserve(buffer.size() + 2048);
        }

        for (size_t i = 0; i < bytesRead; ++i) {
            paket.push_back(buffer[i]);
            if (paket.size() == 64) {
                paket.push_back(berechnungSumme(paket));
                sendBlock(paket);
                paket.clear();
            }
        }
    }

    paket.push_back(berechnungSumme(paket));
    size_t position = paket.size();
    paket.resize(65, 0);
    sendBlock(paket, position);
}

void speichern() {
    std::cout << output << std::flush;
}

int main() {
    ende = false;
    startSenden();
  while (!ende) {
    weiterLesen();
  }

  speichern();
  return 0;
}
