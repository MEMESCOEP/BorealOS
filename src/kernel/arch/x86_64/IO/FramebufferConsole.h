#ifndef BOREALOS_FRAMEBUFFERCONSOLE_H
#define BOREALOS_FRAMEBUFFERCONSOLE_H

#include <Definitions.h>
#include "Utility/StringFormatter.h"
#include "Utility/ANSI.h"
#include "Boot/LimineDefinitions.h"
#include "flanterm.h"
#include "flanterm_backends/fb.h"

namespace IO {
    class FramebufferConsole {
    public:
        void Initialize();
        void PrintString(const char* str);

    private:
        limine_framebuffer* framebuffer;
        struct flanterm_context* ftContext;
        bool initialized = false;
    };
} // IO

#endif //BOREALOS_FRAMEBUFFERCONSOLE_H