#ifndef BOREALOS_FBCONSOLE_H
#define BOREALOS_FBCONSOLE_H

#include <Definitions.h>
#include "Boot/LimineDefinitions.h"
#include "flanterm.h"
#include "flanterm_backends/fb.h"

namespace FBConsole {
    class Console {
    public:
        void Initialize();
        void PrintString(const char* str);

    private:
        limine_framebuffer* framebuffer;
        struct flanterm_context* ftContext;
        bool initialized = false;

        uint32_t defaultFG = 0x0000FF00;  // green
        uint32_t defaultBG = 0x00000000;  // black
    };
} // FBConsole

#endif //BOREALOS_FBCONSOLE_H