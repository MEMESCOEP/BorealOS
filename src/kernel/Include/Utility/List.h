#ifndef BOREALOS_LIST_H
#define BOREALOS_LIST_H

#include <Definitions.h>

namespace Utility {
    /// Basic dynamic array implementation.
    template<typename T>
    class List {
    public:
        List(size_t capacity = 16) : _size(0), _capacity(capacity) {
            _data = new T[_capacity];
        }
        ~List() {
            delete[] _data;
        }
        List(const List&) = delete;
        List& operator=(const List&) = delete;

        void Add(const T& item) {
            if (_size >= _capacity) {
                Resize(_capacity * 2);
            }
            _data[_size++] = item;
        }

        void Remove(size_t index) {
            if (index >= _size) {
                return; // Out of bounds
            }
            for (size_t i = index; i < _size - 1; i++) {
                _data[i] = _data[i + 1];
            }
            _size--;
        }

        T& operator[](size_t index) {
            if (index >= _size) {
                // Out of bounds, since this is a kernel, we should panic since this might be a vulnerability if we just return a reference to some random memory.
                PANIC("List index out of bounds!");
            }

            return _data[index];
        }

        const T& operator[](size_t index) const {
            if (index >= _size) {
                // Out of bounds, since this is a kernel, we should panic since this might be a vulnerability if we just return a reference to some random memory.
                PANIC("List index out of bounds!");
            }

            return _data[index];
        }

        [[nodiscard]] size_t Size() const { return _size; }
        [[nodiscard]] size_t Capacity() const { return _capacity; }

    private:
        void Resize(size_t newCapacity) {
            auto newData = new T[newCapacity];
            memcpy(newData, _data, sizeof(T) * _size);
            delete[] _data;
            _data = newData;
            _capacity = newCapacity;
        }

        T* _data;
        size_t _size;
        size_t _capacity;
    };
}

#endif //BOREALOS_LIST_H