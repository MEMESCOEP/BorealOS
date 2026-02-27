#ifndef BOREALOS_SERVICE_H
#define BOREALOS_SERVICE_H

#define TEMPLATE_MODULE_NAME "Template Service"
#define TEMPLATE_MODULE_DESCRIPTION "This is a template service! Copy and modify this to create your own service."
#define TEMPLATE_MODULE_VERSION VERSION(0,0,1)
#define TEMPLATE_MODULE_IMPORTANCE Formats::DriverModule::Importance::Required

#if HAS_SERVICE
#define TEMPLATE_SERVICE_NAME "template.service" // We define the service as a macro, so we can't make a mistake when registering/getting the service from the manager.

struct TemplateService {
    // Add API functions for your service here, and implement them in the corresponding .cpp file.
    // Use the ServiceManager::GetInstance()->RegisterService function to register your service's API functions with the service manager, and they will be available for other modules to use.
    // Using ServiceManager::GetService, other modules can get a pointer to your service's API functions and call them.
    // This *has* to be a struct, since drivers aren't linked with each-other, so a C++ class would fail, but a struct with function pointers works fine.
};
#endif // HAS_SERVICE

#endif //BOREALOS_SERVICE_H