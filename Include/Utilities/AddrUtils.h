#ifndef ADDRUTILS_H
#define ADDRUTILS_H

/* LIBRARIES*/
#include <stdint.h>

/* DEFINITIONS */
typedef struct
{
  uint8_t AddressSpace;
  uint8_t BitWidth;
  uint8_t BitOffset;
  uint8_t AccessSize;
  uint64_t Address;
} GenericAddressStructure;

#endif