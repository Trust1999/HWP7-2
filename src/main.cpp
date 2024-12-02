#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <bitset>
#include <optional>
#include <b15f/b15f.h>

//extern void inputSender();
//extern void outputReceiver();

B15F drv = B15F :: getInstance () ; //drv wird ein Objekt einer Klasse
std::mutex b15_mutex;
bool ende = true;
bool wechsel = false;


void melden(uint8_t a){
    b15_mutex.lock();    
    drv.setRegister(&PORTA, 0x0a);
    b15_mutex.unlock();
    //b15_mutex.unlock();
} 

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

std::vector<uint8_t> umwandeln(std::vector<uint8_t> &ed) {
    std::vector<uint8_t> paket;


    for (size_t i = 0; i < ed.size(); i += 3) {
        uint8_t speicherByte = 0;
        
        // Debug: Print the 3 bytes
        /*std::cout << "*********" << std::bitset<8>(ed.at(i)) << "*********";
        std::cout << "*********" << std::bitset<8>(ed.at(i + 1)) << "*********";
        std::cout << "*********" << std::bitset<8>(ed.at(i + 2)) << "*********";*/

        if (ed.at(i) & 0x04) {
            speicherByte |= (ed.at(i) << 6);
            speicherByte |= (ed.at(i + 1) << 3);
            speicherByte |= ed.at(i + 2);

            if (speicherByte == 0b00100100) {  // End condition
                ende = false;
                return paket;  // Early exit if end pattern is found
            }
        }
        else {
            speicherByte |= (ed.at(i) << 6);  // Take the 5 most significant bits from the first byte
            speicherByte |= (ed.at(i + 1) << 3);  // Take 5 more from the second byte
            speicherByte |= ed.at(i + 2);  // Take the last 5 bits from the third byte
        }

        //std::cout << "++++++" << std::bitset<8>(speicherByte) << "++++++";
        paket.push_back(speicherByte);  // Add the processed byte to the result
    }

    return paket;
}

uint8_t auslesen(){
    //std::lock_guard<std::mutex> lock(b15_mutex);
    b15_mutex.lock();
    uint8_t byte = (drv.getRegister(&PINA) & 0xF0) >> 4;
    b15_mutex.unlock();
    //std::cout<<std::bitset<8>(byte)<<"; ";
    return byte;   
}


// Function to receive a 64-byte block with 9-bit data format
std::vector<uint8_t> erhaltenesPacket() {

    std::vector<uint8_t> erhalteneDaten;
    std::vector<uint8_t> paket;
    bool schleife = true;

    uint8_t zwischenspeicher = 0x0f;

    while(zwischenspeicher != auslesen()){
        drv.delay_ms(1);
    }

    zwischenspeicher = auslesen();

    do{
        for(size_t i = 0; i < 195; i++){ // 64 - Daten + Check Summe

            while(zwischenspeicher == auslesen()){
                drv.delay_ms(1); //bei nicht schreiben hier erhöhen
            } 
            zwischenspeicher = auslesen();
            erhalteneDaten.push_back(zwischenspeicher&0x07); // Lese 3 bits vom Pin 4-6
            std::cerr<<i<<" ";
        }
        //std::cout<<"";
        paket = umwandeln(erhalteneDaten);

        uint8_t crc = paket.back();
        paket.pop_back();

        /*std::cerr<<"ttttttttt";
        for(char c: paket){
            std::cerr<<c;
        }
        std::cerr<<"ttttttttt";*/


        if(berechnungSummeZurueck(paket) == crc){
            //std::cerr<<"ja";
            melden('A');
            schleife = false;
        }
        else{
            melden('D');
            //std::cerr<<"nein";
            paket.clear();
        }
    }while(schleife);

    return paket;
}

void outputReceiver() {


    while (ende) {
        std::vector<uint8_t> block = erhaltenesPacket();
        if (!block.empty()) {
            std::string output(block.begin(), block.end());
            std::cout << output << std::flush; // schreiben zu STD:OUT
        }
    }
}

