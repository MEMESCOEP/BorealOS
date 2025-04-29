using System.Drawing;
using BorealOS.Utilities;

namespace BorealOS.Managers
{
    public class PowerManager
    {
        /* VARIABLES */
        public static int PowerActionDelay = 5;


        /* FUNCTIONS */
        public static void Shutdown()
        {
            FBConsoleUtils.WriteMessage($"Shutting down in {PowerActionDelay}s...", Color.White, Terminal.MessageTypes.INFO);
            TimeUtils.WaitSeconds(PowerActionDelay);
            Cosmos.System.Power.Shutdown();
        }

        public static void Reboot()
        {
            FBConsoleUtils.WriteMessage($"Rebooting in {PowerActionDelay}s...", Color.White, Terminal.MessageTypes.INFO);
            TimeUtils.WaitSeconds(PowerActionDelay);
            Cosmos.System.Power.Reboot();
        }
    }
}
