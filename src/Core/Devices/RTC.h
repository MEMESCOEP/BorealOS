#ifndef RTC_H
#define RTC_H

/* DEFINITIONS */
#define RTC_COMMAND_REG
#define RTC_DATA_REG


/* FUNCTIONS */
void WriteRTC();
void SelectCMOSRegister(int CMOSRegister, bool EnableNMI);
int ReadRTC();

#endif