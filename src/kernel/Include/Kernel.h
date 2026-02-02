#ifndef BOREALOS_KERNEL_H
#define BOREALOS_KERNEL_H

#include <Definitions.h>

template<typename T>
class Kernel {
public:
    void Initialize();
    void Start();

    void Log(const char* message);
    [[noreturn]] void Panic(const char* message);

    static Kernel* GetInstance();
    T* ArchitectureData;
};

#endif //BOREALOS_KERNEL_H