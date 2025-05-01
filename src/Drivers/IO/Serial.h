#ifndef SERIAL_H
#define SERIAL_H

/* DEFINITIONS */
#define SERIAL_COM1 0x3F8
#define SERIAL_COM2 0x2F8
#define SERIAL_COM3 0x3E8
#define SERIAL_COM4 0x2E8
#define SERIAL_COM5 0x5F8
#define SERIAL_COM6 0x4F8
#define SERIAL_COM7 0x5E8
#define SERIAL_COM8 0x4E8 


/* VARIABLES */
extern bool SerialExists;
extern int ActiveSerialPort;


/* FUNCTIONS */
void InitSerialPort(int COMPort);
void SendCharSerial(int COMPort, char Char);
void SendStringSerial(int COMPort, char* Message);

#endif