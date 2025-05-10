/* LIBRARIES */
#include <stdbool.h>
#include "Utilities/StrUtils.h"
#include "Core/Graphics/Terminal.h"
#include "Core/Graphics/Graphics.h"
#include "Core/IO/RegisterIO.h"
#include "Serial.h"


/* VARIABLES */
bool SerialExists = false;
int ActiveSerialPort = SERIAL_COM1;


/* FUNCTIONS */
void InitSerialPort(int COMPort)
{
    OutB(COMPort + 1, 0x00);
	OutB(COMPort + 3, 0x80); /* Enable divisor mode */
	OutB(COMPort + 0, 0x01); /* Div Low:  03 Set the port to 38400 bps */
	OutB(COMPort + 1, 0x00); /* Div High: 00 */
	OutB(COMPort + 3, 0x03);
	OutB(COMPort + 2, 0xC7);
	OutB(COMPort + 4, 0x0B);
    OutB(COMPort + 4, 0x1E);
    OutB(COMPort + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    int SerialResponse = InB(COMPort + 0);

    if(SerialResponse != 0xAE && SerialResponse != 0x00)
    {
        char SerialResponseBuffer[4];

        if (FBExists == false)
            return

        IntToStr(SerialResponse, SerialResponseBuffer, 16);
        TerminalDrawMessage("Serial init failed due to a faulty/non-existing serial chip. Expected 0xAE but got 0x", WARNING);
        TerminalDrawString(SerialResponseBuffer);
        TerminalDrawString("\n\r");
        return;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    OutB(COMPort + 4, 0x0F);

    SerialExists = true;
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