#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <bitset>
#include <b15f/b15f.h>

//extern void inputSender();
//extern void outputReceiver();

B15F drv = B15F :: getInstance () ; //drv wird ein Objekt einer Klasse
std::mutex b15_mutex;
bool ende = true;

// CRC-8 Implementierung (wie im Sender)
/*uint8_t berechnungSummeZurueck(const std::vector<uint8_t> &data) {
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
    return 1;
}*/

uint8_t umwandeln(std::vector<uint8_t> &ed){
    uint8_t speicherByte;
        
    size_t outputSize = ed.size() / 3;

    if(ed.at(0) == 0b00000101 && ed.at(1) == 0b00000010 && ed.at(2) == 0b00000101){
        ende = false;
        return 0;
    }
    else{
        speicherByte |= (ed.at(2) >> 6);         // Nimm 5 höchstwertige Bits aus dem ersten Byte
        speicherByte |= (ed.at(1) >> 3);        // Nimm 5 höchstwertige Bits aus dem zweiten Byte
        speicherByte |= (ed.at(2));             // Nimm 5 niederwertige Bits aus dem dritten Byte; // In char umwandeln und speichern
    }
    std::cout<<"++++++"<<std::bitset<8>(speicherByte)<<"++++++";
    return speicherByte;
}

uint8_t auslesen(){
    std::lock_guard<std::mutex> lock(b15_mutex);
    std::cout<<std::bitset<8>((drv.getRegister(&PINA) & 0xF0) >> 4)<<"; ";
    return (drv.getRegister(&PINA) & 0xF0) << 6;
}

// Function to receive a 64-byte block with 9-bit data format
std::vector<uint8_t> erhaltenesPacket() {

    //lock_guard<mutex> lock(b15_mutex); // Protect B15F access
    std::vector<uint8_t> erhalteneDaten;
    std::vector<u_int8_t> paket;
    size_t counterClock = 0;

    while (ende) { // 64 - Daten + Check Summe

        if(counterClock%2 == 0){
            while ((auslesen() & 0x80) == 0x00) {   // warte 1-clock
                drv.delay_ms(1);
            }
        }
        else{
            while ((auslesen() & 0x80) == 0x80) {   // warte 1-clock
                drv.delay_ms(1);
            }
        }
        

        erhalteneDaten.push_back( auslesen() & 0x70); // Lese 3 bits vom Pin 4-6
        counterClock++;
        std::cout<<"----"<<counterClock<<"----";

        if(counterClock%3 == 0){
            paket.push_back(umwandeln(erhalteneDaten));
            erhalteneDaten.clear();
        }
    }
    
    ende = true;
    /*if(berechnungSummeZurueck(zeichen)){
        zeichen.clear();
    }
    //melden();*/
    return erhalteneDaten;
}

void outputReceiver() {

    while (true) {
        std::vector<uint8_t> block = erhaltenesPacket();
        if (!block.empty()) {
            std::string output(block.begin(), block.end());
            //std::cout << output << flush; // schreiben zu STD:OUT
        }
    }
}

bool meldung(){
    //drv.setRegister(&DDRA, 0x00);

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

void ausgabe(uint8_t byte, uint8_t cC){
    std::lock_guard<std::mutex> lock(b15_mutex);
    //std::cout<<std::bitset<4>(byte | (cC % 2 * 8))<<", ";
    drv.setRegister(&PORTA, byte | (cC % 2 * 8));    
} 

/*void sendBlock(const std::vector<uint8_t> &paket, size_t position) {

    std::vector<uint8_t> block = paket;
    block.push_back(berechnungSumme(paket));

    do{
        size_t counterClock = 0;

        ausgabe(0b00000111, counterClock);

        for (size_t z = 0; z < block.size(); z++) {
                uint8_t maske = 0x00;
                uint8_t byte = block.at(z);

                if(z == position) maske = 0x04;

                for (int i = 6; i >= 0; i -= 3) {            // Übertragung 3-Bits
                    uint8_t bitsToSend = (byte >> i) & 0x07 | maske; // Nimmt 3 bits
                    
                    ausgabe(bitsToSend, counterClock);

                    drv.delay_ms(10);
                    counterClock++;
                }
            }
            ausgabe(0b00000111, counterClock);
    }while(meldung());
}*/

// Sendet das Datenpacket
void sendBlock(const std::vector<uint8_t> &paket) {

    //lock_guard<mutex> lock(b15_mutex); // Protect B15F access
    std::vector<uint8_t> block = paket;
    block.push_back(berechnungSumme(paket)); // Append checksum
    
    do{
        size_t counterClock = 0;

        /*ausgabe(0b00000111, 1);
        drv.delay_ms(10);
        ausgabe(0b00000000, 0);
        drv.delay_ms(10);
        ausgabe(0b00000111, 1);
        drv.delay_ms(10);*/

        for (uint8_t byte: block) {
            //uint8_t byte = block.at(z);
            for (int i = 6; i >= 0; i -= 3) {            // Übertragung 3-Bits
                uint8_t bitsToSend = (byte >> i) & 0x07; // Nimmt 3 bits
              
                ausgabe(bitsToSend, counterClock);    
                drv.delay_ms(10);
                counterClock++;
            }
        }
        ausgabe(0b00000101, counterClock);
        drv.delay_ms(10);
        ausgabe(0b00000010, counterClock+1);
        drv.delay_ms(10);
        ausgabe(0b00000101, counterClock+2);
        drv.delay_ms(10);
    }while(false);
}

void inputSender() {

    std::string buffer;
    while (getline(std::cin, buffer)) {
        std::vector<uint8_t> paket;
        for (char c : buffer) {
            std::cout<<std::bitset<8>(c)<<'\n'<<'\n';
            std::cout<<"hallo";
            paket.push_back(c);
            if (paket.size() == 64) {
                sendBlock(paket);
                paket.clear();
            }
        }

        // Restdaten sind kleiner 64
        if (!paket.empty()) {
            //size_t position = paket.size();
            //paket.resize(64, 0); // Erweitern mit nullen für Block
            //sendBlock(paket, position);
            sendBlock(paket);
            break;
        }
    }
}

int main() {

    drv.setRegister (&DDRA,0x0f ) ; // Konfiguration PORTA: PIN 0-3 output  PIN 4-7 input
    drv.setRegister(&PORTA, 0x00);
    // Start sender and receiver threads
    std::thread senderThread(inputSender);
    //inputSender();
    std::thread receiverThread(outputReceiver);

    // Join threads
    senderThread.join();
    receiverThread.join();

    return 0;
}