#ifndef BOREALOS_RTC_H
#define BOREALOS_RTC_H

#include <Definitions.h>

#define RTC_IRQ 8

#define RTC_PORT 0x70
#define RTC_DATA_PORT 0x71

#define RTC_REGISTER_A 0x0A
#define RTC_REGISTER_B 0x0B
#define RTC_REGISTER_C 0x0C

#define RTC_TIME_SECONDS 0x00
#define RTC_TIME_MINUTES 0x02
#define RTC_TIME_HOURS 0x04
#define RTC_DAY_OF_MONTH 0x07
#define RTC_MONTH 0x08
#define RTC_YEAR 0x09
#define RTC_CURRENT_CENTURY 2000 // just change this every 100 years

#define RTC_DISABLE_NMI 0x80
#define RTC_ENABLE_NMI 0x7F
#define RTC_ENABLE_IRQS 0x40
#define RTC_UIP_FLAG 0x80
#define RTC_BCD_FLAG 0x04
#define RTC_24H_FLAG 0x02 // 24-hour mode if set, 12-hour mode if clear (i hate am/pm we WILL use 24-hour mode)

typedef struct RTCTimeState {
    uint8_t Seconds; // 0-59
    uint8_t Minutes; // 0-59
    uint8_t Hours; // 0-23
    uint8_t DayOfWeek; // 1-7
    uint8_t DayOfMonth; // 1-31
    uint8_t Month; // 1-12
    uint16_t Year; // Full year (up to the limit of uint16_t)
    bool IsInitialized;
} RTCTimeState;

extern RTCTimeState KernelRTCTime;

Status RTCInit();

#endif //BOREALOS_RTC_H