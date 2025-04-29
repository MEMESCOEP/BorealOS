using System.Drawing;
using BorealOS.Utilities;

namespace BorealOS.Utilities
{
    public class TimeUtils
    {
        /* FUNCTIONS */
        public static void WaitSeconds(long SecondsToWait)
        {
            long EndWait = (Cosmos.HAL.RTC.Second + SecondsToWait) % 60;
            while (Cosmos.HAL.RTC.Second != EndWait);
        }
    }
}
