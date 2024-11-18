#include <iostream>
#include <thread>
#include <b15f/b15f.h>
#include "senden.h"
#include "empfangen.h"

extern void inputSender();
//extern void outputReceiver(B15F &drv);

int main() {
    // Initialize B15F
    B15F & drv = B15F :: getInstance () ; //drv wird ein Objekt einer Klasse
    drv.setRegister (& DDRA , 0x0f ) ; // Configure PORTA: lines 0-4 as output/input

    // Start sender and receiver threads
    std::thread senderThread(inputSender);
    //std::thread receiverThread(outputReceiver, std::ref(drv));

    // Join threads
    senderThread.join();
    //receiverThread.join();

    return 0;
}