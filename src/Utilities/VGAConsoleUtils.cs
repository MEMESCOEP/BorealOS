using System;
using Cosmos.HAL;

namespace BorealOS.Utilities
{
    public class VGAConsoleUtils
    {
        // Write a message to the console and serial port COM1
        public static void WriteMsg(string Message, Terminal.MessageType MessageType)
        {
            Console.Write("[");
            SerialPort.SendString("[", COMPort.COM1);
            
            Console.Write(Terminal.MessageTypeToString(MessageType));
            SerialPort.SendString($"{Terminal.MessageTypeToString(MessageType)}", COMPort.COM1);
            
            Console.Write("] ");
            SerialPort.SendString("] ", COMPort.COM1);
            
            Console.WriteLine(Message);
            SerialPort.SendString($"{Message}\n\r", COMPort.COM1);
        }
    }
}
