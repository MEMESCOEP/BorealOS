#ifndef BOREALOS_TSC_H
#define BOREALOS_TSC_H

#include <Definitions.h>

#include "HPET.h"
#include "../CPU.h"

namespace Core::Time {
    class TSC {
    public:
        explicit TSC(HPET* hpet, CPU* cpu);

        [[nodiscard]] uint64_t GetFrequency() const;
        [[nodiscard]] uint64_t GetTicks() const;
        [[nodiscard]] uint64_t GetNanoseconds() const;

    private:
        uint64_t frequency;

        uint64_t CalculateFrequency(HPET * hpet) const;
    };
}

#endif //BOREALOS_TSC_H