bool meldung(){
    //b15_mutex.lock();
    uint8_t byte = 0b00000000;

    //std::cerr<<"test";
    //for(size_t i; i <= 40; i++){
    while (true){
        //std::cerr<<"test";
        b15_mutex.lock();
        byte = (drv.getRegister(&PINA) & 0xF0) >> 4;
        b15_mutex.unlock();
        if(byte == 0x0A){
            //std::cerr<<"hi";
            //b15_mutex.unlock();
            return false;
        }
        else if(byte == 0x0D){
            //std::cerr<<"by";
            //b15_mutex.unlock();
            return true;
        }
        drv.delay_ms(1);
    }
    //b15_mutex.unlock();
    return true;
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
    //std::cerr<<".."<<std::bitset<8>(crc)<<"..";
    return crc;
}

void schreiben(uint8_t byte, uint8_t cC){
    //std::lock_guard<std::mutex> lock(b15_mutex);
    b15_mutex.lock();
    drv.setRegister(&PORTA, byte | (cC % 2 * 8));
    b15_mutex.unlock();
} 

void sendBlock(const std::vector<uint8_t> &paket, size_t position) {

    std::vector<uint8_t> block = paket;
    do{
        size_t counterClock = 0;

        for (size_t z = 0; z < block.size(); z++) {
            uint8_t maske = 0x00;
            uint8_t byte = block.at(z);

            if(z == position) maske = 0x04;

            for (int i = 6; i >= 0; i -= 3) {            // Übertragung 3-Bits
            uint8_t bitsToSend = (byte >> i) & 0x07 | maske; // Nimmt 3 bits
            //std::cout<<std::bitset<8>(bitsToSend | (counterClock % 2 * 8))<<"E, ";
            schreiben(bitsToSend, counterClock);    
            drv.delay_ms(10);
            std::cerr<<"-"<<counterClock<<"-";
            counterClock++;
            }
        }
    }while(meldung());
    drv.setRegister(&PORTA, 0x0f);
}

// Sendet das Datenpacket
void sendBlock(const std::vector<uint8_t> &paket) {
    
    std::vector<uint8_t> block = paket;    
    
    do{
        size_t counterClock = 0;

        for (uint8_t byte: block) {
            //uint8_t byte = block.at(z);
            for (int i = 6; i >= 0; i -= 3) {            // Übertragung 3-Bits
                uint8_t bitsToSend = (byte >> i) & 0x07; // Nimmt 3 bits
                //std::cout<<std::bitset<8>(bitsToSend | (counterClock % 2 * 8))<<"E, ";
                schreiben(bitsToSend, counterClock);    
                drv.delay_ms(10);
                std::cerr<<"-"<<counterClock<<"-";
                counterClock++;
            }
        }
    }while(meldung());
    drv.setRegister(&PORTA, 0x0f);

}

void inputSender() {

    std::string buffer;
    while (getline(std::cin, buffer)) {
        std::vector<uint8_t> paket;
        for (char c : buffer) {
            //std::cerr<<std::bitset<8>(c)<<'\n'<<'\n';
            paket.push_back(c);
            if (paket.size() == 64) {
                paket.push_back(berechnungSumme(paket));
                sendBlock(paket);
                paket.clear();
            }
        }

        // Restdaten sind kleiner 64
        if (!paket.empty()) {
            std::cerr<<"Fertig";
            paket.push_back(berechnungSumme(paket));
            size_t position = paket.size();
            paket.resize(65, 0); // Erweitern mit nullen für Block
            sendBlock(paket, position);
            //sendBlock(paket)
            break;
        }
    }
}

int main() {

    drv.setRegister (&DDRA,0x0f ); // Konfiguration PORTA: PIN 0-3 output  PIN 4-7 input
    drv.setRegister(&PORTA, 0x0f);
    // Start sender and receiver threads
    std::thread senderThread(inputSender);
    std::thread receiverThread(outputReceiver);

    //Join threads
    senderThread.join();
    receiverThread.join();

    return 0;
}