using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Numerics;
using BorealOS.Managers;

namespace BorealOS.Utilities
{
    public class FBConsoleUtils
    {
        /* VARIABLES */
        public const int FONT_SIZE_X = 8;
        public const int FONT_SIZE_Y = 16;
        public const int MAX_HISTORY = 100_000; // 100k characters, whatever, random arbitrary number. Might be too much or too low IDK!
        
        public static Color BGColor = Color.Black;
        public static Point CursorPosition = new Point(0, 0);
        public static int Scroll = 0; 
        public static Size ConsoleSize = new Size(0, 0);

        private class FBConsoleMessage
        {
            internal string Message;
            internal Color Color;
            internal Point Origin;
        }
        
        private static List<FBConsoleMessage> _messages = new();


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
            DrawRectAtCurrent(BGColor);

            // Replace any tab characters with 4 spaces, and then draw the string
            STR = STR.Replace("\t", "    ");

            _messages.Add(new FBConsoleMessage
                { Message = STR, Color = TextColor, Origin = CursorPosition }); // Track where the message was sent.

            if (_messages.Count > MAX_HISTORY)
            {
                _messages.RemoveAt(0);
            }
            
            bool ForceRescroll = false;
            
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
                    DrawChar(Character, TextColor);
                    CursorPosition.X++;
                }

                // If the X position is greater than the width of the console, reset it back to zero and move down one line
                if (CursorPosition.X >= ConsoleSize.Width)
                {
                    CursorPosition.X = 0;
                    CursorPosition.Y++;
                    ForceRescroll = true;
                }
            }

            // Draw the console cursor
            DrawRectAtCurrent(Color.White);

            // ForceRescroll is true if the cursor is at the bottom of the screen, so we need to scroll up
            if (CursorPosition.Y + Scroll >= ConsoleSize.Height - 1 || ForceRescroll)
            {
                // Increase the scroll by the difference.
                // Genuinely could not get this to work by calculating the difference, so this'll do.
                while (CursorPosition.Y + Scroll >= ConsoleSize.Height - 1)
                {
                    Scroll--;
                }
                
                // Refresh the display
                RefreshDisplay();
            }
            else
            {
                VideoManager.FBCanvas.Display();
            }
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
            Scroll = 0;
            _messages.Clear();
        }

        // Get the length (in characters) of the prompt
        public static int GetPromptLength()
        {
            return $"{UserManager.Username}@{Kernel.Hostname}[{FilesystemManager.CWD}] >> ".Length;
        }

        /// <summary>
        /// Redraw every single message.
        /// </summary>
        public static void RefreshDisplay()
        {
            // Cannot call Clear cause it also removes all messages.
            VideoManager.FBCanvas.Clear(BGColor);
            CursorPosition.X = 0;
            CursorPosition.Y = 0;

            // Unfortunately I cant use the WriteStr function as it will call FBCanvas.Display too many times, which will result in wayyyyyyy too many draw calls (causes really weird bugs)
            foreach (var Message in _messages)
            {
                CursorPosition.Y = Message.Origin.Y;
                
                foreach (var Character in Message.Message)
                {
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
                        DrawChar(Character, Message.Color);

                        CursorPosition.X++;
                    }

                    // If the X position is greater than the width of the console, reset it back to zero and move down one line
                    if (CursorPosition.X >= ConsoleSize.Width)
                    {
                        CursorPosition.X = 0;
                        CursorPosition.Y++;
                    }
                }
            }
            
            DrawRectAtCurrent(Color.White);
            VideoManager.FBCanvas.Display();

        }
        
        public static void ScrollUp()
        {
            Scroll--;
            RefreshDisplay();
        }

        public static void ScrollDown()
        {
            Scroll++;
            RefreshDisplay();
        }

        public static void DrawRectAtCurrent(Color Color)
        {
            int ScrollOffset = FONT_SIZE_Y * Scroll;
            if (CursorPosition.Y + Scroll >= 0 && CursorPosition.Y + Scroll <= ConsoleSize.Height - 1)
            {
                VideoManager.FBCanvas.DrawFilledRectangle(Color, CursorPosition.X * FONT_SIZE_X,
                    CursorPosition.Y * FONT_SIZE_Y + ScrollOffset, FONT_SIZE_X, FONT_SIZE_Y);
            }
        }
        
        private static void DrawChar(char Character, Color Color)
        {
            int ScrollOffset = FONT_SIZE_Y * Scroll;
            if (CursorPosition.Y + Scroll >= 0 && CursorPosition.Y + Scroll <= ConsoleSize.Height - 1)
            {
                VideoManager.FBCanvas.DrawChar(Character, Cosmos.System.Graphics.Fonts.PCScreenFont.Default,
                    Color, CursorPosition.X * FONT_SIZE_X,
                    CursorPosition.Y * FONT_SIZE_Y + ScrollOffset);
            }
        }

        public static void MoveCursorRelative(int XRel, int YRel)
        {
            CursorPosition.X += XRel;
            CursorPosition.Y += YRel;

            if (CursorPosition.X < 0)
            {
                CursorPosition.X = ConsoleSize.Width - 1;
                CursorPosition.Y--;
            }
        }
    }
}
