#ifndef BOREALOS_ANSI_H
#define BOREALOS_ANSI_H

namespace ANSI {
    namespace EscapeCodes {
        // Appearance
        constexpr const char* ResetAppearance   = "\033[0m"; // Resets the styles and colors to their defaults
        constexpr const char* TextStrikethrough = "\033[9m"; // Enables strikethrough text
        constexpr const char* TextUnderline     = "\033[4m"; // Enables underlined text
        constexpr const char* TextInverse       = "\033[7m"; // Inverts the foreground and background colors
        constexpr const char* TextHidden        = "\033[8m"; // Enables hidden text
        constexpr const char* TextItalic        = "\033[3m"; // Enables italic text
        constexpr const char* TextBlink         = "\033[5m"; // Enables blinking text
        constexpr const char* TextBold          = "\033[1m"; // Enables bold text
        constexpr const char* TextDim           = "\033[2m"; // Enables dim text

        // Cursor control
        constexpr const char* CursorLineStart = "\r";     // Moves the terminal cursor to the start of the current line
        constexpr const char* CursorHome      = "\033[H"; // Moves the terminal cursor to (0, 0)
        
        // Erasing
        constexpr const char* EraseToScreenStart = "\033[1J";        // Clears from the cursor to the start of the screen
        constexpr const char* EraseToScreenEnd   = "\033[0J";        // Clears from the cursor to the end of the screen
        constexpr const char* EraseToLineStart   = "\033[1K";        // Clears from the cursor to the start of the current line
        constexpr const char* EraseToLineEnd     = "\033[0K";        // Clears from the cursor to the end of the current line
        constexpr const char* ClearScreen        = "\033[2J\033[H";  // Clears the entire screen and moves the terminal cursor to (0, 0)
        constexpr const char* ClearLine          = "\033[2K\r";      // Clears the current line and moves the cursor to the start of the current line
    } // EscapeCodes

    namespace Colors {
        namespace Foreground {

            // Normal
            constexpr const char* Magenta = "\033[0;35m";
            constexpr const char* Yellow  = "\033[0;33m";
            constexpr const char* Black   = "\033[0;30m";
            constexpr const char* Green   = "\033[0;32m";
            constexpr const char* White   = "\033[0;37m";
            constexpr const char* Blue    = "\033[0;34m";
            constexpr const char* Cyan    = "\033[0;36m";
            constexpr const char* Red     = "\033[0;31m";

            // Bright
            constexpr const char* BrightMagenta = "\033[1;35m";
            constexpr const char* BrightYellow  = "\033[1;33m";
            constexpr const char* BrightBlack   = "\033[1;30m";
            constexpr const char* BrightGreen   = "\033[1;32m";
            constexpr const char* BrightWhite   = "\033[1;37m";
            constexpr const char* BrightBlue    = "\033[1;34m";
            constexpr const char* BrightCyan    = "\033[1;36m";
            constexpr const char* BrightRed     = "\033[1;31m";

            // Epic (TrueColor)
            constexpr const char* OURBLE = "\033[38;2;12;11;26m";
        }

        namespace Background {

            // Normal
            constexpr const char* Magenta = "\033[45m";
            constexpr const char* Yellow  = "\033[43m";
            constexpr const char* Black   = "\033[40m";
            constexpr const char* Green   = "\033[42m";
            constexpr const char* White   = "\033[47m";
            constexpr const char* Blue    = "\033[44m";
            constexpr const char* Cyan    = "\033[46m";
            constexpr const char* Red     = "\033[41m";

            // Bright
            constexpr const char* BrightMagenta = "\033[105m";
            constexpr const char* BrightYellow  = "\033[103m";
            constexpr const char* BrightBlack   = "\033[100m";
            constexpr const char* BrightGreen   = "\033[102m";
            constexpr const char* BrightWhite   = "\033[107m";
            constexpr const char* BrightBlue    = "\033[104m";
            constexpr const char* BrightCyan    = "\033[106m";
            constexpr const char* BrightRed     = "\033[101m";

            // Epic (TrueColor)
            constexpr const char* OURBLE = "\033[48;2;12;11;26m";
        }

    } // Colors
} // ANSI

#endif //BOREALOS_ANSI_H