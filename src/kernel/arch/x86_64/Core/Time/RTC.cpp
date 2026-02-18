#include "RTC.h"

#include "Kernel.h"
#include "../../IO/Serial.h"
#include "../../KernelData.h"

void RTCInterruptHandler() {
    IO::Serial::outb(0x70, 0x0C);
    IO::Serial::inb(0x71);

    auto* rtc = &Kernel<KernelData>::GetInstance()->ArchitectureData->Rtc;
    rtc->UpdateTicks();
}

namespace Core::Time {
    RTC::RTC(Interrupts::IDT *idt) : _idt(idt), _ticks(0) {

    }

    void RTC::Initialize()  {
        _idt->RegisterIRQHandler(RTCInterruptVector, RTCInterruptHandler);
        _idt->ClearIRQMask(RTCInterruptVector);

        // Capture initial time once
        TimeData initialTime = ReadFullCMOS();
        _baseTimestamp = ConvertToTimestamp(initialTime);

        asm volatile ("cli");
        IO::Serial::outb(0x70, 0x8B);
        uint8_t prev = IO::Serial::inb(0x71);
        IO::Serial::outb(0x70, 0x8B);
        // Set periodic interrupt (bit 6) and ensure rate is default (lower 4 bits)
        IO::Serial::outb(0x71, (prev & 0xF0) | 0x40 | 0x06);
        asm volatile ("sti");

        LOG_DEBUG("Current time: %u32/%u32/%u32 %u32:%u32:%u32.",
                  initialTime.day, initialTime.month, initialTime.year,
                  initialTime.hour, initialTime.minute, initialTime.second);
    }

    void RTC::BusyWait(size_t secondsToWait) const {
        uint64_t startSeconds = _ticks / TicksPerSecond;
        while ((_ticks / TicksPerSecond) < startSeconds + secondsToWait) {
            asm volatile ("hlt");
        }
    }

    void RTC::UpdateTicks() {
        _ticks += 1;
    }

    uint64_t RTC::GetUnixTimestamp() const {
        return _baseTimestamp + (_ticks / TicksPerSecond);
    }

    RTC::TimeData RTC::ReadFullCMOS() {
        IO::Serial::outb(0x70, 0x0A | 0x80);
        while (IO::Serial::inb(0x71) & 0x80) {
            // Wait until RTC is not updating
            asm volatile ("hlt");
        }

        uint8_t seconds = BCDToBinary(ReadRegister(0x00));
        uint8_t minutes = BCDToBinary(ReadRegister(0x02));
        uint8_t hours = BCDToBinary(ReadRegister(0x04));
        uint8_t dayOfMonth = BCDToBinary(ReadRegister(0x07));
        uint8_t month = BCDToBinary(ReadRegister(0x08));
        uint16_t year = BCDToBinary(ReadRegister(0x09)) + 2000; // im 99% sure this code is being run in the 2000's.

        return {seconds, minutes, hours, dayOfMonth, month, year};
    }

    uint64_t RTC::ConvertToTimestamp(const TimeData &data) {
        const uint8_t daysInMonth[] = {
            31, // January
            28, // February (29 in leap years)
            31, // March
            30, // April
            31, // May
            30, // June
            31, // July
            31, // August
            30, // September
            31, // October
            30, // November
            31  // December
        };

        auto isLeapYear = [](uint32_t year) {
            return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        };

        uint64_t timestamp = 0;
        for (uint32_t y = 1970; y < data.year; y++) {
            timestamp += 365 * 24 * 3600;
            if (isLeapYear(y)) {
                timestamp += 24 * 3600; // Add one day for leap years
            }
        }

        for (uint8_t m = 1; m < data.month; m++) {
            timestamp += daysInMonth[m - 1] * 24 * 3600;
            if (m == 2 && isLeapYear(data.year)) {
                timestamp += 24 * 3600; // Add one day for leap years in February
            }
        }

        timestamp += (data.day - 1) * 24 * 3600;
        timestamp += data.hour * 3600;
        timestamp += data.minute * 60;
        timestamp += data.second;

        return timestamp;
    }

    uint8_t RTC::ReadRegister(uint8_t reg) {
        IO::Serial::outb(0x70, reg);
        return IO::Serial::inb(0x71);
    }

    uint16_t RTC::BCDToBinary(uint8_t bcd) {
        return ((bcd / 16) * 10) + (bcd % 16);
    }
}