#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <bitset>
#include <b15f/b15f.h>
//#include <chrono>

//extern void inputSender();
//extern void outputReceiver();

B15F drv = B15F :: getInstance () ; //drv wird ein Objekt einer Klasse
std::mutex b15_mutex;
bool ende = true;
bool wechsel = false;


uint8_t gerdehtesByte(uint8_t byte) {
    unsigned char gedreht = 0;
    // Iteriere über alle 8 Bits
    for (int i = 0; i < 8; ++i) {
        // Verschiebe das umgekehrte Byte nach links, um Platz für das neue Bit zu schaffen
        gedreht <<= 1;
        // Füge das niedrigstwertige Bit von `byte` hinzu
        gedreht |= (byte & 1);
        // Verschiebe `byte` nach rechts, um das nächste Bit zu erhalten
        byte >>= 1;
    }
    return gedreht >> 4;
}


void melden(uint8_t a){
    b15_mutex.lock();
    std::cerr<<"blockiert Lesen";    
    drv.setRegister(&PORTA, 0x0a);
    
    while (0x0F != ((drv.getRegister(&PINA) & 0xF0) >> 4))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    b15_mutex.unlock();
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

        if (ed.at(i) & 0x04) {
            speicherByte |= (ed.at(i) << 6);
            speicherByte |= (ed.at(i + 1) << 3);
            speicherByte |= ed.at(i + 2);

            if (speicherByte == 0b00100100) {  // End condition
                ende = false;
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

uint8_t auslesen(){
    //std::lock_guard<std::mutex> lock(b15_mutex);
    b15_mutex.lock();
    uint8_t byte = (drv.getRegister(&PINA) & 0xF0) >> 4;
    b15_mutex.unlock();    
    return byte;   
}


// Function to receive a 64-byte block with 9-bit data format
std::vector<uint8_t> erhaltenesPacket() {

    std::vector<uint8_t> erhalteneDaten;
    std::vector<uint8_t> paket;
    bool schleife = true;

    uint8_t zwischenspeicher = 0x0f;

   while(zwischenspeicher != auslesen()){
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    zwischenspeicher = auslesen();

    do{
        for(size_t i = 0; i < 195; i++){ // 64 - Daten + Check Summe
            auto start_time = std::chrono::steady_clock::now();

            while(zwischenspeicher == auslesen()){
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } 
            zwischenspeicher = auslesen();
            erhalteneDaten.push_back(zwischenspeicher&0x07); // Lese 3 bits vom Pin 4-6
            std::cerr<<"A"<<i<<" ";
        }

        paket = umwandeln(erhalteneDaten);

        uint8_t crc = paket.back();
        paket.pop_back();

        if(berechnungSummeZurueck(paket) == crc){
            melden(gerdehtesByte(0x0A));
            schleife = false;
        }
        else{
            melden(gerdehtesByte(0x0D));
            paket.clear();
        }
    }while(schleife);

    return paket;
}

void outputReceiver() {

    std::string output;  // Hier wird der Output außerhalb der Schleife gesammelt

    while (ende) {
        std::vector<uint8_t> block = erhaltenesPacket();
        if (!block.empty()) {
            output.append(block.begin(), block.end());  // Daten zum output hinzufügen
        }
    }
    // Nach der Schleife einmal alles ausgeben
    std::cout << output << std::flush;
}

bool meldung(){
    b15_mutex.lock();

    uint8_t byte = 0x00;
    std::cerr<<"blockiert Schreiben";
    while (true){
        byte = (drv.getRegister(&PINA) & 0xF0) >> 4;
        if(byte == 0x0A){
            std::cerr<<"hi";
            b15_mutex.unlock();
            return false;
        }
        else if(byte == 0x0D){
            std::cerr<<"by";
            b15_mutex.unlock();
            return true;
        }
        drv.delay_ms(1);
    }
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
    uint8_t senden = gerdehtesByte(byte | (cC % 2 * 8));
    b15_mutex.lock();
    drv.setRegister(&PORTA, senden);
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
            schreiben(bitsToSend, counterClock);    
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            std::cerr<<"E"<<counterClock<<" ";
            counterClock++;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }while(meldung());
    drv.setRegister(&PORTA, 0x0f);
}

void sendBlock(const std::vector<uint8_t> &paket) {
    
    std::vector<uint8_t> block = paket;    
    
    do{
        size_t counterClock = 0;

        for (uint8_t byte: block) {
            //uint8_t byte = block.at(z);
            for (int i = 6; i >= 0; i -= 3) {            // Übertragung 3-Bits
                uint8_t bitsToSend = (byte >> i) & 0x07; // Nimmt 3 bits
                schreiben(bitsToSend, counterClock);    
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                std::cerr<<"E"<<counterClock<<" ";
                counterClock++;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }while(meldung());
    drv.setRegister(&PORTA, 0x0f);

}


void inputSender() {

    std::vector<uint8_t> buffer(4096); // Temporärer Lesepuffer
    std::vector<uint8_t> paket;

    while (std::cin.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || std::cin.gcount() > 0) {
        size_t bytesRead = std::cin.gcount(); // Anzahl der tatsächlich gelesenen Bytes
        // Vergrößern des Buffers, wenn er voll ist
        if (buffer.size() < bytesRead) {
            buffer.reserve(buffer.size() + 2048); // Vergrößern des Buffers um 2048 Bytes
        }

        // Verarbeitung der gelesenen Daten
        for (size_t i = 0; i < bytesRead; ++i) {
            paket.push_back(buffer[i]);
            if (paket.size() == 64) {
                paket.push_back(berechnungSumme(paket));
                sendBlock(paket);
                paket.clear();
            }
        }
    }
    // Restdaten kleiner als 64 Bytes verarbeiten
    if (!paket.empty()) {
        std::cerr<<"letzter Block";
        paket.push_back(berechnungSumme(paket));
        size_t position = paket.size();
        paket.resize(65, 0); // Mit Nullen auffüllen
        sendBlock(paket, position);
    }
}

void synchronisation(){

    u_int8_t test = 0b00001010;
    drv.setRegister(&PORTA, 0x05); 
    while(test != auslesen()){
        std::cerr<<std::bitset<8>(auslesen())<<";";
        drv.delay_ms(1);
    }
    std::cerr<<"raus";
    drv.delay_ms(100);
    test = 0x0f;
    drv.setRegister(&PORTA, 0x0f);
    while(test != auslesen()){
        drv.delay_ms(1);
     }
}

int main() {

    drv.setRegister (&DDRA,0x0f ); // Konfiguration PORTA: PIN 0-3 output  PIN 4-7 input
    drv.setRegister(&PORTA, 0x00);

    synchronisation();
    // Start sender and receiver threads
    std::thread senderThread(inputSender);
    std::thread receiverThread(outputReceiver);

    //Join threads
    senderThread.join();
    receiverThread.join();

    return 0;
}