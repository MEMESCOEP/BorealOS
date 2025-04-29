using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using BorealOS.Managers;
using Cosmos.Core;
using Cosmos.System.Graphics;

namespace BorealOS.Utilities
{
    public class Terminal
    {
        /* ENUMS */
        public enum MessageType
        {
            INFO,
            ERROR,
            WARNING,
            DEBUG
        }


        /* VARIABLES */
        public static string CurrentCommand = "";


        /* FUNCTIONS */
        /// <summary>
        /// Fucking .ToString() does NOT work for some horrible sad reason. It was one of my favorites :(
        /// </summary>
        public static string MessageTypeToString(MessageType Type)
        {
            return Type switch
            {
                MessageType.INFO => "INFO",
                MessageType.ERROR => "ERROR",
                MessageType.WARNING => "WARNING",
                MessageType.DEBUG => "DEBUG",
                _ => "MSG_TYPE_NOT_DEFINED"
            };
        }

        public static Color MessageTypeToColor(MessageType Type)
        {
            return Type switch
            {
                MessageType.INFO => Color.Green,
                MessageType.ERROR => Color.Red,
                MessageType.WARNING => Color.Yellow,
                MessageType.DEBUG => Color.Cyan,
                _ => Color.Magenta
            };
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
                        FBConsoleUtils.WriteMessage($"The {CMD} command must have exactly 0-1 arguments supplied, not {Args.Length - 1}.\n\r", Color.White, MessageType.ERROR);
                        break;
                    }

                    if (Args.Length == 2)
                    {
                        for (int I = 0; I < Args[1].Length; I++)
                        {
                            if (char.IsNumber(Args[1][I]) == false)
                            {
                                FBConsoleUtils.WriteMessage($"The {CMD} command must have numeric arguments supplied.\n\r", Color.White, MessageType.ERROR);
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
                        FBConsoleUtils.WriteMessage($"The {CMD} command must have exactly 0-1 arguments supplied, not {Args.Length - 1}.\n\r", Color.White, MessageType.ERROR);
                        break;
                    }

                    if (Args.Length == 2)
                    {
                        for (int I = 0; I < Args[1].Length; I++)
                        {
                            if (char.IsNumber(Args[1][I]) == false)
                            {
                                FBConsoleUtils.WriteMessage($"The {CMD} command must have numeric arguments supplied.\n\r", Color.White, MessageType.ERROR);
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
                    FBConsoleUtils.WriteMessage($"PWD is \"{FilesystemManager.CWD}\".\n\r", Color.White, MessageType.INFO);
                    break;

                case "ls":
                    if (Args.Length > 1)
                    {
                        foreach(string Path in Args.Skip(1))
                        {
                            FilesystemManager.ListDirectory(Path);
                            FBConsoleUtils.WriteStr("\n\r", Color.White);
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
                        FBConsoleUtils.WriteMessage($"The cd command must have exactly 1 argument supplied, not {Args.Length - 1}.\n\r", Color.White, MessageType.ERROR);
                        break;
                    }

                    FSUtils.ChangeDirectory(Args[1]);
                    break;

                case "realpath":
                    if (Args.Length > 1)
                    {
                        foreach(string Path in Args.Skip(1))
                        {
                            FBConsoleUtils.WriteMessage($"Real path for \"{Path}\" is \"{FSUtils.GetRealPath(Path)}\".\n\r", Color.White, MessageType.INFO);
                        }
                    }
                    else
                    {
                        FBConsoleUtils.WriteMessage($"Real path for \"{FilesystemManager.CWD}\" is \"{FSUtils.GetRealPath(FilesystemManager.CWD)}\".\n\r", Color.White, MessageType.INFO);
                    }

                    break;

                case "hostname":
                    if (Args.Length < 2 || Args.Length > 2)
                    {
                        FBConsoleUtils.WriteMessage($"The hostname command must have exactly 1 argument supplied, not {Args.Length - 1}.\n\r", Color.White, MessageType.ERROR);
                        break;
                    }

                    Kernel.Hostname = Args[1];
                    break;

                case "getvideomode":
                    FBConsoleUtils.WriteMessage($"\"{VideoManager.FBCanvas.Name()}\", running at {VideoManager.FBCanvas.Mode.Width}x{VideoManager.FBCanvas.Mode.Height}@{VideoManager.GetFBColorDepth(VideoManager.FBCanvas.Mode.ColorDepth)} " +
                        $"(CON={FBConsoleUtils.ConsoleSize.Width}x{FBConsoleUtils.ConsoleSize.Height}).\n\r", Color.White, MessageType.INFO);

                    break;

                case "fbvideomodes":
                    FBConsoleUtils.WriteStr("[== SUPPORTED VIDEO MODES ==]\n\r", Color.White);

                    foreach (Mode VideoMode in VideoManager.FBCanvas.AvailableModes)
                    {
                        FBConsoleUtils.WriteStr($"\t{VideoMode.Width}x{VideoMode.Height}@{VideoManager.GetFBColorDepth(VideoMode.ColorDepth)}", Color.White);

                        if (VideoMode == VideoManager.FBCanvas.Mode)
                        {
                            FBConsoleUtils.WriteStr(" <--", Color.Cyan);
                        }

                        FBConsoleUtils.WriteStr("\n\r", Color.White);
                    }

                    break;

                case "sysinfo":
                    uint TotalMem = CPU.GetAmountOfRAM();
                    float UsedMem = GCImplementation.GetUsedRAM() / 1024.0f;
                    float MemPercentage = (UsedMem / (TotalMem * 1024.0f)) * 100.0f;

                    FBConsoleUtils.WriteStr($"Hostname: {Kernel.Hostname}\n\r", Color.White);

                    if (CPU.CanReadCPUID() != 0)
                    {
                        FBConsoleUtils.WriteStr($"CPU: {CPU.GetCPUBrandString()}, {Math.Max(CPU.GetCPUCycleSpeed() / 1000000000.0f, 0)}GHz\n\r", Color.White);
                    }
                    else
                    {
                        FBConsoleUtils.WriteStr("CPU: Unknown\n\r", Color.White);
                    }

                    FBConsoleUtils.WriteStr($"MEM: {TotalMem}MB total, {UsedMem}KB ({MemPercentage:F2}%) used \n\r", Color.White);
                    break;

                default:
                    FBConsoleUtils.WriteMessage($"Invalid command \"{CMD}\".\n\r", Color.White, MessageType.ERROR);
                    break;
            }
        }
    }
}
