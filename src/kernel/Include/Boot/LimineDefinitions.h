#ifndef BOREALOS_LIMINEDEFINITIONS_H
#define BOREALOS_LIMINEDEFINITIONS_H

#include <Definitions.h>
#include "Boot/c_limine.h"

extern volatile uint64_t limine_base_revision[];
extern volatile struct limine_framebuffer_request framebuffer_request;
extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_module_request module_request;
extern volatile struct limine_rsdp_request rsdp_request;
extern volatile uint64_t limine_requests_start_marker[];
extern volatile uint64_t limine_requests_end_marker[];

#endif //BOREALOS_LIMINEDEFINITIONS_H