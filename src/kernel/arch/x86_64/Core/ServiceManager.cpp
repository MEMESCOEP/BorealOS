#include "../../../Include/Core/ServiceManager.h"

#include "Kernel.h"
#include "../KernelData.h"

namespace Core {
    ServiceManager::ServiceManager() : _services(16) {
    }

    void ServiceManager::RegisterService(const char *name, void *address) {
        if (GetService(name) != nullptr) {
            LOG_ERROR("Service with name %s is already registered!", name);
            PANIC("Service is already registered");
        }

        _services.Add({name, address});
    }

    void* ServiceManager::GetService(const char *name) const {
        for (size_t i = 0; i < _services.Size(); i++) {
            if (strcmp(_services[i].name, name) == 0) {
                return _services[i].address;
            }
        }

        return nullptr; // Service not found
    }

    ServiceManager* ServiceManager::GetInstance() {
        return Kernel<KernelData>::GetInstance()->ArchitectureData->ServiceManager;
    }
}