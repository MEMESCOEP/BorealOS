using System.Drawing;
using System;
using Cosmos.Core.Memory;
using Cosmos.HAL;
using Sys = Cosmos.System;
using BorealOS.Utilities;
using BorealOS.Managers;

namespace BorealOS
{
    // Kernel entry point
    public class Kernel : Sys.Kernel
    {
        /* VARIABLES */
        public static string Hostname = "BorealOS";
        private ulong CursorBlinkRate = 125000000;
        private bool CursorOn = false;
        private int RATFreePages = 10;


        /* FUNCTIONS */
        // Perform initialization steps here
        protected override void BeforeRun()
        {
            VGAConsoleUtils.WriteMsg($"[INFO] >> Initializing RAT minpages ({RATFreePages} page(s))...", Terminal.MessageType.INFO);
            RAT.MinFreePages = RATFreePages;

            VGAConsoleUtils.WriteMsg($"[INFO] >> Initializing PIT...", Terminal.MessageType.INFO);
            PIT SysPIT = new PIT();

            // Initialize serial port COM1 with a baud rate of 9600
            VGAConsoleUtils.WriteMsg("[INFO] >> Initializing serial port COM1 (BRate=9600)...", Terminal.MessageType.INFO);
            SerialPort.Enable(COMPort.COM1, BaudRate.BaudRate9600);

            // Now we can initialize the graphics
            VideoManager.Init();
            FBConsoleUtils.Init();

            // Create a PIT timer to blink the FB cursor
            FBConsoleUtils.WriteMessage($"Initializing cursor blink PIT timer ({CursorBlinkRate / 1000000}ms)...\n\r", Color.White, Terminal.MessageType.INFO);
            PIT.PITTimer CursorBlinkTimer = new PIT.PITTimer(BlinkCursor, CursorBlinkRate, true);
            SysPIT.RegisterTimer(CursorBlinkTimer);

            // Initialize the VirtFS
            FilesystemManager.Init();

            // Initialization is done, now we can print OS information and start the shell
            FBConsoleUtils.WriteMessage("Init done.\n\r", Color.White, Terminal.MessageType.INFO);
            SysPIT.Wait(2000);

            FBConsoleUtils.Clear();
            FBConsoleUtils.WriteStr("<== Welcome to ", Color.White);
            FBConsoleUtils.WriteStr("BorealOS", Color.FromArgb(255, 59, 214, 198));
            FBConsoleUtils.WriteStr("! ==>\n\r", Color.White);
            FBConsoleUtils.WriteMessage($"\"{VideoManager.FBCanvas.Name()}\", running at {VideoManager.FBCanvas.Mode.Width}x{VideoManager.FBCanvas.Mode.Height}@{VideoManager.GetFBColorDepth(VideoManager.FBCanvas.Mode.ColorDepth)} " +
                $"(CON={FBConsoleUtils.ConsoleSize.Width}x{FBConsoleUtils.ConsoleSize.Height}).\n\r", Color.White, Terminal.MessageType.INFO);

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
            VideoManager.FBCanvas.Display();
        }

        // Blink the FBconsole cursor
        private void BlinkCursor()
        {
            if (Console.KeyAvailable == true)
                return;

            CursorOn = !CursorOn;

            FBConsoleUtils.DrawRectAtCurrent(CursorOn == true ? Color.White : Color.Black);
            VideoManager.FBCanvas.Display();
        }
    }
}
