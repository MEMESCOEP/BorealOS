#ifndef BOREALOS_PROCESSMANAGER_H
#define BOREALOS_PROCESSMANAGER_H

#include <Definitions.h>

#include "../Formats/ELF.h"

namespace Core {
    /// Load & execute processes, does not manage multiple processes, that's the task of the ProcessScheduler, which will call this class.
    class ProcessManager {
    public:
        ProcessManager() = default;

        struct Process {
            uint64_t pid;
            uint64_t priority;
            Formats::ELF* binary;
            uintptr_t entryPoint;
            bool isRunning;
        };

        /// Create a process from the given ELF binary data. The process will not run, until you, or the scheduler, call RunProcess on it.
        Process *CreateProcess(const uint8_t* binaryData, size_t size);
        void RunProcess(Process* process);
        void BreakProcess(Process* process);
        void ShutdownProcess(Process* process);
        void KillProcess(Process* process);
    };
}

#endif //BOREALOS_PROCESSMANAGER_H
