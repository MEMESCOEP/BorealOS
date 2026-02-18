#ifndef BOREALOS_RTC_H
#define BOREALOS_RTC_H

#include <Definitions.h>
#include "../../Interrupts/IDT.h"

namespace Core::Time {
    class RTC {
    public:
        explicit RTC(Interrupts::IDT *idt);
        void Initialize();

        void BusyWait(size_t secondsToWait) const;

        void UpdateTicks();
        [[nodiscard]] uint64_t GetUnixTimestamp() const;
    private:
        struct TimeData {
            uint8_t second;
            uint8_t minute;
            uint8_t hour;
            uint8_t day;
            uint8_t month;
            uint32_t year;
        };

        static TimeData ReadFullCMOS();
        static uint64_t ConvertToTimestamp(const TimeData& data);
        static uint8_t ReadRegister(uint8_t reg);
        static uint16_t BCDToBinary(uint8_t bcd);

        Interrupts::IDT* _idt;
        volatile uint64_t _ticks = 0;
        uint64_t _baseTimestamp = 0;
        static constexpr uint32_t TicksPerSecond = 1024; // Default RTC periodic rate
        static constexpr uint8_t RTCInterruptVector = 8;
        static volatile bool _firstUpdate;
    };
}

#endif //BOREALOS_RTC_H
