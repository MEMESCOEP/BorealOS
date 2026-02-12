#ifndef BOREALOS_FBCONSOLE_H
#define BOREALOS_FBCONSOLE_H

#include <Definitions.h>
#include "Utility/StringFormatter.h"
#include "Utility/ANSI.h"
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
    };
} // FBConsole

#endif //BOREALOS_FBCONSOLE_H