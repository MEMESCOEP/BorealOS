#include "RTC.h"

#include <Core/Interrupts/IDT.h>
#include <Utility/SerialOperations.h>

RTCTimeState KernelRTCTime = {
    .Seconds = 0,
    .Minutes = 0,
    .Hours = 0,
    .DayOfWeek = 0,
    .DayOfMonth = 0,
    .Month = 0,
    .Year = 0,
    .IsInitialized = false
};

void clear_rtc_interrupt() {
    outb(RTC_PORT, RTC_REGISTER_C);
    inb(RTC_DATA_PORT); // Reading register C clears the interrupt
}

bool is_leap_year(uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

const uint8_t days_in_month[] = {
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

void increment_time(RTCTimeState * rtc_time_state) {
    // Increment seconds
    rtc_time_state->Seconds++;
    if (rtc_time_state->Seconds >= 60) {
        rtc_time_state->Seconds = 0;
        rtc_time_state->Minutes++;
        if (rtc_time_state->Minutes >= 60) {
            rtc_time_state->Minutes = 0;
            rtc_time_state->Hours++;
            if (rtc_time_state->Hours >= 24) {
                rtc_time_state->Hours = 0;
                rtc_time_state->DayOfMonth++;

                // God this is ugly but whatever
                uint8_t month_days = days_in_month[rtc_time_state->Month - 1];
                if (rtc_time_state->Month == 2 && is_leap_year(rtc_time_state->Year)) {
                    month_days = 29; // February in a leap year
                }
                if (rtc_time_state->DayOfMonth > month_days) {
                    rtc_time_state->DayOfMonth = 1;
                    rtc_time_state->Month++;
                    if (rtc_time_state->Month > 12) {
                        rtc_time_state->Month = 1;
                        rtc_time_state->Year++;
                    }
                }
            }
        }
    }
}

void rtc_interrupt(uint8_t vector, RegisterState* state) {
    (void)vector, (void)state;
    static uint32_t tick = 0;
    tick++;
    if (tick >= 1024) {
        // Roughly every second (assuming default rate of 1024Hz)
        tick = 0;
        // Update the RTC time
        increment_time(&KernelRTCTime);
    }

    clear_rtc_interrupt(); // Acknowledge the interrupt
}

static inline uint8_t cmos_read(uint8_t reg) {
    outb(RTC_PORT, reg | RTC_DISABLE_NMI);
    return inb(RTC_DATA_PORT);
}

static bool is_updating() {
    outb(RTC_PORT, RTC_REGISTER_A | RTC_DISABLE_NMI);
    return (inb(RTC_DATA_PORT) & RTC_UIP_FLAG) != 0; // Check the Update In Progress (UIP) flag
}

static uint8_t bcd_to_binary(uint8_t bcd) {
    return ((bcd / 16) * 10) + (bcd & 0x0F);
}

void read_rtc_time(uint8_t *sec, uint8_t *min, uint8_t *hour, uint8_t *day, uint8_t *month, uint16_t *year) {
    uint8_t sec_raw = cmos_read(RTC_TIME_SECONDS);
    uint8_t min_raw = cmos_read(RTC_TIME_MINUTES);
    uint8_t hour_raw = cmos_read(RTC_TIME_HOURS);
    uint8_t day_raw = cmos_read(RTC_DAY_OF_MONTH);
    uint8_t month_raw = cmos_read(RTC_MONTH);
    uint8_t year_raw = cmos_read(RTC_YEAR);
    uint16_t century = RTC_CURRENT_CENTURY; // just change this every 100 years, if you even use this OS in 21XX
    uint8_t register_b = cmos_read(RTC_REGISTER_B);

    if (!(register_b & RTC_BCD_FLAG)) {
        // Convert BCD to binary if necessary
        sec_raw = bcd_to_binary(sec_raw);
        min_raw = bcd_to_binary(min_raw);
        hour_raw = bcd_to_binary(hour_raw & 0x7F); // Mask out the 12/24 hour flag
        day_raw = bcd_to_binary(day_raw);
        month_raw = bcd_to_binary(month_raw);
        year_raw = bcd_to_binary(year_raw);
    }

    if (!(register_b & RTC_24H_FLAG) && (hour_raw & 0x80)) {
        // Convert 12-hour format to 24-hour format
        hour_raw = ((hour_raw & 0x7F) + 12) % 24;
    }

    *sec = sec_raw;
    *min = min_raw;
    *hour = hour_raw;
    *day = day_raw;
    *month = month_raw;
    *year = year_raw + century;
}

Status RTCInit() {
    IDTSetIRQHandler(RTC_IRQ, rtc_interrupt);

    // Disable interrupts while we configure the RTC
    ASM ("cli");

    while (is_updating()){} // Wait until an update is not in progress

    read_rtc_time(
        &KernelRTCTime.Seconds,
        &KernelRTCTime.Minutes,
        &KernelRTCTime.Hours,
        &KernelRTCTime.DayOfMonth,
        &KernelRTCTime.Month,
        &KernelRTCTime.Year
    );

    // Enable periodic interrupts
    outb(RTC_PORT, RTC_REGISTER_B | RTC_DISABLE_NMI);
    uint8_t prev = inb(RTC_DATA_PORT);
    outb(RTC_PORT, RTC_REGISTER_B | RTC_DISABLE_NMI);
    outb(RTC_DATA_PORT, prev | RTC_ENABLE_IRQS); // Set the IRQ enable bit
    clear_rtc_interrupt();
    // We are fine with the default rate of 1024Hz, because we don't use them for anything critical.
    // We only use them to keep the RTC time updated, and not for precise timing.
    // For anything critical we use the PIT or HPET timers.

    ASM ("sti");

    return STATUS_SUCCESS;
}
