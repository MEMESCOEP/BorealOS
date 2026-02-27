#ifndef BOREALOS_SERVICEMANAGER_H
#define BOREALOS_SERVICEMANAGER_H

#include <Definitions.h>
#include <Utility/List.h>

namespace Core {
    class ServiceManager {
    public:
        ServiceManager();

        void RegisterService(const char* name, void* address);
        void* GetService(const char* name) const;

        static ServiceManager *GetInstance();

    private:
        struct Service {
            const char* name;
            void* address;
        };

        Utility::List<Service> _services;
    };
}

#endif //BOREALOS_SERVICEMANAGER_H
