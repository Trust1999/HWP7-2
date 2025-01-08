#pragma once

#include <cstdint>

class ICommunicationInterface
{
	
	public:
	virtual ~ICommunicationInterface() = default;

	virtual void writeByte(uint8_t data) = 0;

	virtual uint8_t readByte() = 0;

	virtual void openCom() = 0;

	virtual void closeCom() = 0;
};
