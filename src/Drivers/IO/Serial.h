#ifndef BOREALOS_SERIAL_H
#define BOREALOS_SERIAL_H

#include <Definitions.h>

#define SERIAL_COM1 0x3F8
#define SERIAL_COM2 0x2F8
#define SERIAL_COM3 0x3E8
#define SERIAL_COM4 0x2E8
#define SERIAL_COM5 0x5F8
#define SERIAL_COM6 0x4F8
#define SERIAL_COM7 0x5E8
#define SERIAL_COM8 0x4E8

typedef struct SerialPort {
    uint16_t BasePort;
    bool Initialized;
} SerialPort;

Status SerialLoad(uint16_t BasePort, SerialPort *port);

Status SerialWriteChar(const SerialPort *port, char c);
Status SerialWriteString(const SerialPort *port, const char *str);
Status SerialReadChar(const SerialPort *port, char *outChar);

#endif //BOREALOS_SERIAL_H