#include <iostream>
#include <vector>
#include <bitset>
#include <b15f/b15f.h>
#include <ctime>

B15F drv = B15F :: getInstance ();

bool ende;
bool fertig_lesen;
std::string output;
uint8_t zwischenspeicher;
std::time_t start;
std::time_t zende;
int anzahlBlock;

std::vector<uint8_t> umwandeln(std::vector<uint8_t> &ed) {
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

uint8_t auslesen(){
    uint8_t byte = (drv.getRegister(&PINA) & 0xF0) >> 4;
    return byte;   
}

uint8_t lesen(){
    size_t i = 0;

    while(zwischenspeicher == auslesen() && i < 4)
    {
        drv.delay_ms(1);
        i++;
    }
    zwischenspeicher = auslesen();
    std::cerr<<"L "<<std::bitset<8>(zwischenspeicher)<<" ";
    return (auslesen()&0x07);
}

uint8_t gerdehtesByte(uint8_t byte) {
    unsigned char gedreht = 0;
    // drehen alle 8 Bits
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

void schreiben(uint8_t byte, uint8_t cC){
    uint8_t senden = gerdehtesByte(byte | (cC % 2 * 8));
    std::cerr<<"S "<<std::bitset<8>(byte | (cC % 2 * 8))<<" ";
    drv.setRegister(&PORTA, senden);
}

uint8_t berechnungSumme(std::vector<uint8_t> data) {
    std::cerr <<data.size()<<"----";
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
    std::cerr<<"CRC: "<<std::bitset<8>(crc)<<" = " << crc;
    return crc;
}

void speichern(const std::vector<uint8_t> &blockDaten){
    output.append(blockDaten.begin(), blockDaten.end());
    std::cout << output << std::flush;
    output.clear();
}

void synchronisation() {
    uint8_t expected_value;

    // Setze PORTA auf 0x00 und warte, bis das Ergebnis übereinstimmt
    drv.setRegister(&PORTA, 0x00); 
    expected_value = 0x00;
    while (expected_value != auslesen()) {
        drv.delay_ms(1);
    }

    // Kurze Verzögerung (50ms)
    drv.delay_ms(50);

    // Setze PORTA auf 0x0F und warte, bis das Ergebnis übereinstimmt
    drv.setRegister(&PORTA, 0x0F); 
    expected_value = 0x0F;
    while (expected_value != auslesen()) {
        drv.delay_ms(1);
    }

    // Weitere Verzögerung von 50ms
    drv.delay_ms(50);
}


bool pruefen(std::vector<uint8_t> &blockDaten){

    synchronisation();
    synchronisation();

    if(blockDaten.empty()){
        schreiben(0x0C, 0);
        std::cerr<<"achtung";
        drv.delay_ms(500);
        return false;
    }

    uint8_t crc = blockDaten.back();
    std::cerr<<"CRC angekommen"<<crc<<" ";
    blockDaten.pop_back();

    //ist lesen schon fertig
    if(fertig_lesen){
        schreiben(0x0C, 0);
        blockDaten.clear();
    }
    else{
        //lesen Test 
        if(berechnungSumme(blockDaten) == crc){
            std::cerr<<"richtig ";
            schreiben(0x0C, 0);
            speichern(blockDaten);
        }
        else{
            schreiben(0x0D, 0);
            ende = false;
            std::cerr<<"falsch ";
        }
    }

    drv.delay_ms(500);

    uint8_t test = 0x00;
    //schreib Test
    while(true){
        if(test == 0x0D){
                std::cerr<<"Falsches CRC";
                zwischenspeicher = 0x00;
                return true;
            }
            else if(test == 0x0C){
                std::cerr<<"Richtiges CRC";
                zwischenspeicher = 0x00;
                return false;
            }
        test = auslesen();
        std::cerr<<std::bitset<8>(test);
    }
    return true;
}

void weiterLesen(){
    std::vector<uint8_t> erhalteneDaten;
    bool erneutSenden = false;
    std::cerr<<"weiterlesen";

    synchronisation();

    for(size_t i = 0; i < 195; i++){
        schreiben(0x00, i);
        std::cerr<<i<<" ";
        erhalteneDaten.push_back(lesen());
    }

    std::vector<uint8_t> blockDaten  = umwandeln(erhalteneDaten);
    erneutSenden = pruefen(blockDaten);
}


void sendBlock(const std::vector<uint8_t> &paket, size_t position) {

    std::vector<uint8_t> block = paket;
    std::vector<uint8_t> erhalteneDaten;
    bool erneutSenden = true;
    do{
        synchronisation();

        
        schreiben(0x00, 0);
        erhalteneDaten.clear();
        size_t counterClock = 0;

        for (size_t z = 0; z < block.size(); z++) {
            uint8_t maske = 0x00;
            uint8_t byte = block.at(z);

            if(z == position) maske = 0x04;

            for (int i = 6; i >= 0; i -= 3) {            // Übertragung 3-Bits
                uint8_t bitsToSend = (byte >> i) & 0x07 | maske; // Nimmt 3 bits
                std::cerr<<counterClock<<" ";
                schreiben(bitsToSend, counterClock);    
                erhalteneDaten.push_back(lesen());
                counterClock++;
            }
        }

        std::vector<uint8_t> blockDaten  = umwandeln(erhalteneDaten);
        erneutSenden = pruefen(blockDaten);
        drv.delay_ms(100);
    }while(erneutSenden);
     std::cerr<<"____________________________________________________________";
    fertig_lesen = ende;
}

void sendBlock(const std::vector<uint8_t> &paket) {

    std::vector<uint8_t> block = paket;
    std::vector<uint8_t> erhalteneDaten;
    bool erneutSenden = true;  

    do{
        synchronisation();

        schreiben(0x00, 0);
        erhalteneDaten.clear();
        size_t counterClock = 0;

        for (uint8_t byte: block) {
            for (int i = 6; i >= 0; i -= 3) {            // Übertragung 3-Bits
                uint8_t bitsToSend = (byte >> i) & 0x07; // Nimmt 3 bits
                std::cerr<<counterClock<<" ";
                schreiben(bitsToSend, counterClock);   
                erhalteneDaten.push_back(lesen());
                counterClock++;
            }
        }

        std::vector<uint8_t> blockDaten  = umwandeln(erhalteneDaten);
        erneutSenden = pruefen(blockDaten);
        drv.delay_ms(100);
    }while(erneutSenden);
    std::cerr<<"____________________________________________________________";
    fertig_lesen = ende;
}

void startSenden() {
    
    std::vector<uint8_t> buffer(4096); // Temporärer Lesepuffer
    std::vector<uint8_t> paket;

    size_t paketSize = 0;
    while (std::cin.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || std::cin.gcount() > 0) {
        size_t bytesRead = std::cin.gcount(); // Anzahl der tatsächlich gelesenen Bytes
        // Vergrößern des Buffers, wenn er voll ist
        if (buffer.size() < bytesRead) {
        buffer.reserve(buffer.size() + 2048); // Vergrößern des Buffers um 2048 Bytes
        }

        // Verarbeitung der gelesenen Daten
        for (size_t i = 0; i < bytesRead; ++i) {
            paket.push_back(buffer[i]);
            paketSize++;
            if (paketSize == 64) {
                paket.push_back(berechnungSumme(paket));
                sendBlock(paket);
                paket.clear();
                paketSize = 0;
                anzahlBlock++;
            }
        }
    }

    anzahlBlock++;
    // Restdaten kleiner als 64 Bytes verarbeiten
    if (!paket.empty()) {
        paket.push_back(berechnungSumme(paket));
        size_t position = paket.size();
        std::cerr<<"fast leerer Block";
        paket.resize(65, 0); // Mit Nullen auffüllen
        sendBlock(paket, position);
    }
    else{
        std::cerr<<"leerer Block";
        paket.resize(65, 0); // Mit Nullen auffüllen
        sendBlock(paket, 0);
    }
    std::cerr<<"fertig";
    zende = std::time(nullptr);
}


int main() {

    drv.setRegister (&DDRA,0x0f ); // Konfiguration PORTA: PIN 0-3 output  PIN 4-7 input
    drv.setRegister(&PORTA, 0x00); 
    ende = false;
    fertig_lesen = false;
    zwischenspeicher = 0x0f;
    anzahlBlock = 0;

    synchronisation();
    start = std::time(nullptr);

    startSenden();

    while(!ende){
        weiterLesen();  
    }

    int diff = zende - start;
    std::cerr<<zende<<" ,"<<start<<", "<<diff;
    double geschwindigkeit = anzahlBlock * 8 * 64/ diff;
    std::cerr <<"Übertragungsgeschwindigkeit: "<< geschwindigkeit <<"bit/s";
    return 0;
}