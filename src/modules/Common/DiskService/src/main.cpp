#include <Module.h>
#include <Core/ServiceManager.h>

#include "../Service.h"
#include "DiskTracker.h"

MODULE(DISK_MODULE_NAME, DISK_MODULE_DESCRIPTION, DISK_MODULE_VERSION, DISK_MODULE_IMPORTANCE);

COMPATIBLE_FUNC() {
    // The disk service is compatible with all systems, since it is abstract.
    return true;
}

LOAD_FUNC() {
    auto tracker = new DiskTracker();
    Core::ServiceManager::GetInstance()->RegisterService(DISK_SERVICE_NAME, tracker->GetService());
    return STATUS::SUCCESS;
}