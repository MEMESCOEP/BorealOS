#ifndef BOREALOS_PROCESSMANAGER_H
#define BOREALOS_PROCESSMANAGER_H

#include <Definitions.h>

#include "../Formats/ELF.h"
#include "../Utility/List.h"
#include "Memory/Paging.h"

namespace Core {
    /// Load & execute processes, does not manage multiple processes, that's the task of the ProcessScheduler, which will call this class.
    class ProcessManager {
    public:
        ProcessManager();

        struct ProcessState {
            bool isRunning;
            bool isPaused;
            uintptr_t stackPointer;
            uintptr_t instructionPointer; // used to return to the correct instruction when resuming a paused process.
            Memory::Paging::PagingState* pagingState;
        };

        struct Process {
            uint64_t pid;
            uint64_t priority;
            Formats::ELF* binary;
            uintptr_t entryPoint;
            ProcessState state;
        };

        /// Create a process from the given ELF binary data. The process will not run, until you, or the scheduler, call RunProcess on it.
        Process *CreateProcess(const uint8_t* binaryData, size_t size);

        /// Run the given process, this will not return until the process exits or is killed. Interrupts are still enabled, so the process can be preempted by the scheduler.
        void RunProcess(Process* process);

        /// Pause the given process, this will allow it to be resumed later with RunProcess.
        void BreakProcess(Process* process);

        /// Ask the program to shut down, this will allow it to do any cleanup it needs to do before exiting. It will not be forced to exit.
        void ShutdownProcess(Process* process);

        /// Forcefully kill the given process, this will not allow it to do any cleanup, so it should only be used if the process is unresponsive.
        void KillProcess(Process* process);

    private:
        Utility::List<Process*> _processes;
    };
}

#endif //BOREALOS_PROCESSMANAGER_H
