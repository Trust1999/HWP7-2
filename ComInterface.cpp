#include "ComInterface.hpp"

void ComInterface::openCom()
{

	const auto portName = "/dev/ttyUSB0";
	serialPort = open(portName, O_RDWR | O_NOCTTY | O_NDELAY);

	if (serialPort < 0)
	{
		std::cerr << "Error opening serial port!";
		exit(1);
	}

	termios tty{};
	memset(&tty, 0, sizeof(tty));

	if (tcgetattr(serialPort, &tty) != 0)
	{
		std::cerr<< "Error getting port settings!";
		exit(1);
	}

	cfsetispeed(&tty, B57600);
	cfsetospeed(&tty, B57600);
	tty.c_cflag &= ~PARENB;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;
	tty.c_cflag |= CREAD | CLOCAL;
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);
	tty.c_oflag &= ~OPOST;
	tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	tcsetattr(serialPort, TCSANOW, &tty);

	// Eingabe- und Ausgabe-Buffer leeren
	if (tcflush(serialPort, TCIOFLUSH) == -1)
	{
		std::cerr << "Failed to flush buffers.";
	}
	else
	{
		std::cerr << "Serial buffers flushed successfully.";
	}

	std::cerr << "Serial port open!";
}

void ComInterface::closeCom()
{
	std::cerr << "Closing interface port...";
	close(serialPort);
}

void ComInterface::writeByte(const uint8_t data)
{
    ssize_t n = write(serialPort, &data, sizeof(data));
    if (n < 0)
    {
        throw std::runtime_error("Error writing to serial port!");
    }
}


uint8_t ComInterface::readByte()
{
    int bytesAvailable;
    if (ioctl(serialPort, FIONREAD, &bytesAvailable) == -1)
    {
        throw std::runtime_error("Error checking available data on serial port!");
    }

    if (bytesAvailable <= 0)
    {
        throw std::runtime_error("No data available to read from serial port!");
    }

    uint8_t data;
    ssize_t m = read(serialPort, &data, sizeof(data));
    if (m < 0)
    {
        throw std::runtime_error("Error reading from serial port!");
    }
    
    return data;
}

