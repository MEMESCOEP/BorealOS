#ifndef BOREALOS_SERIALPORT_H
#define BOREALOS_SERIALPORT_H

#include <Definitions.h>
namespace IO {
    class SerialPort {
    public:
        SerialPort(uint16_t port);

        void Initialize();
        void WriteChar(char c) const;
        void WriteString(const char* str) const;
        [[nodiscard]] bool IsInitialized() const;

    private:
        uint16_t _port;
        bool _initialized;
    };
}

#endif //BOREALOS_SERIALPORT_H