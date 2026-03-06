#ifndef BOREALOS_SERVICE_H
#define BOREALOS_SERVICE_H

#define PS2_MODULE_NAME "PS/2 HID Service"
#define PS2_MODULE_DESCRIPTION "Configures PS/2 keyboards and mice for HID use"
#define PS2_MODULE_VERSION VERSION(0,0,1)
#define PS2_MODULE_IMPORTANCE Formats::DriverModule::Importance::Optional

#if HAS_SERVICE
#define PS2_SERVICE_NAME "ps2.service" // We define the service as a macro, so we can't make a mistake when registering/getting the service from the manager.

struct TemplateService {
    // Add API functions for your service here, and implement them in the corresponding .cpp file.
    // Use the ServiceManager::GetInstance()->RegisterService function to register your service's API functions with the service manager, and they will be available for other modules to use.
    // Using ServiceManager::GetService, other modules can get a pointer to your service's API functions and call them.
    // This *has* to be a struct, since drivers aren't linked with each-other, so a C++ class would fail, but a struct with function pointers works fine.
};
#endif // HAS_SERVICE

#endif //BOREALOS_SERVICE_H