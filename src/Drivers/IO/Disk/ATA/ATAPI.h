#ifndef BOREALOS_ATAPI_H
#define BOREALOS_ATAPI_H

#include <Definitions.h>

typedef struct ATABlockDriverData ATABlockDriverData; // Forward declaration to avoid circular dependency
typedef struct BlockDevice BlockDevice;
Status ATAPIProbeDevice(uint16_t ioBase, uint8_t drive, ATABlockDriverData* blockDriverData, BlockDevice* outDevice);

#endif //BOREALOS_ATAPI_H