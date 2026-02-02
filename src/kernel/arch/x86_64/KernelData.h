#ifndef BOREALOS_KERNELDATA_H
#define BOREALOS_KERNELDATA_H

#include "IO/Serial.h"
#include "IO/SerialPort.h"

struct KernelData {
    IO::SerialPort SerialPort {IO::Serial::COM1};
};

#endif //BOREALOS_KERNELDATA_H