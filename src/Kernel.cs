using System.Drawing;
using System;
using Cosmos.HAL;
using BorealOS.Utilities;
using Sys = Cosmos.System;

namespace BorealOS
{
    // Kernel entry point
    public class Kernel : Sys.Kernel
    {
        /* VARIABLES */
        public static string Hostname = "BorealOS";


        /* FUNCTIONS */
        // Perform initialization steps here
        protected override void BeforeRun()
        {
            Console.WriteLine("[== BOREALOS ==]");

            // Initialize serial port COM1 with a baud rate of 9600
            VGAConsoleUtils.WriteMsg("[INFO] >> Initializing serial port COM1 (BRate=9600)...", Terminal.MessageType.INFO);
            SerialPort.Enable(COMPort.COM1, BaudRate.BaudRate9600);

            // Now we can initialize the graphics
            Managers.VideoManager.Init();
            FBConsoleUtils.Init();

            // Initialize the VirtFS
            Managers.FilesystemManager.Init();

            // Initialization is done, now we can print OS information and start the shell
            FBConsoleUtils.Clear();
            FBConsoleUtils.WriteStr("<== Welcome to BorealOS! ==>\n\r", Color.White);
            FBConsoleUtils.WriteMessage($"Running at {Managers.VideoManager.FBCanvas.Mode.Width}x{Managers.VideoManager.FBCanvas.Mode.Height} (CON={FBConsoleUtils.ConsoleSize.Width}x{FBConsoleUtils.ConsoleSize.Height}).\n\r", Color.White, Terminal.MessageType.INFO);
            FBConsoleUtils.DrawPrompt();
        }
        
        // Now the OS can do other things here; this is essentially "while (true)"
        protected override void Run()
        {
            var Key = Console.ReadKey();

            if (Key.Key == ConsoleKey.Enter)
            {
                int PreviousYPos = FBConsoleUtils.CursorPosition.Y;

                if (Terminal.CurrentCommand.Length > 0)
                {
                    FBConsoleUtils.WriteStr("\n\r", Color.White);
                    Terminal.ParseCommand(Terminal.CurrentCommand);
                }
                
                if (FBConsoleUtils.CursorPosition.Y - PreviousYPos >= 2 || Terminal.CurrentCommand.Length <= 0)
                {
                    FBConsoleUtils.WriteStr("\n\r", Color.White);
                }
                
                FBConsoleUtils.DrawPrompt();
                Terminal.CurrentCommand = "";
            }
            else if (Key.Key == ConsoleKey.Backspace)
            {
                // Get rid of the previous console
                FBConsoleUtils.DrawRectAtCurrent(FBConsoleUtils.BGColor);

                if (Terminal.CurrentCommand.Length > 0)
                {
                    // Remove the last character from the command
                    Terminal.CurrentCommand = Terminal.CurrentCommand.Substring(0, Terminal.CurrentCommand.Length - 1);
                    FBConsoleUtils.MoveCursorRelative(-1, 0);
                }
                
                // Redraw the cursor
                FBConsoleUtils.DrawRectAtCurrent(Color.White);
            }
            else if (Key.Key == ConsoleKey.PageDown)
            {
                // Reversed is nicer.
                FBConsoleUtils.ScrollUp();
            }
            else if (Key.Key == ConsoleKey.PageUp)
            {
                FBConsoleUtils.ScrollDown();
            }
            else if (StrUtils.IsCharTypable(Key.KeyChar))
            {
                Terminal.CurrentCommand += Key.KeyChar;
                FBConsoleUtils.WriteStr(Key.KeyChar.ToString(), Color.White);
            }

            // Update the framebuffer
            Managers.VideoManager.FBCanvas.Display();
        }
    }
}
