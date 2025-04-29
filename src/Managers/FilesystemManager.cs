using System.Collections.Generic;
using System.Drawing;
using System;
using Cosmos.System.FileSystem.VFS;
using Cosmos.System.FileSystem;
using Cosmos.HAL.BlockDevice;
using BorealOS.Utilities;
using System.IO;

namespace BorealOS.Managers
{
    public class FilesystemManager
    {
        /* VARIABLES */
        public static CosmosVFS VirtFS = new CosmosVFS();
        public static List<string[]> DriveMap = new List<string[]>();
        public static string[] RootDriveMap;
        public static string CWD = "NoFS";


        /* FUNCTIONS */
        public static void Init()
        {
            FBConsoleUtils.WriteMessage("Registering VirtFS...\n\r", Color.White, Terminal.MessageType.INFO);
            VFSManager.RegisterVFS(VirtFS);

            FBConsoleUtils.WriteMessage("Mapping disks...\n\r", Color.White, Terminal.MessageType.INFO);

            foreach (Disk VFSDisk in VFSManager.GetDisks())
            {
                switch (VFSDisk.Type)
                {
                    case BlockDeviceType.HardDrive:
                        FBConsoleUtils.WriteStr($"\tFound disk of type \"HDD\" with {VFSDisk.Partitions.Count} recognized partition(s).\n\r", Color.White);
                        MountAllPartitions(VFSDisk);
                        break;

                    case BlockDeviceType.RemovableCD:
                        FBConsoleUtils.WriteStr($"\tFound disk of type \"CD-ROM\" with {VFSDisk.Partitions.Count} recognized partition(s).\n\r", Color.White);
                        MountAllPartitions(VFSDisk);
                        break;

                    case BlockDeviceType.Removable:
                        FBConsoleUtils.WriteStr($"\tFound disk of type \"Removable\" with {VFSDisk.Partitions.Count} recognized partition(s).\n\r", Color.White);
                        MountAllPartitions(VFSDisk);
                        break;

                    default:
                        FBConsoleUtils.WriteStr("\tA disk with an unknown type has been detected, it won't be mapped.\n\r", Color.White);
                        break;
                }
            }

            // Make sure we have an FS path if there are mounted partitions
            if (CWD == "NoFS" && DriveMap.Count > 0)
            {
                RootDriveMap = new string[] {DriveMap[0][0], "/"};
                CWD = RootDriveMap[1];
            }
        }

        // Attempt to mount every partition on the disk
        public static void MountAllPartitions(Disk VFSDisk)
        {
            string DeviceType = "Unknown";

            for (int PartitionIndex = 0; PartitionIndex < VFSDisk.Partitions.Count; PartitionIndex++)
            {
                if (VFSDisk.Partitions[PartitionIndex].MountedFS == null)
                {
                    try
                    {
                        FBConsoleUtils.WriteStr($"\t\tPartition #{PartitionIndex} is not mounted, attempting to mount...\n\r", Color.White);
                        VFSDisk.MountPartition(PartitionIndex);
                    }
                    catch (Exception EX)
                    {
                        FBConsoleUtils.WriteMessage($"Failed to mount partition #{PartitionIndex}: {EX.Message}\n\n\r", Color.White, Terminal.MessageType.ERROR);
                        continue;
                    }
                }

                if (VFSDisk.Partitions[PartitionIndex].HasFileSystem == false)
                {
                    FBConsoleUtils.WriteStr($"\t\tPartition #{PartitionIndex} has no recognized filesystem.\n\n\r", Color.White);
                    continue;
                }

                switch (VFSDisk.Type)
                {
                    case BlockDeviceType.HardDrive:
                        DeviceType = "BLK_HDD";
                        break;

                    case BlockDeviceType.RemovableCD:
                        DeviceType = "BLK_CDROM";
                        break;

                    case BlockDeviceType.Removable:
                        DeviceType = "BLK_RMV";
                        break;

                    default:
                        DeviceType = "BLK_UNKNOWN";
                        break;
                }

                FBConsoleUtils.WriteStr($"\t\tPartition #{PartitionIndex}: \"{VFSDisk.Partitions[PartitionIndex].RootPath}\"\n\n\r", Color.White);
                DriveMap.Add(new string[] {$"{VFSDisk.Partitions[PartitionIndex].RootPath}", $"/{DeviceType}/{VFSDisk.Partitions[PartitionIndex].RootPath.Split(':')[0]}"});
            }
        }

        // Attempt to list the contents of a directory
        public static void ListDirectory(string Path)
        {
            string RealPath = FSUtils.GetRealPath(Path);
            string UnixPath = FSUtils.RelativeToAbsolutePath(Path);

            // Make sure the path actually exists
            if (Directory.Exists(RealPath) == false)
            {
                FBConsoleUtils.WriteMessage($"The path \"{UnixPath}\" either doesn't exist or isn't a directory.\n\r", Color.White, Terminal.MessageType.ERROR);
                return;
            }

            string[] FileList = Directory.GetFiles(RealPath);
            string[] DirList = Directory.GetDirectories(RealPath);

            FBConsoleUtils.WriteStr("[== DIRECTORY LIST (", Color.White);
            FBConsoleUtils.WriteStr(UnixPath, Color.Red);
            FBConsoleUtils.WriteStr(") ==]\n\r", Color.White);
            
            if (FileList.Length == 0 && DirList.Length == 0)
            {
                FBConsoleUtils.WriteStr($"Directory is empty.\n\r", Color.White);
            }
            else
            {
                foreach (string Dir in DirList)
                {
                    FBConsoleUtils.WriteStr($"[DIRECTORY] {Dir}\n\r", Color.White);
                }

                foreach (string File in FileList)
                {
                    FBConsoleUtils.WriteStr($"[FILE] {File}\n\r", Color.White);
                }
            }
        }
    }
}
