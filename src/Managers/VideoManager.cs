using System.Drawing;
using Cosmos.System.Graphics;

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
            Utilities.VGAConsoleUtils.WriteMsg("Initializing canvas...", Utilities.Terminal.MessageTypes.INFO);
            FBCanvas = FullScreenCanvas.GetFullScreenCanvas(new Mode(1024, 768, ColorDepth.ColorDepth32));
            FBCanvas.Clear(Color.Black);
            FBCanvas.Display();
        }
    }
}
