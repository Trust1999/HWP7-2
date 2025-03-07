#pragma once

#include <iostream>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>

#include "ICommunicationInterface.hpp"

// SAVE WORKING: 35ms to 32ms
#define WRITE_DELAY 35

class ComInterface final : public ICommunicationInterface
{
	int serialPort{}; /**< The file descriptor for the interface port. */

public:
	/**
	 * @brief Writes a byte of data to the interface port.
	 * @param data The byte of data to write.
	 * @return The number of bytes written, or -1 on error.
	 */
	void writeByte(uint8_t data) override;

	/**
	 * @brief Reads a byte of data from the interface port.
	 * @return The byte of data read, or -1 on error.
	 */
	uint8_t readByte() override;

	/**
	 * @brief Sets up the interface port with the specified settings.
	 */
	void openCom() override;

	/**
	 * @brief Closes the interface port.
	 * @return 0 if the interface port was closed successfully, -1 otherwise.
	 */
	void closeCom() override;
};
