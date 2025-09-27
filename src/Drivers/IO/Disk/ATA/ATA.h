#ifndef BOREALOS_ATA_H
#define BOREALOS_ATA_H

#include <Definitions.h>

typedef struct ATABlockDriverData ATABlockDriverData; // Forward declaration to avoid circular dependency
typedef struct BlockDevice BlockDevice;
Status ATAProbeDevice(uint16_t ioBase, uint8_t drive, ATABlockDriverData* blockDriverData, BlockDevice* outDevice);

#endif //BOREALOS_ATA_H