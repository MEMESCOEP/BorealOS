/* LIBRARIES */
#include <Utilities/StrUtils.h>
#include <Drivers/IO/Serial.h>
#include <Core/Graphics/Graphics.h>
#include <Core/Graphics/Console.h>
#include <Core/IO/RegisterIO.h>
#include <stdbool.h>


/* VARIABLES */
bool SerialExists = false;
int ActiveSerialPort = SERIAL_COM1;


/* FUNCTIONS */
void InitSerialPort(int COMPort)
{
    /*if (DoesSerialPortExist(COMPort) == false)
    {
        LOG_KERNEL_MSG("A non-existant serial port cannot be initialized!", WARN);
    }*/

    LOG_KERNEL_MSG("Initializing serial port...", INFO);
    ConsolePutString("\tSerial port to initialize is at address 0x");
    PrintNum(COMPort, 16);
    ConsolePutString(".\n\r");

    LOG_KERNEL_MSG("\tEnabling divisor mode and setting baud rate to 38400...", NONE);
    OutB(COMPort + 1, 0x00);
	OutB(COMPort + 3, 0x80); /* Enable divisor mode */
	OutB(COMPort + 0, 0x01); /* Div Low:  03 Set the port to 38400 bps */
	OutB(COMPort + 1, 0x00); /* Div High: 00 */
	OutB(COMPort + 3, 0x03);
	OutB(COMPort + 2, 0xC7);
	OutB(COMPort + 4, 0x0B);
    OutB(COMPort + 4, 0x1E);

    LOG_KERNEL_MSG("\tTesting serial port (TRX=0xAE)...", NONE);
    OutB(COMPort + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    int timeout = 1000;
    while (!(InB(COMPort + 5) & 1) && --timeout);

    int SerialResponse = InB(COMPort + 0);

    if(SerialResponse != 0xAE)
    {
        char SerialResponseBuffer[4];

        /*if (FBExists == false)
            return*/

        IntToStr(SerialResponse, SerialResponseBuffer, 16);
        LOG_KERNEL_MSG("Serial init failed due to a faulty/non-existing serial chip. Expected 0xAE but got 0x", WARN);
        ConsolePutString(SerialResponseBuffer);
        ConsolePutString("\n\r");
        return;
    }

    // If serial is not faulty, set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    LOG_KERNEL_MSG("\tSetting port to normal operation mode...", NONE);
    OutB(COMPort + 4, 0x0F);

    SerialExists = true;
    LOG_KERNEL_MSG("\tSerial port initialized.\n\r", NONE);
}

int DoesSerialPortExist(int COMPort)
{
    // Check if Line Status Register (LSR) indicates that the port is ready
    uint8_t lsr = InB(COMPort + 0x05);

    // If bit 0 (LSR0) is set, the transmitter is ready to send data
    if (lsr & 0x01)
    {
        return 1;  // Serial port exists and is ready
    }

    return 0;  // Serial port does not exist or is not ready
}

int SerialTransmitEmpty(int COMPort)
{
    return InB(COMPort + 5) & 0x20;
}

void SendCharSerial(int COMPort, char Char)
{
    if (SerialExists == false)
    {
        return;
    }

    while (SerialTransmitEmpty(COMPort) == 0);

    OutB(COMPort, Char);
}

void SendStringSerial(int COMPort, char* Message)
{
    for (int i = 0; Message[i] != '\0'; i++)
    {
        SendCharSerial(COMPort, Message[i]);
    }
}