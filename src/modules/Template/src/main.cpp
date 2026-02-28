#include <Module.h>

#define HAS_SERVICE 0 // Set this to 1 if your module provides a service for other modules to use.
#include "../Service.h"

MODULE(TEMPLATE_MODULE_NAME, TEMPLATE_MODULE_DESCRIPTION, TEMPLATE_MODULE_VERSION, TEMPLATE_MODULE_IMPORTANCE);

// RELY_ON(EXTERNAL_MODULE("Some other module", VERSION(0,0,1)));

// It is safe to call any kernel function wherever!

COMPATIBLE_FUNC() {
    // Here you should check if the current system configuration is compatible with your module.
    // For example, if your module is an AMD GPU driver, you should check if the system has an AMD GPU before returning true. If the module is not compatible, return false and it will not be loaded.
    // **THIS IS UNORDERED, SO DON'T RELY ON ANY OTHER MODULES OR SERVICES BEING AVAILABLE WHEN THIS FUNCTION RUNS!**

    return true;
}

LOAD_FUNC() {
    // This function is called when the module is loaded according to the dependencies and compatibility.
    // Here you should put initialization code, and register any services your module provides with the service manager so that other modules can use them.

    return STATUS::SUCCESS;
}