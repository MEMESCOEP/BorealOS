#include <Core/ProcessManager.h>
#include <Formats/ELF.h>

namespace Core {
    ProcessManager::ProcessManager() : _processes(256) {

    }

    ProcessManager::Process * ProcessManager::CreateProcess(const uint8_t *binaryData, size_t size) {
    }

    void ProcessManager::RunProcess(Process *process) {
    }

    void ProcessManager::BreakProcess(Process *process) {
    }

    void ProcessManager::ShutdownProcess(Process *process) {
    }

    void ProcessManager::KillProcess(Process *process) {

    }
}
