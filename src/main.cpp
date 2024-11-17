#include <iostream>
#include <thread>
#include <b15f/b15f.h>


int main() {
    // Initialize B15F
    B15F &drv = B15F::getInstance();
    drv.setRegister(&DDRA, 0x0F); // Configure PORTA: lines 0-4 as output/input

    // Start sender and receiver threads
    std::thread senderThread(inputSender, ref(drv));
    std::thread receiverThread(outputReceiver, ref(drv));

    // Join threads
    senderThread.join();
    receiverThread.join();

    return 0;
}