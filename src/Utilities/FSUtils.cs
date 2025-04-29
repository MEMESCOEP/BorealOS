using System.Drawing;
using System.IO;
using BorealOS.Managers;

namespace BorealOS.Utilities
{
    public class FSUtils
    {
        /* FUNCTIONS */
        // Converts the UNIX-like paths we use to the DOS-like paths Cosmos uses
        public static string GetRealPath(string Path)
        {
            string NewPath = RelativeToAbsolutePath(Path);

            if (Path.StartsWith("/BLK_"))
            {
                FBConsoleUtils.WriteStr($"{Path}\n\r", Color.White);
            }

            return StrUtils.ReplaceFirstOccurrence("/", FilesystemManager.RootDriveMap[0], NewPath).Replace("/", "\\");
        }

        // Convert a relative path into an absolute path
        public static string RelativeToAbsolutePath(string RelPath)
        {
            if (RelPath.StartsWith("/") == false)
            {
                if (FilesystemManager.CWD.EndsWith("/"))
                {
                    return $"{FilesystemManager.CWD.Substring(0, FilesystemManager.CWD.Length - 1)}/{RelPath}";
                }
                else
                {
                    return $"{FilesystemManager.CWD}/{RelPath}";
                }
            }

            return RelPath;
        }

        // Change the CWD to a new one, if the new directory exists
        public static void ChangeDirectory(string NewDir)
        {
            string RelPath = RelativeToAbsolutePath(NewDir);

            // Make sure the directory exists
            if (Directory.Exists(GetRealPath(NewDir)) == false)
            {
                FBConsoleUtils.WriteMessage($"The directory \"{RelPath}\" doesn't exist.\n\r", Color.White, Terminal.MessageTypes.ERROR);
                return;
            }

            FilesystemManager.CWD = RelPath;
        }
    }
}
