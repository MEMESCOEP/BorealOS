#include "Scheduler.h"
#include "Scheduler.h"

namespace Core::Time {
    Scheduler::Scheduler(TSC *tsc) : _tsc(tsc), _taskHeap(256) {

    }

    void Scheduler::ScheduleTask(TaskFunction function, void *context, uint64_t delayNs) {
        uint64_t currentNs = _tsc->GetNanoseconds();
        uint64_t endNs = currentNs + delayNs;

        Task newTask {
            .endTime = endNs,
            .function = function,
            .context = context
        };

        if (endNs < currentNs) {
            // The end time is in the past, so we should execute this task immediately. We can do this by setting endTime to 0, which will cause it to be executed on the next tick.
            newTask.endTime = 0;
        }

        InsertTask(newTask);
    }

    void Scheduler::Tick() {
        uint64_t currentNs = _tsc->GetNanoseconds();
        while (!_taskHeap.IsEmpty()) {
            Task* nextTask = _taskHeap.Peek();
            if (nextTask->endTime > currentNs) {
                // The next task is scheduled for the future, so we're done for this tick.
                break;
            }

            // The next task is scheduled for now or the past, so we should execute it.
            _taskHeap.Pop(); // Remove the task from the heap
            nextTask->function(nextTask->context);
        }
    }

    void Scheduler::InsertTask(Task &task) {
        if (!_taskHeap.Insert(task)) {
            PANIC("Scheduler task heap is full, cannot schedule more tasks!");
        }
    }
}
