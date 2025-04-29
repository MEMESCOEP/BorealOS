using System;
using System.Drawing;
using System.Linq;
using BorealOS.Managers;

namespace BorealOS.Utilities
{
    public class Terminal
    {
        /* ENUMS */
        public enum MessageTypes
        {
            INFO,
            ERROR,
            WARNING,
            DEBUG
        }


        /* VARIABLES */
        public static string CurrentCommand = "";


        /* FUNCTIONS */
        // Move the cursor relative to where it currently is
        public static void MoveCursorRelative(int XRel, int YRel)
        {
            FBConsoleUtils.CursorPosition.X = Math.Max(0, Math.Min(FBConsoleUtils.ConsoleSize.Width, FBConsoleUtils.CursorPosition.X + XRel));
            FBConsoleUtils.CursorPosition.Y = Math.Max(0, Math.Min(FBConsoleUtils.ConsoleSize.Height, FBConsoleUtils.CursorPosition.Y + YRel));
        }

        public static void ParseCommand(string Command)
        {
            string TrimmedCommand = Command.TrimStart();
            string[] Args = TrimmedCommand.Split(" ");
            string CMD = TrimmedCommand.Split(" ")[0];

            switch (CMD)
            {
                case "restart":
                case "reboot":
                    if (Args.Length > 2)
                    {
                        FBConsoleUtils.WriteMessage($"The {CMD} command must have exactly 0-1 arguments supplied, not {Args.Length - 1}.\n\r", Color.White, MessageTypes.ERROR);
                        break;
                    }

                    if (Args.Length == 2)
                    {
                        for (int I = 0; I < Args[1].Length; I++)
                        {
                            if (char.IsNumber(Args[1][I]) == false)
                            {
                                FBConsoleUtils.WriteMessage($"The {CMD} command must have numeric arguments supplied.\n\r", Color.White, MessageTypes.ERROR);
                                return;
                            }
                        }

                        PowerManager.PowerActionDelay = Int32.Parse(Args[1]);
                    }

                    PowerManager.Reboot();
                    break;

                case "poweroff":
                case "shutdown":
                case "turnoff":
                    if (Args.Length > 2)
                    {
                        FBConsoleUtils.WriteMessage($"The {CMD} command must have exactly 0-1 arguments supplied, not {Args.Length - 1}.\n\r", Color.White, MessageTypes.ERROR);
                        break;
                    }

                    if (Args.Length == 2)
                    {
                        for (int I = 0; I < Args[1].Length; I++)
                        {
                            if (char.IsNumber(Args[1][I]) == false)
                            {
                                FBConsoleUtils.WriteMessage($"The {CMD} command must have numeric arguments supplied.\n\r", Color.White, MessageTypes.ERROR);
                                return;
                            }
                        }

                        PowerManager.PowerActionDelay = Int32.Parse(Args[1]);
                    }

                    PowerManager.Shutdown();
                    break;

                case "clear":
                case "cls":
                    FBConsoleUtils.Clear();
                    break;

                case "pwd":
                    FBConsoleUtils.WriteMessage($"PWD is \"{FilesystemManager.CWD}\".\n\r", Color.White, MessageTypes.INFO);
                    break;

                case "ls":
                    if (Args.Length > 1)
                    {
                        foreach(string Path in Args.Skip(1))
                        {
                            FilesystemManager.ListDirectory(Path);
                        }
                    }
                    else
                    {
                        FilesystemManager.ListDirectory(FilesystemManager.CWD);
                    }

                    break;

                case "cd":
                    if (Args.Length < 2 || Args.Length > 2)
                    {
                        FBConsoleUtils.WriteMessage($"The cd command must have exactly 1 argument supplied, not {Args.Length - 1}.\n\r", Color.White, MessageTypes.ERROR);
                        break;
                    }

                    FSUtils.ChangeDirectory(Args[1]);
                    break;

                case "realpath":
                    if (Args.Length > 1)
                    {
                        foreach(string Path in Args.Skip(1))
                        {
                            FBConsoleUtils.WriteMessage($"Real path for \"{Path}\" is \"{FSUtils.GetRealPath(Path)}\".\n\r", Color.White, MessageTypes.INFO);
                        }
                    }
                    else
                    {
                        FBConsoleUtils.WriteMessage($"Real path for \"{FilesystemManager.CWD}\" is \"{FSUtils.GetRealPath(FilesystemManager.CWD)}\".\n\r", Color.White, MessageTypes.INFO);
                    }

                    break;

                default:
                    FBConsoleUtils.WriteMessage($"Invalid command \"{CMD}\".\n\r", Color.White, MessageTypes.ERROR);
                    break;
            }
        }
    }
}
