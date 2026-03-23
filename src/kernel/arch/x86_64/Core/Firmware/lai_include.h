#ifndef BOREALOS_LAI_INCLUDE_H
#define BOREALOS_LAI_INCLUDE_H

void laihost_init();

extern "C" {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <lai/core.h>
#include <lai/helpers/sci.h>
#include <lai/helpers/pm.h>
#pragma GCC diagnostic pop
}

#endif //BOREALOS_LAI_INCLUDE_H