#ifndef BOREALOS_MODULE_H
#define BOREALOS_MODULE_H

// Static definition of a module.
#include <Definitions.h>
#include <Formats/DriverModule.h>

#define VERSION(major, minor, patch) \
    Formats::DriverModule::Version{major, minor, patch}

#define MODULE(name, description, version, importance) \
    extern "C" { MODULE_SECTION Formats::DriverModule::Module module {name, description, version, importance}; }

#define EXTERNAL_MODULE(name, version) \
    Formats::DriverModule::Reliance{name, version}

#define RELY_ON(...) \
    extern "C" { MODULE_RELIANCE_SECTION Formats::DriverModule::Reliance reliance[] = {__VA_ARGS__}; }

#define COMPATIBLE_FUNC() \
    extern "C" int MODULE_COMPATIBLE_FUNC_NAME()

#define LOAD_FUNC() \
    extern "C" STATUS _start()

#endif //BOREALOS_MODULE_H