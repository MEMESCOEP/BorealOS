#ifndef BOREALOS_SCHEDULER_H
#define BOREALOS_SCHEDULER_H

#include <Definitions.h>
#include "TSC.h"

namespace Core::Time {
    class Scheduler {
    public:
        typedef void (*TaskFunction)(void* context);

        explicit Scheduler(TSC *tsc);

        void ScheduleTask(TaskFunction function, void* context, uint64_t delayNs);
        void Tick();
    private:
        TSC *_tsc;

        struct Task {
            uint64_t endTime; // In nanoseconds.
            TaskFunction function;
            void* context;
        };

        /// Basic min-heap implementation for storing scheduled tasks, sorted by endTime. The task with the earliest endTime is at the top of the heap.
        class TaskHeap {
        public:
            TaskHeap(size_t capacity) : _size(0), _capacity(capacity) {
                _data = new Task[_capacity];
            }

            ~TaskHeap() {
                delete[] _data;
            }

            bool Insert(Task &task) {
                if (_size >= _capacity) {
                    return false; // Heap is full
                }

                // Insert the new task at the end of the heap and then bubble it up to maintain the min-heap property.
                size_t i = _size++;
                while (i > 0) {
                    size_t parent = (i - 1) / 2;
                    if (_data[parent].endTime <= task.endTime) break; // Parent is smaller or equal, we're done
                    _data[i] = _data[parent];
                    i = parent;
                }
                _data[i] = task;
                return true;
            }

            [[nodiscard]] Task* Peek() const {
                if (_size == 0) return nullptr;
                return &_data[0];
            }

            Task* Pop() {
                if (_size == 0) return nullptr;

                Task& root = _data[0];
                Task& last = _data[--_size];

                // Bubble down
                size_t i = 0;
                while (i * 2 + 1 < _size) {
                    size_t child = i * 2 + 1;
                    if (child + 1 < _size && _data[child + 1].endTime < _data[child].endTime) {
                        child++;
                    }
                    if (last.endTime <= _data[child].endTime) break;
                    _data[i] = _data[child];
                    i = child;
                }
                _data[i] = last;
                return &root;
            }

            [[nodiscard]] bool IsEmpty() const { return _size == 0; }

            [[nodiscard]] size_t Size() const { return _size; }
            [[nodiscard]] size_t Capacity() const { return _capacity; }

        private:
            Task* _data;
            size_t _size;
            size_t _capacity;
        };

        TaskHeap _taskHeap;
        void InsertTask(Task& task);
    };
}

#endif //BOREALOS_SCHEDULER_H
