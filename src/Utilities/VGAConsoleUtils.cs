using System;
using Cosmos.HAL;

namespace BorealOS.Utilities
{
    public class VGAConsoleUtils
    {
        // Write a message to the console and serial port COM1
        public static void WriteMsg(string Message, Terminal.MessageTypes MessageType)
        {
            Console.Write("[");
            SerialPort.SendString("[", COMPort.COM1);

            switch (MessageType)
            {
                case Terminal.MessageTypes.INFO:
                    Console.Write("INFO] >> ");
                    SerialPort.SendString("INFO] >> ", COMPort.COM1);
                    break;

                case Terminal.MessageTypes.ERROR:
                    Console.Write("ERROR] >> ");
                    SerialPort.SendString("ERROR] >> ", COMPort.COM1);
                    break;

                case Terminal.MessageTypes.WARNING:
                    Console.Write("WARNING] >> ");
                    SerialPort.SendString("WARNING] >> ", COMPort.COM1);
                    break;

                case Terminal.MessageTypes.DEBUG:
                    Console.Write("DEBUG] >> ");
                    SerialPort.SendString("DEBUG] >> ", COMPort.COM1);
                    break;

                default:
                    Console.Write("MSG_TYPE_NOT_DEFINED] >> ");
                    SerialPort.SendString("MSG_TYPE_NOT_DEFINED] >> ", COMPort.COM1);
                    break;
            }

            Console.WriteLine(Message);
            SerialPort.SendString($"{Message}\n\r", COMPort.COM1);
        }
    }
}
