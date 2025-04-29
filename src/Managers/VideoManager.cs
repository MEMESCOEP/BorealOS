using System.Drawing;
using Cosmos.System.Graphics;
using static BorealOS.Utilities.Terminal;

namespace BorealOS.Managers
{
    public class VideoManager
    {
        /* VARIABLES */
        public static Canvas FBCanvas;


        /* FUNCTIONS */
        // Initialize the framebuffer (called the canvas)
        public static void Init()
        {
            Utilities.VGAConsoleUtils.WriteMsg("Initializing canvas...", Utilities.Terminal.MessageType.INFO);
            FBCanvas = FullScreenCanvas.GetFullScreenCanvas();
            FBCanvas.Clear(Color.Black);
            FBCanvas.Display();
        }

        public static int GetFBColorDepth(ColorDepth ClrDepth)
        {
            return ClrDepth switch
            {
                ColorDepth.ColorDepth4 => 4,
                ColorDepth.ColorDepth8 => 8,
                ColorDepth.ColorDepth16 => 16,
                ColorDepth.ColorDepth24 => 24,
                ColorDepth.ColorDepth32 => 32,
                _ => -1
            };
        }
    }
}
