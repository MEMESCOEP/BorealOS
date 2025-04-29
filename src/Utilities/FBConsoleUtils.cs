using System.Drawing;
using BorealOS.Managers;

namespace BorealOS.Utilities
{
    public class FBConsoleUtils
    {
        /* VARIABLES */
        public static Color BGColor = Color.Black;
        public static Point CursorPosition = new Point(0, 0);
        public static Size ConsoleSize = new Size(0, 0);


        /* FUNCTIONS */
        public static void Init()
        {
            ConsoleSize.Width = (int)(VideoManager.FBCanvas.Mode.Width / Cosmos.System.Graphics.Fonts.PCScreenFont.Default.Width);
            ConsoleSize.Height = (int)(VideoManager.FBCanvas.Mode.Height / Cosmos.System.Graphics.Fonts.PCScreenFont.Default.Height);

            WriteMessage($"Console size is {ConsoleSize.Width}x{ConsoleSize.Height}\n\r", Color.White, Terminal.MessageTypes.INFO);
        }

        // Write a colored string to the screen at the current cursor position
        public static void WriteStr(string STR, Color TextColor)
        {
            // Get rid of the cursor
            VideoManager.FBCanvas.DrawFilledRectangle(BGColor, CursorPosition.X * 8, CursorPosition.Y * 16, 8, 16);

            // Replace any tab characters with 4 spaces, and then draw the string
            STR = STR.Replace("\t", "    ");

            foreach (char Character in STR)
            {
                // If the character is a newline, move down a line. If it's a carriage return, move back to the start of the line
                // If it's neither, draw the character and increment the X position. We don't want to draw \n and \r because they
                // are not ASCII characters
                if (Character == '\n')
                {
                    CursorPosition.Y++;
                }
                else if (Character == '\r')
                {
                    CursorPosition.X = 0;
                }
                else
                {
                    VideoManager.FBCanvas.DrawChar(Character, Cosmos.System.Graphics.Fonts.PCScreenFont.Default, TextColor, CursorPosition.X * 8, CursorPosition.Y * 16);
                    CursorPosition.X++;
                }

                // If the X position is greater than the width of the console, reset it back to zero and move down one line
                if (CursorPosition.X >= ConsoleSize.Width)
                {
                    CursorPosition.X = 0;
                    CursorPosition.Y++;
                }
            }

            // Draw the console cursor
            VideoManager.FBCanvas.DrawFilledRectangle(Color.White, CursorPosition.X * 8, CursorPosition.Y * 16, 8, 16);

            /*if (CursorPosition.Y >= ConsoleSize.Height)
            {
                CursorPosition.Y = ConsoleSize.Height - 1;
            }*/

            VideoManager.FBCanvas.Display();
        }

        public static void WriteMessage(string Message, Color MessageColor, Terminal.MessageTypes MsgType)
        {
            WriteStr("[", Color.White);
            
            switch (MsgType)
            {
                case Terminal.MessageTypes.INFO:
                    WriteStr("INFO", Color.Green);
                    break;

                case Terminal.MessageTypes.ERROR:
                    WriteStr("ERROR", Color.Red);
                    break;

                case Terminal.MessageTypes.WARNING:
                    WriteStr("WARN", Color.Yellow);
                    break;

                case Terminal.MessageTypes.DEBUG:
                    WriteStr("DEBUG", Color.Magenta);
                    break;

                default:
                    WriteStr("MSG_TYPE_NOT_DEFINED", Color.DarkCyan);
                    break;
            }

            WriteStr($"] >> {Message}", Color.White);
        }

        // Draw the prompt in color
        public static void DrawPrompt()
        {
            WriteStr(UserManager.Username, Color.Cyan);
            WriteStr("@", Color.White);
            WriteStr(Kernel.Hostname, Color.Magenta);
            WriteStr("[", Color.White);
            WriteStr(FilesystemManager.CWD, Color.Red);
            WriteStr("] >> ", Color.White);
        }

        // Clear the terminal with a background color
        public static void Clear()
        {
            VideoManager.FBCanvas.Clear(BGColor);
            CursorPosition.X = 0;
            CursorPosition.Y = 0;
        }

        // Get the length (in characters) of the prompt
        public static int GetPromptLength()
        {
            return $"{UserManager.Username}@{Kernel.Hostname}[{FilesystemManager.CWD}] >> ".Length;
        }
    }
}
